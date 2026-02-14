#include "reader.h"

void on_uring_ready(UringLoop *self)
{
    struct io_uring_cqe *cqe;

    PyGILState_STATE gstate = PyGILState_Ensure();

    while (io_uring_peek_cqe(&(self->ring), &cqe) == 0)
    {
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(self->registry, index);
        if (!slot || !slot->future)
        {
            io_uring_cqe_seen(&(self->ring), cqe);
            continue;
        }

        if (is_future_canceled) {
            continue;
        }

        PyObject *result = NULL;
        PyObject *exc = NULL;

        if (cqe->res < 0)
        {
            PyObject *exc;
            exc = PyObject_CallFunction(
                PyExc_OSError, "i", -cqe->res
            );
            PyObject_CallMethod(slot->future, "set_exception", "O", exc);
            Py_DECREF(exc);
        }
        else
        {
            PyObject *result;
            switch (slot->opcode)
            {
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
    if (!sock) {
        return PyErr_NoMemory();
    }

    sock->sock_fd = fd;
    sock->loop = py_loop;
    sock->closed = false;
    Py_INCREF(py_loop);

    return (PyObject *)sock;
}

// Register reader
PyObject *py_on_uring_ready(PyObject *self, PyObject *args)
{
    PyObject *capsule;
    if (!PyArg_ParseTuple(args, "O", &capsule)) return NULL;

    UringLoop *loop = PyCapsule_GetPointer(capsule, "uring_loop");
    if (!loop) return NULL;

    on_uring_ready(loop);
    Py_RETURN_NONE;
}

static PyMethodDef py_uring_reader_cb_def = {
    "on_uring_ready", (PyCFunction)py_on_uring_ready, METH_VARARGS, "Called by asyncio when io_uring FD is ready"
};

// Simple version
void uring_loop_register_fd(UringLoop *loop)
{
    int uring_fd = loop->ring->ring_fd;
    PyObject *capsule = PyCapsule_New(loop, "uring_loop", NULL);

    PyObject *callback = PyCFunction_New(&py_uring_reader_cb_def, capsule);

    PyObject_CallMethod(
        loop->py_loop,
        "add_reader",
        "iOO",
        uring_fd,
        callback,
        capsule
    );

    Py_DECREF(callback);
    Py_DECREF(capsule);
}