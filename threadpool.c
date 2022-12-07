#define _XOPEN_SOURCE
#include <ucontext.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <x86intrin.h>

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
    void *arg;
    tpool_work work;
    tpool_handle *handle;
    uint8_t *stack;
    ucontext_t context;
} task_t;

struct tpool_pool {
    tpool_queue *task_queue;
    tpool_queue *result_queue;

    size_t task_count;
    pthread_mutex_t task_count_mutex;
    pthread_cond_t task_count_cond;

    size_t pool_size;
    pthread_t threads[];
};

typedef struct thread_data {
    ucontext_t yield_context;
    ucontext_t return_context;
    void *aux1;
    void *aux2;
    size_t id;
    task_t *curr_task;
    pthread_t self;
} tdata_t;

#define TPOOL_DEFAULT_SIZE 16

pthread_once_t thread_local_init = PTHREAD_ONCE_INIT;
pthread_key_t thread_local_key;

__thread volatile tdata_t *tdata;

static volatile tdata_t *get_tdata();

#define _LOG_INNER(PREFIX, FMT, ARGS...)\
    do {\
        if (get_tdata() == NULL) {\
            fprintf(stderr, PREFIX "M:   %s:%d: %s: " FMT, __FILE__, __LINE__, __func__ ,##ARGS);\
        } else {\
            fprintf(stderr, PREFIX "T%02lu: %s:%d: %s: " FMT, get_tdata()->id, __FILE__, __LINE__, __func__ ,##ARGS);\
        }\
    } while (0)

#define LOG(FMT, ARGS...) _LOG_INNER("", FMT, ##ARGS)

#define ERROR(ARGS...)\
    do {\
        _LOG_INNER("ERROR: ", ARGS);\
        _mm_mfence();\
        exit(*(volatile int *) 0xFA);\
    } while (0)

#define LOG_LEVEL 2

#if (LOG_LEVEL >= 1)
#define WARN LOG
#else
#define WARN(...) do {} while (0)
#endif

#if (LOG_LEVEL >= 2)
#define INFO LOG
#else
#define INFO(...) do {} while (0)
#endif

#if (LOG_LEVEL >= 3)
#define DEBUG LOG
#else
#define DEBUG(...) do {} while (0)
#endif

#if (LOG_LEVEL >= 4)
#define VERBOSE LOG
#else
#define VERBOSE(...) do {} while (0)
#endif

#define ASSERT_TRACE(EXPR...)\
    do {\
        if (!(EXPR)) {\
            ERROR("Assertion `" #EXPR "` failed.\n");\
        }\
    } while (0)

#define UNIMPLEMENTED(...) ERROR("Unimplemented.\n")

#define UNREACHABLE(...) ERROR("Unreachable.\n")

static volatile tdata_t *get_tdata() {
    if ((volatile tdata_t *) tdata != NULL) {
        ASSERT_TRACE(pthread_equal(pthread_self(), tdata->self)
            && "Thread contexted switched without properly handling thread local data.");
    }
    return tdata;
}

static void tdata_free(void *data) {
    free(data);
}

static void thread_local_init_func() {
    pthread_key_create(&thread_local_key, tdata_free);
}

#define GETCONTEXT(ARGS...) do {\
    _mm_mfence();\
    getcontext(ARGS);\
    _mm_mfence();\
} while (0)

#define SETCONTEXT(ARGS...) do {\
    _mm_mfence();\
    setcontext(ARGS);\
    UNREACHABLE();\
} while (0)

#define SWAPCONTEXT(ARGS...) do {\
    _mm_mfence();\
    swapcontext(ARGS);\
    _mm_mfence();\
} while (0)

#define PAGE_SIZE 4096
#define TASK_STACK_SIZE PAGE_SIZE * 128

static void print_context(ucontext_t *context) {
    (void) context;
    VERBOSE("\n"
            "\tRIP:         %p\n"
            "\tRSP:         %p\n"
            "\tStack start: %p\n"
            "\tStack size:  %lu\n",
            (void *) context->uc_mcontext->__ss.__rip,
            (void *) context->uc_mcontext->__ss.__rsp,
            (void *) context->uc_stack.ss_sp,
            context->uc_stack.ss_size);
}

static void task_wrapper() {
    tpool_work work = get_tdata()->aux1;
    void *arg = get_tdata()->aux2;
    DEBUG("Started with arg %p\n", arg);
    void *out = work(arg); // Calling thread and returning thread may differ.
    DEBUG("Finished with ret %p\n", out);
    get_tdata()->aux1 = out;
    SETCONTEXT(&get_tdata()->return_context);
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
    get_tdata()->curr_task = task;
    if (task->type == INITIAL) {
        DEBUG("Initializing task %p\n", task->handle);
        getcontext(&task->context);
        task->stack = malloc(TASK_STACK_SIZE);
        task->context.uc_stack = (stack_t) {
            .ss_sp = task->stack,
            .ss_size = TASK_STACK_SIZE,
            .ss_flags = 0
        };
        get_tdata()->aux1 = task->work;
        get_tdata()->aux2 = task->arg;
        makecontext(&task->context, task_wrapper, 0);
    } else if (task->type == RESUME) {
        DEBUG("Resuming task %p\n", task->handle);
    } else {
        ERROR("Invalid task type.\n");
    }
    SWAPCONTEXT(&get_tdata()->return_context, &task->context);

    DEBUG("Returning from task %p with value %p\n", task->handle, get_tdata()->aux1);
    free(get_tdata()->curr_task->stack);
    free(get_tdata()->curr_task);
    get_tdata()->curr_task = NULL;
    return get_tdata()->aux1;
}

size_t num_queued_tasks(tpool_pool *pool) {
    pthread_mutex_lock(&pool->task_queue->body_mutex);
    size_t ret = pool->task_queue->count;
    pthread_mutex_unlock(&pool->task_queue->body_mutex);
    return ret;
}

static bool launch_task(tpool_pool *pool) {
    task_t *task = tpool_dequeue(pool->task_queue);

    if (task == NULL) {
        return true;
    }

    tpool_handle *handle = task->handle;
    void *result = run_task(task); // May not return.
    if (handle == (void *) QUEUE_RESULT) {
        tpool_enqueue(pool->result_queue, result);
    }
    else if (handle == (void *) DISCARD_RESULT) {
        // Do nothing with result.
    }
    else {
        handle->result = result;

        pthread_mutex_lock(&handle->mutex);
        handle->status = FINISHED;
        pthread_cond_broadcast(&handle->result_cond);
        pthread_mutex_unlock(&handle->mutex);
        DEBUG("Signaled handle %p\n", handle);
    }
    pthread_mutex_lock(&pool->task_count_mutex);
    pool->task_count--;
    if (pool->task_count == 0) {
        pthread_cond_broadcast(&pool->task_count_cond);
    }
    pthread_mutex_unlock(&pool->task_count_mutex);
    DEBUG("Finished task %p\n", handle);
    return false;
}

static void *pool_thread(void *arg) {
    tdata = calloc(1, sizeof(tdata_t));
    tdata->self = pthread_self();
    tdata->id = *(size_t *) (arg + sizeof(tpool_pool *));
    tdata->curr_task = NULL;
    // pthread_setspecific(thread_local_key, tdata);

    tpool_pool *pool = *(tpool_pool **) arg;
    free(arg);

    GETCONTEXT(&get_tdata()->yield_context);
    DEBUG("Passed yield context.\n");
    if (get_tdata()->curr_task != NULL) {
        DEBUG("Enqueued task.\n");
        tpool_enqueue(pool->task_queue, get_tdata()->curr_task);
        get_tdata()->curr_task = NULL;
    }
    while (true) {
        if (launch_task(pool)) {
            return NULL;
        }
    }
}

tpool_pool *tpool_init(size_t size) {
    if (size == 0) {
        size = TPOOL_DEFAULT_SIZE;
    }
    pthread_once(&thread_local_init, thread_local_init_func);

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
        int ret = pthread_create(&pool->threads[i], NULL, pool_thread, thread_start_arg);
        if (ret) {
            return NULL;
        }
    }
    return pool;
}

tpool_list *tpool_close(tpool_pool *pool, bool get_results) {
    size_t size = pool->pool_size;
    
    // queue will return NULL instead of blocking when empty
    pthread_mutex_lock(&pool->task_count_mutex);
    while (pool->task_count > 0) {
        pthread_cond_wait(&pool->task_count_cond, &pool->task_count_mutex);
    }
    tpool_queue_unblock(pool->task_queue);
    pthread_mutex_unlock(&pool->task_count_mutex);

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
 * May return in different thread than the caller, but returns exactly once.
 * 
 * @param pool 
 */
void yield() {
    get_tdata()->curr_task->type = RESUME;
    DEBUG("Yielding task %p.\n", get_tdata()->curr_task->handle);
    SWAPCONTEXT(&get_tdata()->curr_task->context, &get_tdata()->yield_context); // doesn't return
    DEBUG("Resuming %p.\n", get_tdata()->curr_task->handle);
}

void *tpool_task_await(tpool_handle *handle) {
    DEBUG("Awaiting handle %p.\n", handle);
    if (get_tdata()) {
        pthread_mutex_lock(&handle->mutex);
        while (handle->status == WAITING && num_queued_tasks(handle->pool) > 0) {
            pthread_mutex_unlock(&handle->mutex);
            yield(); // Potentially returns in different thread.
            pthread_mutex_lock(&handle->mutex);
        }
    } else {
        pthread_mutex_lock(&handle->mutex);
    }
    while (handle->status == WAITING) {
        pthread_cond_wait(&handle->result_cond, &handle->mutex);
    }
    void *result = handle->result;
    pthread_mutex_unlock(&handle->mutex);
    DEBUG("Done waiting on handle %p.\n", handle);
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
    pool = tpool_init(0);

    // for (size_t i = 0; i < 100; i++) {
    uintptr_t f = (uintptr_t) fibonacci((void *) 20);
    printf("%lu\n", f);

    tpool_close(pool, false);
}

tpool_list *tpool_wait(tpool_pool *pool, bool get_results) {
    (void) pool;
    (void) get_results;
    UNIMPLEMENTED();
}

tpool_list *tpool_map(tpool_pool *pool, tpool_list *list, tpool_work func) {
    (void) pool;
    (void) list;
    (void) func;
    UNIMPLEMENTED();
}
