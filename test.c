#include "async.h"

#include "stdio.h"

async(intptr_t, square, intptr_t, arg) {
    return arg * arg;
}

async(intptr_t, fibonacci, intptr_t, n) {
    if (n <= 1) {
        return n;
    }
    return await(intptr_t, fibonacci(n - 1)) + await(intptr_t, fibonacci(n - 2));
}

async(void *, fake_malloc) {
    return malloc(100);
}

int main() {
    async_init(0);
    printf("%ld\n", await(intptr_t, fibonacci(10)));
    printf("%p\n", await(void *, fake_malloc()));
    async_close();
    return 0;
}