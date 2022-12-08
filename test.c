#include "async.h"

#include "stdio.h"

ASYNC(intptr_t, square, intptr_t, arg) {
    return arg * arg;
}

ASYNC(intptr_t, fibonacci, intptr_t, n) {
    if (n <= 1) {
        return n;
    }
    return AWAIT(intptr_t, fibonacci(n - 1)) + AWAIT(intptr_t, fibonacci(n - 2));
}

ASYNC(void *, fake_malloc) {
    return malloc(100);
}

int main() {
    async_init(0);
    printf("%ld\n", AWAIT(intptr_t, fibonacci(10)));
    printf("%p\n", AWAIT(void *, fake_malloc()));
    async_close();
    return 0;
}