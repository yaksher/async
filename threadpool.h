#include <pthread.h>

#include "queue.h"

typedef struct tpool_handle tpool_handle;
typedef struct tpool_pool tpool_pool;
typedef void *(*tpool_work)(void *);

/**
 * Initializes the pool size threads. 0 for default.
 * Returns NULL on failure.
 */
tpool_pool *tpool_init(size_t size);

/**
 * Closes pool by joining threads and freeing all allocated memory.
 * Waits until all workers are waiting for tasks, then exits.
 * May never return if tasks add other tasks.
 * Behavior is unspecified if tasks are added while this is running.
 * If "get_results" is true, mallocs and returns an array with the contents of the results
 * queue
 */
void tpool_close(tpool_pool *pool);

/**
 * @brief Yields execution to the threadpool, enqueueing a resume task so that
 * the threadpool can resume execution of the current task eventually.
 * 
 * Assumes the calling thread is a threadpool thread.
 * 
 * May return in different thread than the caller, but returns exactly once.
 */
void tpool_yield();

/**
 * Gets the result of a future.
 */
void *tpool_task_await(tpool_handle *handle);

/**
 * Enqueues a task with a task handle which can awaited.
 */
tpool_handle *tpool_task_enqueue(tpool_pool *pool, tpool_work work, void *arg);