#include "core.h"


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

        PyObject *result = NULL;
        PyObject *exc = NULL;

        if (cqe->res < 0) {
            exc = PyObject_CallFunction(PyExc_OSError, "i", -cqe->res);
        } else {
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
        }

        PyObject_CallMethod(
            self->py_loop,
            "call_soon_threadsafe",
            "OOOO",
            self->resolve_future_cb,
            slot->future,
            result ? result : Py_None,
            exc ? exc : Py_None
        );

        Py_XDECREF(result);
        Py_XDECREF(exc);

        registry_remove(self->registry, index);
        io_uring_cqe_seen(&self->ring, cqe);
    }

    PyGILState_Release(gstate);
}


PyObject* 
init_socket(int fd, PyObject *py_loop)
{
    UringSocket *sock = PyObject_New(UringSocket, &UringSocketType);
    if (!sock) return PyErr_NoMemory();

    sock->sock_fd = fd;
    sock->loop = py_loop;
    sock->close = false;
    Py_INCREF(py_loop);

    return (PyObject*)sock;
}


static PyObject*
_resolve_future(PyObject *self, PyObject *args)
{
    PyObject *fut;
    PyObject *result;
    PyObject *exc;

    if (!PyArg_ParseTuple(args, "OOO", &fut, &result, &exc))
        return NULL;

    if (exc != Py_None) {
        PyObject_CallMethod(fut, "set_exception", "O", exc);
    } else {
        PyObject_CallMethod(fut, "set_result", "O", result);
    }

    Py_RETURN_NONE;
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
        PyCapsule_New(self, "uring_loop", NULL)
    );
}
