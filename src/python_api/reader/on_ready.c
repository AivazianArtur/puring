#include "reader.h"

void on_uring_ready(UringLoop *loop)
{
    struct io_uring_cqe *cqe;

    PyGILState_STATE gstate = PyGILState_Ensure();

    while (io_uring_peek_cqe(loop->ring, &cqe) == 0) {
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(loop->registry, index);

        if (!slot || !slot->future) {
            io_uring_cqe_seen(loop->ring, cqe);
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
            switch (slot->opcode) {
                case IORING_OP_READ:
                    if (slot->buffer && PyBytes_Check(slot->buffer)) {
                        result = PyBytes_FromStringAndSize(PyBytes_AS_STRING(slot->buffer), cqe->res);
                    }
                    break;
                case IORING_OP_OPENAT2:
                    if (slot->file) {
                        UringFile *file = (UringFile *)slot->file;
                        file->fd = cqe->res;
                        // TODO: maybe should try to fix return types to return file and socket here and below?
                        result = (PyObject *)slot->file;
                    }
                    break;
                case IORING_OP_SOCKET:
                    if (slot->socket) {
                        UringSocket *sock = (UringSocket *)slot->socket;
                        sock->sock_fd = cqe->res;
                        result = (PyObject *)slot->socket;
                    }
                    break;
                case IORING_OP_ACCEPT:
                    if (slot->socket) {
                        UringSocket *conn = PyObject_New(UringSocket, &UringSocketType);
                        if (!conn) {
                            PyErr_SetString(PyExc_RuntimeError, "Can't create socket");
                            PyErr_Print();
                            io_uring_cqe_seen(loop->ring, cqe);
                            continue;
                        }
                        conn->sock_fd = cqe->res;
                        conn->closed = false;
                        conn->loop = slot->socket->loop;
                        Py_INCREF(conn->loop);
                        result = (PyObject *)conn;
                    }
                    break;
                case IORING_OP_RECV:
                    if (slot->socket) {
                        if (cqe->res == 0) {
                            result = PyBytes_FromStringAndSize(NULL, 0);
                        } else if (cqe->res > 0) {
                            result = PyBytes_FromStringAndSize((char *)slot->buffer, cqe->res);
                        }
                    }
                    break;
                default:
                    result = PyLong_FromLong(cqe->res);
            }
        }

        if (result) {
            PyObject *res_call = PyObject_CallMethod(slot->future, "set_result", "O", result);
            Py_XDECREF(res_call);
            Py_DECREF(result);
        }

        registry_remove(loop->registry, index);
        io_uring_cqe_seen(loop->ring, cqe);
    }

    PyGILState_Release(gstate);
}