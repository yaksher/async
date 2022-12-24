#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>

#include <stdio.h>

void noop() {}

void (*pre_mem_func)() = noop;
void (*post_mem_func)() = noop;

void set_mem_wrapper(void (*pre)(), void (*post)()) {
    pre_mem_func = pre;
    post_mem_func = post;
}

void *malloc(size_t size) {
    static void *(*real_malloc)(size_t) = NULL;
    if (real_malloc == NULL) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
    }
    pre_mem_func();
    void *ret = real_malloc(size);
    post_mem_func();
    return ret;
}

void *calloc(size_t nmemb, size_t size) {
    static void *(*real_calloc)(size_t, size_t) = NULL;
    if (real_calloc == NULL) {
        real_calloc = dlsym(RTLD_NEXT, "calloc");
    }
    pre_mem_func();
    void *ret = real_calloc(nmemb, size);
    post_mem_func();
    return ret;
}

void *realloc(void *ptr, size_t size) {
    static void *(*real_realloc)(void *, size_t) = NULL;
    if (real_realloc == NULL) {
        real_realloc = dlsym(RTLD_NEXT, "realloc");
    }
    pre_mem_func();
    void *ret = real_realloc(ptr, size);
    post_mem_func();
    return ret;
}

void free(void *ptr) {
    static void (*real_free)(void *) = NULL;
    if (real_free == NULL) {
        real_free = dlsym(RTLD_NEXT, "free");
    }
    pre_mem_func();
    real_free(ptr);
    post_mem_func();
}