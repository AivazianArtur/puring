#include "loop.h"


void fast_shutdown(struct io_uring* ring, RequestRegistry *reg) 
{
    ring_destroy(ring);
    registry_destroy(reg);
}


void graceful_shutdown(struct io_uring* ring, RequestRegistry *reg)
{
    ring_destroy(ring);
    registry_destroy(reg);

    struct io_uring_cqe *cqe;  // TEMP
    struct __kernel_timespec ts;
    ts.tv_nsec = 0;
    ts.tv_sec = 3;

    while (io_uring_wait_cqe_timeout(ring, &cqe, &ts) == 0) {
        int index = (int)(uintptr_t)cqe->user_data;
        registry_remove(reg, index);  // TODO: set future exception
        io_uring_cqe_seen(ring, cqe);
    }
}
