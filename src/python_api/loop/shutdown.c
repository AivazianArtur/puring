#include "loop.h"

void
fast_shutdown(struct io_uring *ring, RequestRegistry *reg) {
    ring_destroy(ring);
    registry_destroy(reg);
}

void
graceful_shutdown(struct io_uring *ring, RequestRegistry *reg) {
    struct io_uring_cqe *cqe;
    struct __kernel_timespec ts;

    ts.tv_sec = 0;
    ts.tv_nsec = 100 * 1000 * 1000; // 100ms

    while (io_uring_wait_cqe_timeout(ring, &cqe, &ts) == 0) {
        if (cqe->user_data == WAKEUP_FD_TAG) {
            io_uring_cqe_seen(ring, cqe);
            continue;
        }
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(reg, index);
        if (!slot) {
            io_uring_cqe_seen(ring, cqe);
            continue;
        }
        PyObject *future = slot->future;
        if (future) {
            PyObject *exc = PyObject_CallFunction(PyExc_RuntimeError, "s", "Event loop is closed");

            if (exc) {
                PyObject_CallMethod(future, "set_exception", "O", exc);
                Py_DECREF(exc);
            }
        }
        registry_remove(reg, index);
        io_uring_cqe_seen(ring, cqe);
    }
    registry_destroy(reg);
    PyObject *token = ContextVar_set(Py_None);
    if (token) {
        Py_DECREF(token);
    } else {
        PyErr_Clear();
    }
    ring_destroy(ring);
}
