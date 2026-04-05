#include "sqe.h"

struct io_uring_sqe* create_sqe(struct io_uring *ring) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return NULL;
    }
    return sqe;
}
