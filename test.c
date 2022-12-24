#include <stdio.h>

#include "async.h"

async(intptr_t, prod, intptr_t, n1, intptr_t, n2) {
    return n1 * n2;
}

async(intptr_t, fibonacci, intptr_t, n) {
    if (n <= 1) {
        return n;
    }
    async_handle *h1 = fibonacci(n - 1);
    async_handle *h2 = fibonacci(n - 2);
    return await(intptr_t, h1) + await(intptr_t, h2);
}

async(void *, malloc_100) {
    void *p;
    atomic {
        p = malloc(100);
    }
    return p;
}

int main() {
    async_init(0);
    printf("%ld\n", await(intptr_t, prod(10, 20)));
    printf("%ld\n", await(intptr_t, fibonacci(20)));
    printf("%p\n", await(void *, malloc_100()));
    async_close();
    return 0;
}