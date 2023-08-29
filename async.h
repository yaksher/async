#ifndef _TP_ASYNC_H
#define _TP_ASYNC_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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

/**
 * @brief Atomic macros are used to create atomic sections in tasks.
 * 
 * Note that preemption is not currently actually implemented, making these
 * serve no actual purpose.
 *
 * Atomic sections are guaranteed to be executed without interruption, which
 * means that they do not need to be signal safe.
 *
 * All code not inside an atomic block must be signal safe.
 *
 * Usage is
 *
 * `atomic {
 *   // atomic code
 * }`
 *
 * or
 *
 * `atomic_start();
 *
 * // atomic code
 *
 * atomic_end();`
 *
 * If using the `atomic {}` block, note that using `goto`,
 * `break`, or `continue` to move from inside the block outisde it,
 * as well as returning or calling a function which does not return,
 * will result in unspecified behavior.
 *
 * Atomic sections may be nested (though this is equivalent to just the outer
 * atomic section)
 *
 * Calling `atomic_end()` more times than `atomic_start()` will result in unspecified
 * behavior.
 *
 * The `atomic_break` and `atomic_return` macros will jump to the end of the atomic block
 * and safely return from the function, respectively.
 *
 * Note that `atomic_return` is only valid when inside *exactly* one atomic block in
 * the current function.
 *
 * The `yield` macros above, as well as `await` will still yield control of the
 * thread to other tasks. The `atomic` mode only prevents signal interrupts.
 */
#define atomic _impl_ATOMIC
#define atomic_start _impl_ATOMIC_START
#define atomic_end _impl_ATOMIC_END
#define atomic_break _impl_ATOMIC_BREAK
#define atomic_return _impl_ATOMIC_RETURN

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