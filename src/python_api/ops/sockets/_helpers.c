#include "python_api/ops/sockets/sockets.h"

PyObject *
_check_sockets_result(int result, AioUringSocket *socket, int request_idx, PyObject *future) {
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(socket->loop->registry, request_idx);
        return NULL;
    }
    return future;
}

struct sockaddr_storage *
_serialize_address(const char *host, int port, int domain) {
    struct sockaddr_storage *addr = malloc(sizeof(struct sockaddr_storage));
    if (!addr) {
        PyErr_NoMemory();
        return NULL;
    }

    memset(addr, 0, sizeof(*addr));

    if (domain == AF_INET) {
        struct sockaddr_in *temp_addr = (struct sockaddr_in *)addr;
        temp_addr->sin_family = AF_INET;
        temp_addr->sin_port = htons((uint16_t)port);
        if (host && inet_pton(AF_INET, host, &temp_addr->sin_addr) != 1) {
            free(addr);
            PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IPv4 address");
            return NULL;
        }
    } else if (domain == AF_INET6) {
        struct sockaddr_in6 *temp_addr = (struct sockaddr_in6 *)addr;
        temp_addr->sin6_family = AF_INET6;
        temp_addr->sin6_port = htons((uint16_t)port);
        if (host && inet_pton(AF_INET6, host, &temp_addr->sin6_addr) != 1) {
            free(addr);
            PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IPv6 address");
            return NULL;
        }
    } else {
        free(addr);
        return NULL;
    }
    return addr;
}

socklen_t
_get_socket_size(int domain) {
    switch (domain) {
    case AF_INET6:
        return sizeof(struct sockaddr_in6);
    default:
        return sizeof(struct sockaddr_in);
    }
}

PyObject *
_raise_socket_exception_group(PyObject *body_exc_type, PyObject *body_exc_val, PyObject *body_exc_tb) {
    PyObject *close_type, *close_val, *close_tb;
    PyErr_Fetch(&close_type, &close_val, &close_tb);
    PyErr_NormalizeException(&close_type, &close_val, &close_tb);
    if (close_tb) {
        PyException_SetTraceback(close_val, close_tb);
    }
    Py_XDECREF(close_type);
    Py_XDECREF(close_tb);

    if (!close_val) {
        PyErr_Restore(body_exc_type, body_exc_val, body_exc_tb);
        Py_INCREF(body_exc_type);
        Py_XINCREF(body_exc_val);
        Py_XINCREF(body_exc_tb);
        return NULL;
    }

    if (body_exc_tb && body_exc_tb != Py_None) {
        PyException_SetTraceback(body_exc_val, body_exc_tb);
    }

    PyObject *builtins = PyImport_ImportModule("builtins");
    if (!builtins) {
        Py_DECREF(close_val);
        return NULL;
    }
    PyObject *eg_type = PyObject_GetAttrString(builtins, "ExceptionGroup");
    Py_DECREF(builtins);
    if (!eg_type) {
        Py_DECREF(close_val);
        return NULL;
    }

    PyObject *exc_list = PyList_New(2);
    if (!exc_list) {
        Py_DECREF(eg_type);
        Py_DECREF(close_val);
        return NULL;
    }
    Py_INCREF(body_exc_val);
    PyList_SET_ITEM(exc_list, 0, body_exc_val);
    PyList_SET_ITEM(exc_list, 1, close_val);

    PyObject *eg = PyObject_CallFunction(
        eg_type, "sO", "socket close failed while handling another exception", exc_list
    );
    Py_DECREF(eg_type);
    Py_DECREF(exc_list);
    if (!eg)
        return NULL;

    PyErr_SetObject((PyObject *)Py_TYPE(eg), eg);
    Py_DECREF(eg);
    return NULL;
}
