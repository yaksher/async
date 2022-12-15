#include "async.h"

tpool_pool *pool = NULL;
pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;

void async_init(size_t num_threads) {
    pthread_mutex_lock(&pool_mutex);
    if (pool == NULL) {
        pool = tpool_init(num_threads);
    }
    pthread_mutex_unlock(&pool_mutex);
}

async_handle *async_run(async_work fn, void *arg) {
    return (async_handle *) tpool_task_enqueue(pool, fn, arg, HANDLE);
}

void *async_await(async_handle *handle, struct timespec *timeout, void *timeout_val) {
    return tpool_task_await((tpool_handle *) handle, timeout, timeout_val);
}

void async_close() {
    pthread_mutex_lock(&pool_mutex);
    if (pool != NULL) {
        tpool_close(pool, false);
        pool = NULL;
    }
    pthread_mutex_unlock(&pool_mutex);
}