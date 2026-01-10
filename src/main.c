#include <stdio.h>
#include "puring.h"

int main(void) {
    // struct io_uring_params params;
    // puring_init((struct io_uring_params){ .draft = 25, .another_draft = 28 });


    int ret = puring_init(&ring);
    if (ret < 0) {
        perror("puring_init failed");
        return 1;
    }

    printf("io_uring initialized\n");

    puring_exit(&ring);
    return 0;
}
