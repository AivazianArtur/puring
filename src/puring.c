#include "liburing.h"

int puring_init(struct io_uring *ring) {
    return io_uring_queue_init(8, ring, 0);
}

void puring_exit(struct io_uring *ring) {
    io_uring_queue_exit(ring);
}
