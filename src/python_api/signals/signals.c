#include "signals.h"


bool is_future_canceled(
    RequestSlot *slot,
    RequestRegistry *registry,
    struct io_uring ring,
    struct io_uring_cqe *cqe,
    int index
)
{
    if (PyObject_CallMethod(slot->future, "cancelled", NULL) == Py_False) {
        return false;
    }

    io_uring_cqe_seen(&ring, cqe);
    registry_remove(registry, index);
    return true;
}
