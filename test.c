#include <stdio.h>
#include <unistd.h>

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

async(int *, malloc_int, int, n) {
    int *ptr = malloc(sizeof(int));
    *ptr = n;
    return ptr;
}

async(int, a_sleep, double, t) {
    async_sleep(t);
    return 1;
}

async(int, sleep_5) {
    sleep(5);
    return 1;
}

int main() {
    async_init(0);
    printf("%ld\n", await(intptr_t, prod(10, 20)));
    printf("%ld\n", await(intptr_t, fibonacci(20)));
    printf("%d\n", *await(int *, malloc_int(20)));
    // printf("%d\n", await(int, sleep_5(), 0.1, 3));
    // printf("%d\n", await(int, a_sleep(1), 0.1, 4));
    // printf("%d\n", await(int, a_sleep(2), 0.1, 5));
    async_close();
    return 0;
}