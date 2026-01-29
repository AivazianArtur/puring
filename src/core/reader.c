#include "core.h"

/* Principle of work of this: */
// int uring_fd = io_uring_get_ring_fd(&ring);
// loop.add_reader(uring_fd, _on_uring_ready)

// So basically its a reader
static void on_uring_ready(UringLoop *self)
{
    struct io_uring_cqe *cqe;

    while (io_uring_peek_cqe(&self->ring, &cqe) == 0) {
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(self->registry, index);

        if (cqe->res >= 0) {
            PyObject_CallMethod(
                slot->future,
                "set_result",
                "O",
                slot->buffer
            );
        } else {
            PyObject_CallMethod(
                slot->future,
                "set_exception",
                "O",
                PyErr_NewException("TODO: PROPAGATE ERRORS HERE")
            );
        }

        registry_remove(self->registry, index);
        io_uring_cqe_seen(&self->ring, cqe);
    }
}
