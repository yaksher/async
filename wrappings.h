#ifndef WRAPPINGS_H
#define WRAPPINGS_H

#include <dlfcn.h>
#include <stdio.h>

void set_mem_wrapper(void (*pre)(), void (*post)()) {
    static void (*set)(void (*pre)(), void (*post)()) = NULL;
    if (!set) {
        set = dlsym(RTLD_DEFAULT, "set_mem_wrapper");
        char *error = dlerror();
        if (error != NULL) {
            puts(error);
            exit(1);
        }
    }
    set(pre, post);
}

#endif