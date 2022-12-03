#include <pthread.h>

#include "queue.h"

typedef enum {WAITING, FINISHED} tpool_task_status;
typedef enum {HANDLE, QUEUE_RESULT, DISCARD_RESULT} tpool_option;

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
tpool_list *tpool_close(tpool_pool *pool, bool get_results);

/**
 * Runs a function on every element of a list, returns a list of results.
 * Does not mutate input list
 */
tpool_list *tpool_map(tpool_pool *map, tpool_list *list, tpool_work func);

/**
 * Gets the result of a future.
 */
void *tpool_task_await(tpool_handle *handle);

/**
 * Enqueues a task with a task handle which can awaited.
 */
tpool_handle *tpool_task_enqueue(tpool_pool *pool, tpool_work work, void *arg, tpool_option option);

/**
 * Gets the next result from the anonymous result queue. Blocks until result exists.
 */
void *tpool_result_dequeue(tpool_pool *pool);