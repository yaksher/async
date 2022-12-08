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

#include "async_macros.h"

#endif