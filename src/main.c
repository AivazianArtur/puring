#include <stdio.h>
#include "puring.h"

int main(void) {
    struct io_uring ring;

    int ret = puring_init(&ring);
    if (ret < 0) {
        perror("puring_init failed");
        return 1;
    }

    printf("io_uring initialized\n");

    puring_exit(&ring);
    return 0;
}
