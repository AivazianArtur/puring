#include "reader.h"


void on_uring_ready(UringLoop *self)
{
    struct io_uring_cqe *cqe;

    PyGILState_STATE gstate = PyGILState_Ensure();

    if (io_uring_peek_cqe(self->ring, &cqe) != 0) {
        PyGILState_Release(gstate);
        return;
    }

    do {
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(self->registry, index);
        if (!slot || !slot->future) {
            io_uring_cqe_seen(self->ring, cqe);
            continue;
        }

        PyObject *result = NULL;
        PyObject *exc = NULL;

        if (cqe->res < 0) {
            exc = PyObject_CallFunction(PyExc_OSError, "i", -cqe->res);
            if (exc) {
                PyObject_CallMethod(slot->future, "set_exception", "O", exc);
                Py_DECREF(exc);
            } else {
                PyErr_Print();
            }
        } else {
            if (slot->opcode == IORING_OP_READ && slot->buffer && PyBytes_Check(slot->buffer)) {
                result = PyBytes_FromStringAndSize(PyBytes_AS_STRING(slot->buffer), cqe->res);
            } else if (slot->opcode == IORING_OP_SOCKET && slot ->socket) {
                UringSocket *sock = (UringSocket *)slot->socket;
                sock->sock_fd = cqe->res;
                result = (PyObject*)slot->socket;
            } else if (slot->opcode == IORING_OP_ACCEPT && slot->socket) {
                UringSocket *conn = PyObject_New(UringSocket, &UringSocketType);
                if (!conn) {
                    PyErr_SetString(PyExc_RuntimeError, "Can't create socket");
                    PyErr_Print();
                    io_uring_cqe_seen(self->ring, cqe);
                    continue;
                }
                int new_fd = cqe->res;
                conn->sock_fd = new_fd;
                conn->closed = false;
                conn->loop = slot->socket->loop;
                Py_INCREF(slot->socket->loop);
                result = (PyObject*)conn;
            } else if (slot->opcode == IORING_OP_RECV && slot->socket) {
                if (cqe->res == 0) {
                    // EOF
                    result = PyBytes_FromStringAndSize(NULL, 0);
                } else if (cqe->res > 0) {
                    result = PyBytes_FromStringAndSize(
                        (char *)slot->buffer,
                        cqe->res
                    );
                } else result = NULL;
            } else if (slot->socket) {
                UringSocket *sock = (UringSocket *)slot->socket;
                sock->closed = false;
                result = (PyObject*)slot->socket;
            } else {
                result = PyLong_FromLong(cqe->res);
            }

            if (!result) {
                PyErr_Print();
                io_uring_cqe_seen(self->ring, cqe);
                continue;
            }

            PyObject *call_res = PyObject_CallMethod(slot->future, "set_result", "O", result);
            if (!call_res) PyErr_Print();
            Py_XDECREF(call_res);
            Py_XDECREF(result);
        }

        registry_remove(self->registry, index);
        io_uring_cqe_seen(self->ring, cqe);

    } while (io_uring_peek_cqe(self->ring, &cqe) == 0);

    PyGILState_Release(gstate);
}


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