#ifndef TPOOL__ASYNC_MACROS_H
#define TPOOL__ASYNC_MACROS_H

#define _ASYNC_8(T_RET, FUNC, T0, N0, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5, T6, N6, T7, N7)\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6, T7 N7);\
typedef struct {\
    T0 N0; T1 N1; T2 N2; T3 N3;\
    T4 N4; T5 N5; T6 N6; T7 N7;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1, ARGS->N2, ARGS->N3,\
        ARGS->N4, ARGS->N5, ARGS->N6, ARGS->N7);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6, T7 N7) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0; _async_arg->N1 = N1; _async_arg->N2 = N2; _async_arg->N3 = N3;\
    _async_arg->N4 = N4; _async_arg->N5 = N5; _async_arg->N6 = N6; _async_arg->N7 = N7;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6, T7 N7)

#define _ASYNC_7(T_RET, FUNC, T0, N0, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5, T6, N6)\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6);\
typedef struct {\
    T0 N0; T1 N1; T2 N2; T3 N3;\
    T4 N4; T5 N5; T6 N6;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1, ARGS->N2, ARGS->N3,\
        ARGS->N4, ARGS->N5, ARGS->N6);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0; _async_arg->N1 = N1; _async_arg->N2 = N2; _async_arg->N3 = N3;\
    _async_arg->N4 = N4; _async_arg->N5 = N5; _async_arg->N6 = N6;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5, T6 N6)

#define _ASYNC_6(T_RET, FUNC, T0, N0, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5)\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5);\
typedef struct {\
    T0 N0; T1 N1; T2 N2; T3 N3;\
    T4 N4; T5 N5;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1, ARGS->N2, ARGS->N3,\
        ARGS->N4, ARGS->N5);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0; _async_arg->N1 = N1; _async_arg->N2 = N2; _async_arg->N3 = N3;\
    _async_arg->N4 = N4; _async_arg->N5 = N5;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4, T5 N5)

#define _ASYNC_5(T_RET, FUNC, T0, N0, T1, N1, T2, N2, T3, N3, T4, N4)\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4);\
typedef struct {\
    T0 N0; T1 N1; T2 N2; T3 N3;\
    T4 N4;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1, ARGS->N2, ARGS->N3,\
        ARGS->N4);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0; _async_arg->N1 = N1; _async_arg->N2 = N2; _async_arg->N3 = N3;\
    _async_arg->N4 = N4;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3, T4 N4)

#define _ASYNC_4(T_RET, FUNC, T0, N0, T1, N1, T2, N2, T3, N3)\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3);\
typedef struct {\
    T0 N0; T1 N1; T2 N2; T3 N3;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1, ARGS->N2, ARGS->N3);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, T1 N1, T2 N2, T3 N3) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0; _async_arg->N1 = N1; _async_arg->N2 = N2; _async_arg->N3 = N3;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2, T3 N3)

#define _ASYNC_3(T_RET, FUNC, T0, N0, T1, N1, T2, N2)\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2);\
typedef struct {\
    T0 N0; T1 N1; T2 N2;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1, ARGS->N2);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, T1 N1, T2 N2) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0; _async_arg->N1 = N1; _async_arg->N2 = N2;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1, T2 N2)

#define _ASYNC_2(T_RET, FUNC, T0, N0, T1, N1)\
T_RET _async_int_##FUNC(T0 N0, T1 N1);\
typedef struct {\
    T0 N0; T1 N1;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(\
        ARGS->N0, ARGS->N1);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0, T1 N1) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0; _async_arg->N1 = N1;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0, T1 N1)

#define _ASYNC_1(T_RET, FUNC, T0, N0)\
T_RET _async_int_##FUNC(T0 N0);\
typedef struct {\
    T0 N0;\
} _async_##FUNC##_args;\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    _async_##FUNC##_args *ARGS = arg;\
    T_RET ret = _async_int_##FUNC(ARGS->N0);\
    free(arg);\
    return ((union {T_RET x; void *y;}) {.x = ret}).y;\
}\
async_handle *FUNC(T0 N0) {\
    _async_##FUNC##_args *_async_arg = malloc(sizeof(_async_##FUNC##_args));\
    _async_arg->N0 = N0;\
    return async_run(_async_int_vv_##FUNC, _async_arg);\
}\
T_RET _async_int_##FUNC(T0 N0)


#define _ASYNC_0(T_RET, FUNC)\
T_RET _async_int_##FUNC();\
void *_async_int_vv_##FUNC(void *arg) {\
    assert(sizeof(T_RET) <= sizeof(void *));\
    (void) arg;\
    return ((union {T_RET x; void *y;}) {.x = _async_int_##FUNC()}).y;;\
}\
async_handle *FUNC() {\
    return async_run(_async_int_vv_##FUNC, NULL);\
}\
T_RET _async_int_##FUNC()

#define INVALID_ARG_COUNT(...) typedef int _ASYNC_INVALID_ARG_COUNT[-1]; void _ASYNC_INVALID_ARG_COUNT_FUNC()

#define GET_17TH_ARG(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, ...) A16

#define _ASYNC_DISPATH(ARGS...)\
    GET_17TH_ARG(ARGS\
        INVALID_ARG_COUNT, _ASYNC_8, INVALID_ARG_COUNT, _ASYNC_7, INVALID_ARG_COUNT, _ASYNC_6,\
        INVALID_ARG_COUNT, _ASYNC_5, INVALID_ARG_COUNT, _ASYNC_4, INVALID_ARG_COUNT, _ASYNC_3,\
        INVALID_ARG_COUNT, _ASYNC_2, INVALID_ARG_COUNT, _ASYNC_1, _ASYNC_0)

#define _impl_ASYNC(T_RET, FUNC, ARGS...) _ASYNC_DISPATH(ARGS)(T_RET, FUNC, ##ARGS)

#define _impl_AWAIT(T, EXPR...) ((union {T x; void *y;}) {.y = async_await(EXPR)}).x

#define _impl_YIELD tpool_yield

#define _impl_YIELD_UNTIL(EXPR...) do {\
    while (!(EXPR)) {\
        _impl_YIELD();\
    }\
} while (0)

#define _impl_YIELD_WHILE(EXPR...) do {\
    while (EXPR) {\
        _impl_YIELD();\
    }\
} while (0)

#define _impl_ATOMIC_START async_atomic_start
#define _impl_ATOMIC_END async_atomic_end
#define _impl_ATOMIC for (\
    bool _async_int_ATOMIC = _impl_ATOMIC_START();\
    !_async_int_ATOMIC;\
    _async_int_ATOMIC = _impl_ATOMIC_END()\
    )
#define _impl_ATOMIC_BREAK continue
#define _impl_ATOMIC_RETURN _impl_ATOMIC_END(); return

#endif