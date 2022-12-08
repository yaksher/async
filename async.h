#ifndef _TP_ASYNC_H
#define _TP_ASYNC_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "threadpool.h"

typedef tpool_handle async_handle;
typedef void *(*async_work)(void *arg);

void async_init(size_t num_threads);

async_handle *async_run(async_work work, void *arg);

void *async_await(async_handle *handle);

void async_close();

typedef int _async_NOTHING[0];

#define _ASYNC_ARG_STRUCT_8(FUNC, T0, N0, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5, T6, N6, T7, N7, ...)\
typedef struct {\
    T0 N0;\
    T1 N1;\
    T2 N2;\
    T3 N3;\
    T4 N4;\
    T5 N5;\
    T6 N6;\
    T7 N7;\
} _async_##FUNC##_args

#define ASYNC_8(T_RET, FUNC, T0, N0, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5, T6, N6, T7, N7, ...)\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6, T7 N7);\
_ASYNC_ARG_STRUCT_8(FUNC, T0, N0, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5, T6, N6, T7, N7);\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    assert(sizeof(T0) <= sizeof(void *));\
    assert(sizeof(T1) <= sizeof(void *));\
    assert(sizeof(T2) <= sizeof(void *));\
    assert(sizeof(T3) <= sizeof(void *));\
    assert(sizeof(T4) <= sizeof(void *));\
    assert(sizeof(T5) <= sizeof(void *));\
    assert(sizeof(T6) <= sizeof(void *));\
    assert(sizeof(T7) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1, ARGS->N2, ARGS->N3,\
        ARGS->N4, ARGS->N5, ARGS->N6, ARGS->N7);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, ...) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    va_list _async_args;\
    va_start(_async_args, N0);\
    _async_arg->N0 = N0;\
    if (sizeof(T1) > 0) {\
        uintptr_t N1 = va_arg(_async_args, uintptr_t);\
        memcpy(&_async_arg->N1, &N1, sizeof(T1));\
    }\
    if (sizeof(T2) > 0) {\
        uintptr_t N2 = va_arg(_async_args, uintptr_t);\
        memcpy(&_async_arg->N2, &N2, sizeof(T2));\
    }\
    if (sizeof(T3) > 0) {\
        uintptr_t N3 = va_arg(_async_args, uintptr_t);\
        memcpy(&_async_arg->N3, &N3, sizeof(T3));\
    }\
    if (sizeof(T4) > 0) {\
        uintptr_t N4 = va_arg(_async_args, uintptr_t);\
        memcpy(&_async_arg->N4, &N4, sizeof(T4));\
    }\
    if (sizeof(T5) > 0) {\
        uintptr_t N5 = va_arg(_async_args, uintptr_t);\
        memcpy(&_async_arg->N5, &N5, sizeof(T5));\
    }\
    if (sizeof(T6) > 0) {\
        uintptr_t N6 = va_arg(_async_args, uintptr_t);\
        memcpy(&_async_arg->N6, &N6, sizeof(T6));\
    }\
    if (sizeof(T7) > 0) {\
        uintptr_t N7 = va_arg(_async_args, uintptr_t);\
        memcpy(&_async_arg->N7, &N7, sizeof(T7));\
    }\
    va_end(_async_args);\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6, T7 N7)

#define ASYNC(T_RET, FUNC, ARGS...)\
ASYNC_8(T_RET, FUNC, ARGS,\
    _async_NOTHING, _async_PLACEHOLDER_0, _async_NOTHING, _async_PLACEHOLDER_1,\
    _async_NOTHING, _async_PLACEHOLDER_2, _async_NOTHING, _async_PLACEHOLDER_3,\
    _async_NOTHING, _async_PLACEHOLDER_4, _async_NOTHING, _async_PLACEHOLDER_5,\
    _async_NOTHING, _async_PLACEHOLDER_6, _async_NOTHING, _async_PLACEHOLDER_7)

#define ASYNC_NOARGS(T_RET, FUNC)\
T_RET _async_int_##FUNC();\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    (void) arg;\
    T_RET ret = _async_int_##FUNC();\
    return *(void **) &ret;\
}\
async_handle *FUNC() {\
    return async_run(_async_int_vv_##FUNC, NULL);\
}\
T_RET _async_int_##FUNC()

#define AWAIT(T, EXPR...) ((union {T x; void *y;}) {.y = async_await(EXPR)}).x

#endif