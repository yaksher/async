#define _XOPEN_SOURCE
#include <ucontext.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "threadpool.h"
#include "queue.h"

struct tpool_queue {
    pthread_mutex_t body_mutex;
    tpool_list in;
    tpool_list out;
    size_t count;
    pthread_mutex_t new_mut;
    pthread_cond_t new_cond;
    bool closing;
    size_t n_waiting;
    size_t wait_threshold;
};

struct tpool_handle {
    tpool_task_status status;
    pthread_mutex_t mutex;
    pthread_cond_t result_cond;
    tpool_pool *pool;
    void *result;
};

typedef enum {INITIAL, RESUME} task_type_t;


typedef struct {
    task_type_t type;
    tpool_work work;
    void *arg;
    tpool_handle *handle;
    ucontext_t context;
} task_t;

struct task_t {
    void *arg;
    tpool_work work;
    tpool_handle *handle;
};

struct tpool_pool {
    tpool_queue *task_queue;
    tpool_queue *result_queue;

    size_t task_count;
    pthread_mutex_t task_count_mutex;

    size_t pool_size;
    pthread_t threads[];
};

typedef struct thread_data {
    ucontext_t *yield_context;
    ucontext_t *return_context;
    void *aux1;
    void *aux2;
    size_t id;
    tpool_handle *curr_handle;
} tdata_t;

#define TPOOL_DEFAULT_SIZE 16

__thread tdata_t *tdata = NULL;

#define PAGE_SIZE 4096
#define TASK_STACK_SIZE PAGE_SIZE * 128

static void set_rip(ucontext_t *context, void *rip) {
    assert((uintptr_t) rip >= PAGE_SIZE);
    context->uc_mcontext->__ss.__rip = (uintptr_t) rip;
}

static void print_context(ucontext_t *context) {
    return;
    fprintf(stderr, "Context:\n");
    fprintf(stderr, "RIP:         %p\n", (void *) context->uc_mcontext->__ss.__rip);
    fprintf(stderr, "RSP:         %p\n", (void *) context->uc_mcontext->__ss.__rsp);
    fprintf(stderr, "Stack start: %p\n", (void *) context->uc_stack.ss_sp);
    fprintf(stderr, "Stack size:  %lu\n", context->uc_stack.ss_size);
}

static void task_call_wrapper() {
    tpool_work work = tdata->aux1;
    void *arg = tdata->aux2;
    fprintf(stderr, "Thread %lu started a task\n", tdata->id);
    tdata->aux1 = work(arg);
    fprintf(stderr, "Thread %lu finished a task\n", tdata->id);
    print_context(tdata->return_context);
    setcontext(tdata->return_context);
}

/**
 * @brief Begins or continues execution of a task. If the task contains
 * an asynchronous call, the task will be suspended and the thread will
 * return to the thread pool. If the task completes, the result will be
 * returned.
 * 
 * Notably, it is possible that this function will not return at all and the
 * thread will continue execution inside work_launcher.
 * 
 * @param task 
 * @return void* 
 */
static void *run_task(task_t *task) {
    ucontext_t return_context;
    bool returning = false;
    getcontext(&return_context);
    fprintf(stderr, "Thread %lu passing getcontext\n", tdata->id);
    if (returning) {
        fprintf(stderr, "Thread %lu returning from task\n", tdata->id);
        return tdata->aux1;
    }
    returning = true;
    ucontext_t context;

    if (task->type == INITIAL) {
        getcontext(&context);
        context.uc_stack = (stack_t) {
            .ss_sp = calloc(TASK_STACK_SIZE, 1),
            .ss_size = TASK_STACK_SIZE,
            .ss_flags = 0
        };
        tdata->aux1 = task->work;
        tdata->aux2 = task->arg;
        tdata->return_context = &return_context;
        // set_rip(&context, task_call_wrapper);
        // context.uc_link = &return_context;
        makecontext(&context, task_call_wrapper, 0);
    } else if (task->type == RESUME) {
        fprintf(stderr, "Resuming task\n");
        print_context(&task->context);
        context = task->context;
        tdata->return_context = &return_context;
        // context.uc_link = &return_context;
    } else {
        assert(false && "Invalid task type.");
    }
    free(task);
    setcontext(&context);
    assert(false && "Unreachable.");
}

static bool launch_task(tpool_pool *pool) {
    task_t *task = tpool_dequeue(pool->task_queue);

    if (task == NULL) {
        return true;
    }

    tdata->curr_handle = task->handle;
    if (task->handle == (void *) QUEUE_RESULT) {
        tpool_enqueue(pool->result_queue, run_task(task));
    }
    else if (task->handle == (void *) DISCARD_RESULT) {
        run_task(task);
    }
    else {
        tpool_handle *handle = task->handle;
        handle->result = run_task(task);

        pthread_mutex_lock(&handle->mutex);
        handle->status = FINISHED;
        pthread_cond_broadcast(&handle->result_cond);
        pthread_mutex_unlock(&handle->mutex);
    }
    pthread_mutex_lock(&pool->task_count_mutex);
    pool->task_count--;
    pthread_mutex_unlock(&pool->task_count_mutex);
    return false;
}

static void *work_launcher(void *pool) {
    ucontext_t yield_context;
    tdata->yield_context = &yield_context;
    getcontext(&yield_context);
    while (true) {
        if (launch_task(pool)) {
            return NULL;
        }
    }
}

static void *thread_start(void *arg) {
    tdata = calloc(1, sizeof(tdata_t));
    tdata->id = *(size_t *) (arg + sizeof(tpool_pool *));
    tpool_pool *pool = *(tpool_pool **) arg;
    free(arg);
    return work_launcher(pool);
}

tpool_pool *tpool_init(size_t size) {
    if (size == 0) {
        size = TPOOL_DEFAULT_SIZE;
    }

    tpool_pool *pool = malloc(sizeof(tpool_pool) + sizeof(pthread_t) * size);

    pool->pool_size = size;
    pool->task_count = 0;

    assert(!pthread_mutex_init(&pool->task_count_mutex, NULL));

    pool->task_queue = tpool_queue_init();
    pool->result_queue = tpool_queue_init();
    pool->task_queue->wait_threshold = size;

    // spawn the thread pool
    for (size_t i = 0; i < size; i++) {
        void *thread_start_arg = malloc(sizeof(tpool_pool *) + sizeof(size_t));
        *(tpool_pool **) thread_start_arg = pool;
        *(size_t *) (thread_start_arg + sizeof(tpool_pool *)) = i;
        int ret = pthread_create(&pool->threads[i], NULL, thread_start, thread_start_arg);
        if (ret) {
            return NULL;
        }
    }
    return pool;
}

tpool_list *tpool_close(tpool_pool *pool, bool get_results) {
    size_t size = pool->pool_size;
    
    // queue will return NULL instead of blocking when empty
    tpool_queue_close(pool->task_queue);

    for (size_t i = 0; i < size; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    tpool_list *ret = NULL;
    if (get_results) {
        ret = malloc(sizeof(tpool_list));
        tpool_list_init(ret);
        // all other threads are closed at this point, no need for mutexes
        tpool_list *in = &pool->result_queue->in;
        tpool_list *out = &pool->result_queue->out;
        for (
            void *item = tpool_list_pop(out);
            item != NULL;
            item = tpool_list_pop(out)
        ) {
            tpool_list_push(ret, item);
        }
        for (size_t i = 0; i < in->count; i++) {
            tpool_list_push(ret, in->data[i]);
        }
    }
    tpool_queue_free(pool->task_queue);
    tpool_queue_free(pool->result_queue);
    free(pool);
    return ret;
}

/**
 * @brief Yields execution to the threadpool, enqueueing a resume task so that
 * the threadpool can resume execution of the current task eventually.
 * 
 * @param pool 
 */
void yield(tpool_pool *pool) {
    task_t *task = malloc(sizeof(task_t));
    task->type = RESUME;
    task->handle = tdata->curr_handle;
    bool returning = false;
    getcontext(&task->context); // returns here when resumed
    fprintf(stderr, "Thread %lu passed save context with returning %d\n", tdata->id, returning);
    if (!returning) {
        returning = true;
        tpool_enqueue(pool->task_queue, task);
        fprintf(stderr, "Thread %lu yielding\n", tdata->id);
        print_context(tdata->yield_context);
        setcontext(tdata->yield_context); // doesn't return
    }
}

void *tpool_task_await(tpool_handle *handle) {
    if (tdata) {
        yield(handle->pool);
    }
    // Else, this is not one of the threadpool threads, so it should just wait,
    // rather than yield control.

    pthread_mutex_lock(&handle->mutex);
    while (handle->status == WAITING) {
        pthread_cond_wait(&handle->result_cond, &handle->mutex);
    }
    void *result = handle->result;
    pthread_mutex_unlock(&handle->mutex);
    free(handle);

    return result;
}

void *tpool_result_dequeue(tpool_pool *pool) {
    return tpool_dequeue(pool->result_queue);
}

static tpool_handle *task_handle_init(tpool_pool *pool) {
    tpool_handle *handle = malloc(sizeof(tpool_handle));
    handle->result = NULL;
    handle->status = WAITING;
    handle->pool = pool;
    pthread_mutex_init(&handle->mutex, NULL);
    pthread_cond_init(&handle->result_cond, NULL);
    return handle;
}

tpool_handle *tpool_task_enqueue(tpool_pool *pool, tpool_work work, void *arg, tpool_option option) {
    task_t *task = malloc(sizeof(task_t));
    tpool_handle *handle;
    if (option == HANDLE) {
        handle = task_handle_init(pool);
    }
    else {
        handle = (void *) option;
    }
    task->type = INITIAL;
    task->work = work;
    task->arg = arg;
    task->handle = handle;
    tpool_enqueue(pool->task_queue, task);

    pthread_mutex_lock(&pool->task_count_mutex);
    pool->task_count++;
    pthread_mutex_unlock(&pool->task_count_mutex);

    return (void *) handle > (void *) DISCARD_RESULT ? handle : NULL;
}

void *test_func(void *arg) {
    sleep(1);
    uint64_t *ret = malloc(sizeof(uint64_t));
    *ret = ((uint64_t) arg) * ((uint64_t) arg);
    return ret;
}

tpool_pool *pool;

void *fibonacci(void *arg) {
    uintptr_t val = (uintptr_t) arg;
    if (val == 0) {
        return (void *) 0;
    } else if (val == 1) {
        return (void *) 1;
    }
    tpool_handle *handle = tpool_task_enqueue(pool, fibonacci, (void *) val - 2, HANDLE);
    uintptr_t n1 = (uintptr_t) fibonacci((void *) val - 1);
    uintptr_t n2 = (uintptr_t) tpool_task_await(handle);
    return (void *) n1 + n2;
}

int main() {
    // tpool_pool *pool = tpool_init(0);
    pool = tpool_init(2);

    uintptr_t f = (uintptr_t) fibonacci((void *) 6);
    printf("%lu\n", f);

    tpool_close(pool, false);
}

tpool_list *tpool_wait(tpool_pool *pool, bool get_results) {
    (void) pool;
    (void) get_results;
    assert(false && "ERROR: Unimplimented!");
}

tpool_list *tpool_map(tpool_pool *pool, tpool_list *list, tpool_work func) {
    (void) pool;
    (void) list;
    (void) func;
    assert(false && "ERROR: Unimplimented!");
}
