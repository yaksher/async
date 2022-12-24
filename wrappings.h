#ifndef WRAPPINGS_H
#define WRAPPINGS_H

#include <dlfcn.h>
#include <stdio.h>

void init_mem_wrapper(void (*pre)(), void (*post)()) {
    static void (*init)(void (*pre)(), void (*post)()) = NULL;
    if (!init) {
        init = dlsym(RTLD_DEFAULT, "init_mem_wrapper");
        char *error = dlerror();
        if (error != NULL) {
            puts(error);
            exit(1);
        }
    }
    init(pre, post);
    // (void) pre;
    // (void) post;
    // puts("Default init.");
    // return;
}

#endif