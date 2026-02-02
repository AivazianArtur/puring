#include "core.h"

/* Principle of work of this: */
// int uring_fd = io_uring_get_ring_fd(&ring);
// loop.add_reader(uring_fd, _on_uring_ready)

// So basically its a reader
static void on_uring_ready(UringLoop *self)
{
    struct io_uring_cqe *cqe;

    PyGILState_STATE gstate = PyGILState_Ensure();

    while (io_uring_peek_cqe(&self->ring, &cqe) == 0) {
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(self->registry, index);
        if (!slot || !slot->future) {
            io_uring_cqe_seen(&self->ring, cqe);
            continue;
        }

        if (cqe->res < 0) {
            PyObject *exc = PyObject_CallFunction(
                PyExc_OSError,
                "i",
                -cqe->res
            );
            PyObject_CallMethod(slot->future, "set_exception", "O", exc);
            Py_DECREF(exc);
        } else {
            PyObject *result;

            switch (slot->opcode) {
                case IORING_OP_READ:
                    result = PyBytes_FromStringAndSize(
                        PyBytes_AS_STRING(slot->buffer),
                        cqe->res
                    );
                    break;

                case IORING_OP_SOCKET:
                    result = init_socket(cqe->res, self->py_loop);
                    break;
                default:
                    result = PyLong_FromLong(cqe->res);
            }

            PyObject_CallMethod(slot->future, "set_result", "O", result);
            Py_DECREF(result);
        }
        registry_remove(self->registry, index);
        io_uring_cqe_seen(&self->ring, cqe);
    }
    PyGILState_STATE gstate = PyGILState_Ensure();
}


UringSocket* init_socket(int res, PyObject *py_loop) {
    UringSocket *socket = malloc(sizeof(UringSocket));
    if (!socket) return 0;
    socket->sock_fd = res;
    socket->loop = py_loop;
    socket->close = false;
    return socket;
}