#include "async.h"

#include "stdio.h"

ASYNC(intptr_t, square, intptr_t, arg) {
    return arg * arg;
}

int main() {
    async_init(0);
    async_handle *handle = square(5);
    printf("%d\n", (intptr_t) async_await(handle));
    async_close();
    return 0;
}