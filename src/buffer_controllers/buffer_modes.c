#include "buffer_controllers/buffer_controllers.h"

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
init_provided_mode(struct io_uring *ring, void *addr, int len, int nr, int bgid) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, NULL);

    io_uring_prep_provide_buffers(sqe, addr, len, nr, bgid, 0);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "Submitting provided buffers initialization failed: %s\n", strerror(-result));
        return -1;
    }
    return 1;
}

int
close_provided_mode(struct io_uring *ring, int nr, int bgid) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, NULL);
    io_uring_prep_remove_buffers(sqe, nr, bgid);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "Submitting provided buffers closing failed: %s\n", strerror(-result));
        return -1;
    }
    return 1;
}

struct io_uring_buf_ring *
init_buf_ring_mode(struct io_uring *ring, void *addr, int len, int nentries, int bgid) {
    struct io_uring_buf_ring *buffer_ring;
    int err;

    buffer_ring = io_uring_setup_buf_ring(ring, (unsigned)nentries, bgid, 0, &err);

    if (!buffer_ring) {
        fprintf(stderr, "io_uring_setup_buf_ring failed: %s\n", strerror(-err));
        return NULL;
    }

    io_uring_buf_ring_init(buffer_ring);

    int mask = io_uring_buf_ring_mask((__u32)nentries);

    for (int i = 0; i < nentries; i++) {
        io_uring_buf_ring_add(buffer_ring, (char *)addr + i * len, (unsigned)len, (unsigned short)i, mask, i);
    }

    io_uring_buf_ring_advance(buffer_ring, nentries);

    return buffer_ring;
}

int
close_buf_ring_mode(struct io_uring *ring, struct io_uring_buf_ring *buffer_ring, int nentries, int bgid) {
    if (!buffer_ring)
        return -1;

    io_uring_free_buf_ring(ring, buffer_ring, (unsigned)nentries, bgid);

    return 1;
}
