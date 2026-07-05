#include "buffer_modes.h"

// TODO: Updating buffers in next versions

int
init_fixed_mode(struct io_uring *ring, struct iovec *iovecs, unsigned nr_iovecs) {
    // TODO: tags in next version
    int result = io_uring_register_buffers(ring, iovecs, nr_iovecs);
    if (result < 0) {
        fprintf(stderr, "io_uring_register_buffers failed: %s\n", strerror(-result));
        return -1;
    }
    return 1;
}

int
close_fixed_mode(struct io_uring *ring) {
    int result = io_uring_unregister_buffers(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_unregister_buffers failed: %s\n", strerror(-result));
        return -1;
    }
    return 1;
}

int
init_provided_mode(struct io_uring *ring, struct io_uring_sqe *sqe, void *addr, int len, int nr, int bgid, int bid) {
    io_uring_prep_provide_buffers(sqe, addr, len, nr, bgid, bid);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit(provide_buffers) failed: %s\n", strerror(-result));
        return -1;
    }
    return 1;
}

int
close_provided_mode(struct io_uring *ring, struct io_uring_sqe *sqe, int nr, int bgid) {
    io_uring_prep_remove_buffers(sqe, nr, bgid);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit(remove_buffers) failed: %s\n", strerror(-result));
        return -1;
    }
    return 1;
}

int
init_buf_ring_mode(struct io_uring *ring, struct io_uring_buf_reg *reg, unsigned int flags) {
    // Maybe create io_uring_buf_reg here?
    // int result = io_uring_register_buf_ring(ring, reg, flags);

    // if (result < 0) {
    //     fprintf(stderr, "io_uring_register_buf_ring failed: %s\n", strerror(-result));
    //     return -1;
    // }
    // return 1;
}

int
close_buf_ring_mode(struct io_uring *ring, struct io_uring_buf_reg *reg) {
    // int result = io_uring_unregister_buf_ring(ring, reg);

    // if (result < 0) {
    //     fprintf(stderr, "io_uring_unregister_buf_ring failed: %s\n", strerror(-result));
    //     return -1;
    // }
    // return 1;
}
