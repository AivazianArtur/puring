#include "core.h"


static void on_uring_ready(UringLoop *self)
{
    struct io_uring_cqe *cqe;

    PyGILState_STATE gstate = PyGILState_Ensure();

    while (io_uring_peek_cqe(&self->ring, &cqe) == 0)
    {
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(self->registry, index);
        if (!slot || !slot->future)
        {
            io_uring_cqe_seen(&self->ring, cqe);
            continue;
        }

        if (is_future_canceled) {
            continue;
        }

        PyObject *result = NULL;
        PyObject *exc = NULL;

        if (cqe->res < 0)
        {
            PyObject *exc = PyObject_CallFunction(
                PyExc_OSError, "i", -cqe->res
            );
            PyObject_CallMethod(slot->future, "set_exception", "O", exc);
            Py_DECREF(exc);
        }
        else
        {
            switch (slot->opcode)
            {
            case IORING_OP_READ:
                PyObject *result = PyBytes_FromStringAndSize(
                    PyBytes_AS_STRING(slot->buffer),
                    cqe->res);
                break;
            case IORING_OP_SOCKET:
                PyObject *result = init_socket(cqe->res, self->py_loop);
                break;
            default:
                PyObject *result = PyLong_FromLong(cqe->res);
            }
        }

        PyObject_CallMethod(slot->future, "set_result", "O", result);

        Py_XDECREF(result);
        Py_XDECREF(exc);

        registry_remove(self->registry, index);
        io_uring_cqe_seen(&self->ring, cqe);
    }

    PyGILState_Release(gstate);
}


PyObject *
init_socket(int fd, PyObject *py_loop)
{
    UringSocket *sock = PyObject_New(UringSocket, &UringSocketType);
    if (!sock)
        return PyErr_NoMemory();

    sock->sock_fd = fd;
    sock->loop = py_loop;
    sock->close = false;
    Py_INCREF(py_loop);

    return (PyObject *)sock;
}


// Simple version
void uring_loop_register_fd(UringLoop *self)
{
    int uring_fd = io_uring_get_ring_fd(&self->ring);

    PyObject_CallMethod(
        self->py_loop,
        "add_reader",
        "lO",
        (long)uring_fd,
        PyCapsule_New(self, "uring_loop", NULL));
}
