#ifndef _TP_ASYNC_H
#define _TP_ASYNC_H

#include <stdlib.h>
#include <assert.h>

#include "threadpool.h"
#include "async_macros.h"

typedef tpool_handle async_handle;
typedef void *(*async_work)(void *arg);

/**
 * @brief The async macro is used to define an asynchronous function.
 * 
 * Usage is 
 * `async(return_type, function_name, [arg_type1, arg_name1], ...) {
 *    // function body
 * }
 * 
 * The created function will return a handle to the asynchronous task.
 * 
 * Supports 0 to 8 arguments.
 * Asserts that sizeof(return_type) <= sizeof(void *).
 * For larger returns, heap-allocate memory and return a pointer.
 */
#define async(T, FUNC, ARGS...) _impl_ASYNC(T, FUNC, ##ARGS)

/**
 * @brief The await macro is used to wait for the result of an asynchronous task
 * created by a function defined with the `async` macro.
 * 
 * Usage is `await(return_type, handle, [timeout, default])`.
 * 
 * Omitting the timeout argument or setting it to 0 will wait indefinitely,
 * otherwise should be a `timespec` or a `double` representing seconds.
 * 
 * If timeout is specified, the default value must also be specified.
 * 
 * Guarantees that it waits at least as long as the timeout, but can be more.
 * 
 * For handles created directly by `async_run` rather than `async` functions,
 * behavior is undefined if `return_type` is not `void *`.
 * 
 * Evaluates to the result of the asynchronous task, yielding execution until
 * the task is complete.
 * 
 * If timeout is specified, returns a wrapper struct with a `bool success` indicating
 * whether the task completed before the timeout and a `return_type val` field.
 * 
 * If timeout is not specified, returns the result of the task.
 */
#define await(T, HANDLE, TIMEOUT_DEFAULT...) _impl_AWAIT(T, HANDLE, ##TIMEOUT_DEFAULT)


/**
 * @brief The yield macro is used to yield execution to another task.
 * 
 * Usage is `yield()` or `yield_while(condition)`.
 * 
 * `yield_until(condition)` is equivalent to `yield_while(!(condition))`.
 * 
 * Should only be used inside of asynchronous functions and will result in
 * unspecified behavior if used outside of an asynchronous function.
 * 
 * Used to define asynchronous primitives by doing things like
 * yielding until non-blocking I/O is complete.
 * 
 * In yield_until or yield_while, the condition will expanded once but may be
 * evaluated multiple times.
 */
#define yield() _impl_YIELD()
#define yield_until(COND) _impl_YIELD_UNTIL(COND)
#define yield_while(COND) _impl_YIELD_WHILE(COND)

/**
 * @brief Sleeps for the specified time. While sleeping, yields execution to
 * other tasks. The sleep will never be shorter than the specified time, but may
 * exceed it.
 * 
 * Usage is `async_sleep(time)`.
 * 
 * Time can be either a double representing seconds or a timespec.
 * 
 * Should only be used inside of asynchronous functions and will result in
 * unspecified behavior if used outside of an asynchronous function.
 */
#define async_sleep(TIME) _impl_ASYNC_SLEEP(TIME)

/**
 * @brief Initializes the async library. Calls made before the next call to
 * async_close will do nothing. Calls made while async_close is running will
 * block until async_close returns, then run.
 * 
 * @param num_threads The number of threads to use. 0 for default.
 */
void async_init(size_t num_threads);

/**
 * @brief Runs a non-async `void *` to `void *` function asynchronously
 * 
 * @param work The function to run.
 * @param arg The argument to pass to the function.
 * @return async_handle* A handle to the asynchronous task.
 */
async_handle *async_run(async_work work, void *arg);

/**
 * @brief Waits for the result of an asynchronous task.
 * 
 * The await macro is preferred for functions defined with the async macro.
 * 
 * @param handle The handle to the asynchronous task.
 * @param timeout The timeout to wait for the task to complete.
 * @param timeout_val The value to return if the timeout is reached.
 * @return void* The result of the asynchronous task or default if timeout
 */
void *async_await(async_handle *handle, struct timespec *timeout, void *timeout_val);

/**
 * @brief Wrapper for async_await which takes timeout as a double representing
 * seconds instead of as a timespec.
 */
void *async_await_double(async_handle *handle, double timeout, void *timeout_val) {
    assert(timeout >= 0.0);
    struct timespec ts;
    ts.tv_sec = (time_t) timeout;
    ts.tv_nsec = (long) ((timeout - ts.tv_sec) * 1e9);
    return async_await(handle, &ts, timeout_val);
}

/**
 * @brief Closes the global threadpool.
 * 
 * Waits until all workers are waiting for tasks, then exits.
 */
void async_close();

#endif