#ifndef WRAPPINGS_H
#define WRAPPINGS_H

#include <dlfcn.h>

void init_mem_wrapper(void (*pre)(), void (*post)()) {
    void *handle = dlopen("wrap_malloc.so", RTLD_LAZY);
    static void (*init)(void (*pre)(), void (*post)()) = NULL;
    if (!init) {
        init = dlsym(handle, "init_mem_wrapper");
    }
    init(pre, post);
    // (void) pre;
    // (void) post;
    // puts("Default init.");
    // return;
}

#endif