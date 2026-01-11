#include "liburing.h"
#include "handler.h"

int puring_init(
    struct io_uring *ring,
    ring_initialization_params *params,
    memory_params *memory_params,
) {
    /**
        TODO: Write description
    */

    struct io_uring ring;

    if (memory_params) {
        // TODO
        if (!params) {
            params = {0}
        }
        return io_uring_queue_init_mem(),
    }
    else if (params) {
        // TODO
        return io_uring_queue_init_params(8, ring, params);
    }
    else {
        return io_uring_queue_init(0, ring, 0);
    }
}

void puring_exit(struct io_uring *ring) {
    io_uring_queue_exit(ring);
}