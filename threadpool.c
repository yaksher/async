#define _XOPEN_SOURCE
#include <ucontext.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "threadpool.h"
#include "queue.h"

struct tpool_handle {
    enum {WAITING, FINISHED, ABORTING} status;
    pthread_mutex_t mutex;
    pthread_cond_t result_cond;
    tpool_pool *pool;
    void *result;
};

typedef struct {
    enum {INITIAL, RESUME, ABORT} type;
    void *arg;
    tpool_work work;
    tpool_handle *handle;
    uint8_t *stack;
    ucontext_t context;
} task_t;

struct tpool_pool {
    tpool_queue *task_queue;

    size_t task_count;
    pthread_mutex_t task_count_mutex;
    pthread_cond_t task_count_cond;

    size_t pool_size;
    pthread_t threads[];
};

typedef struct thread_data {
    ucontext_t yield_context;
    ucontext_t return_context;
    size_t id;
    task_t *curr_task;
    pthread_t self;
} tdata_t;

#define TPOOL_DEFAULT_SIZE 16

pthread_once_t thread_local_init = PTHREAD_ONCE_INIT;
pthread_key_t thread_local_key;

__thread tdata_t *tdata;

static tdata_t *get_tdata();

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

#define ASSERT(EXPR...)\
    do {\
        if (!(EXPR)) {\
            ERROR("Assertion `" #EXPR "` failed.\n");\
        }\
    } while (0)

#define UNIMPLEMENTED(...) ERROR("Unimplemented.\n")

#define UNREACHABLE(...) ERROR("Unreachable.\n")

static tdata_t *get_tdata() {
    if ((volatile tdata_t *) tdata != NULL) {
        ASSERT(pthread_equal(pthread_self(), tdata->self)
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

#define PAGE_SIZE 4096
#define TASK_STACK_SIZE PAGE_SIZE * 16

static void task_wrapper() {
    tdata_t *tdata = get_tdata();
    void *out = tdata->curr_task->work(tdata->curr_task->arg);
    tdata = get_tdata();
    tdata->curr_task->arg = out;
    setcontext(&tdata->return_context);
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
    tdata_t *tdata = get_tdata();
    tdata->curr_task = task;
    if (task->type == INITIAL) {
        DEBUG("Initializing task %p\n", task->handle);
        getcontext(&task->context);
        task->stack = malloc(TASK_STACK_SIZE);
        task->context.uc_stack = (stack_t) {
            .ss_sp = task->stack,
            .ss_size = TASK_STACK_SIZE,
            .ss_flags = 0
        };
        makecontext(&task->context, task_wrapper, 0);
    } else if (task->type == RESUME) {
        DEBUG("Resuming task %p\n", task->handle);
    } else if (task->type == ABORT) {
        DEBUG("Aborting task %p\n", task->handle);
        goto EXIT;
    } else {
        ERROR("Invalid task type.\n");
    }
    swapcontext(&tdata->return_context, &task->context);

    tdata = get_tdata();

    DEBUG("Returning from task %p with value %p\n", task->handle, tdata->curr_task->arg);
    EXIT:;
    void *out = tdata->curr_task->arg;
    free(tdata->curr_task->stack);
    free(tdata->curr_task);
    tdata->curr_task = NULL;
    return out;
}

void modify_task_count(tpool_pool *pool, int32_t mod) {
    pthread_mutex_lock(&pool->task_count_mutex);
    pool->task_count += mod;
    DEBUG("Task count: %zu\n", pool->task_count);
    if (pool->task_count == 0) {
        pthread_cond_broadcast(&pool->task_count_cond);
    }
    pthread_mutex_unlock(&pool->task_count_mutex);
}

static bool launch_task(tpool_pool *pool) {
    task_t *task = tpool_dequeue(pool->task_queue);

    if (task == NULL) {
        return true;
    }

    tpool_handle *handle = task->handle;
    void *result = run_task(task); // May not return.
    if (handle && handle->status != ABORTING) {
        handle->result = result;

        pthread_mutex_lock(&handle->mutex);
        handle->status = FINISHED;
        pthread_cond_broadcast(&handle->result_cond);
        pthread_mutex_unlock(&handle->mutex);
        DEBUG("Signaled handle %p\n", handle);
    }
    modify_task_count(pool, -1);
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

    getcontext(&tdata->yield_context);
    DEBUG("Passed yield context.\n");
    if (tdata->curr_task != NULL) {
        task_t *task = tdata->curr_task;
        if (task->type == ABORT) {
            free(task->stack);
            free(task);
            modify_task_count(pool, -1);
        } else if (task->type == RESUME) {
            tpool_enqueue(pool->task_queue, tdata->curr_task);
        } else {
            ERROR("Invalid task type.\n");
        }
        tdata->curr_task = NULL;
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

    ASSERT(!pthread_mutex_init(&pool->task_count_mutex, NULL));

    pool->task_queue = tpool_queue_init();

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

void tpool_close(tpool_pool *pool) {
    DEBUG("Closing thread pool.\n");
    size_t size = pool->pool_size;
    
    // queue will return NULL instead of blocking when empty
    pthread_mutex_lock(&pool->task_count_mutex);
    while (pool->task_count > 0) {
        pthread_cond_wait(&pool->task_count_cond, &pool->task_count_mutex);
    }
    tpool_queue_unblock(pool->task_queue);
    pthread_mutex_unlock(&pool->task_count_mutex);

    DEBUG("Waiting for threads to finish.\n");

    for (size_t i = 0; i < size; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    tpool_queue_free(pool->task_queue);
    free(pool);
}

/**
 * @brief Yields execution to the threadpool, enqueueing a resume task so that
 * the threadpool can resume execution of the current task eventually.
 * 
 * Assumes the calling thread is a threadpool thread.
 * 
 * May return in different thread than the caller, but returns exactly once.
 */
void tpool_yield() {
    tdata_t *tdata = get_tdata();
    task_t *task = tdata->curr_task;
    if (task->handle && task->handle->status == ABORTING) {
        task->type = ABORT;
        DEBUG("Aborting task %p.\n", tdata->curr_task->handle);
        setcontext(&tdata->yield_context);
    } else {
        tdata->curr_task->type = RESUME;
        DEBUG("Yielding task %p.\n", tdata->curr_task->handle);
        swapcontext(&tdata->curr_task->context, &tdata->yield_context);
        DEBUG("Resuming %p.\n", get_tdata()->curr_task->handle);
    }
}

void abort_handle(tpool_handle *handle) {
    DEBUG("Aborting handle %p.\n", handle);
    handle->status = ABORTING;
}

void *tpool_task_await(tpool_handle *handle, struct timespec *timeout, void *timeout_val) {
    DEBUG("Awaiting handle %p.\n", handle);
    pthread_mutex_lock(&handle->mutex);
    bool worker = get_tdata() != NULL;
    void *result;
    struct timespec end;
    if (timeout != NULL) {
        struct timespec start;
        clock_gettime(worker ? CLOCK_MONOTONIC : CLOCK_REALTIME, &start);
        end.tv_sec = start.tv_sec + timeout->tv_sec;
        end.tv_nsec = start.tv_nsec + timeout->tv_nsec;
        if (end.tv_nsec >= 1000000000) {
            end.tv_sec++;
            end.tv_nsec -= 1000000000;
        }
    }
    while (handle->status == WAITING) {
        if (worker) {
            if (timeout != NULL) {
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                if (now.tv_sec > end.tv_sec 
                    || (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec)
                ) {
                    abort_handle(handle);
                    result = timeout_val;
                    goto EXIT;
                }
            }
            pthread_mutex_unlock(&handle->mutex);
            tpool_yield(); // Potentially returns in different thread.
            pthread_mutex_lock(&handle->mutex);
        } else if (timeout != NULL) {
            int timed_out = pthread_cond_timedwait(&handle->result_cond, &handle->mutex, &end);
            if (timed_out) {
                struct timespec now;
                clock_gettime(CLOCK_REALTIME, &now);
                abort_handle(handle);
                result = timeout_val;
                goto EXIT;
            }
        } else {
            pthread_cond_wait(&handle->result_cond, &handle->mutex);
        }
    }
    result = handle->result;
    EXIT:
    pthread_mutex_unlock(&handle->mutex);
    DEBUG("Done waiting on handle %p.\n", handle);
    free(handle);

    return result;
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
    
    modify_task_count(pool, 1);

    return (void *) handle > (void *) DISCARD_RESULT ? handle : NULL;
}
