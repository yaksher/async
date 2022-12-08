#include "async.h"

#include "stdio.h"

ASYNC(intptr_t, square, intptr_t, arg) {
    return arg * arg;
}

ASYNC(intptr_t, fibonacci, intptr_t, n) {
    if (n <= 1) {
        return n;
    }
    return (intptr_t) AWAIT(fibonacci(n - 1)) + (intptr_t) AWAIT(fibonacci(n - 2));
}

int main() {
    async_init(0);
    printf("%ld\n", (intptr_t) AWAIT(fibonacci(10)));
    async_close();
    return 0;
}