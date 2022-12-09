#ifndef _TP_ASYNC_H
#define _TP_ASYNC_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "threadpool.h"
#include "async_macros.h"

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
#define async(T, FUNC, ARGS...) _impl_ASYNC(T, FUNC, ARGS)

/**
 * @brief The await macro is used to wait for the result of an asynchronous task
 * created by a function defined with the `async` macro.
 * 
 * Usage is `await(return_type, handle)`.
 * 
 * For handles created directly by `async_run` rather than `async` functions,
 * behavior is undefined if `return_type` is not `void *`.
 * 
 * Evaluates to the result of the asynchronous task, yielding execution until
 * the task is complete.
 */
#define await(T, HANDLE...) _impl_AWAIT(T, HANDLE)


/**
 * @brief The yield macro is used to yield execution to another task.
 * 
 * Usage is `yield()` or `yield_while(condition)`.
 * 
 * `yield_until(condition)` is equivalent to `yield_while(!(condition))`.
 * 
 * `do_yield_while(condition)` will yield once and then until the condition is
 * false.
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
#define yield_until(COND...) _impl_YIELD_UNTIL(COND)
#define yield_while(COND...) _impl_YIELD_WHILE(COND)
#define do_yield_while(COND...) _impl_DO_YIELD_WHILE(COND)

typedef tpool_handle async_handle;
typedef void *(*async_work)(void *arg);

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
 * @return void* The result of the asynchronous task.
 */
void *async_await(async_handle *handle);

/**
 * @brief Closes the global threadpool.
 * 
 * Waits until all workers are waiting for tasks, then exits.
 */
void async_close();

#endif