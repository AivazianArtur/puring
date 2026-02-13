#include "sockets.h"


PyObject*
UringLoop_tcp_socket(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(self->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (tcp_socket(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

PyObject*
UringLoop_udp_socket(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(self->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (udp_socket(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringLoop_unix_stream(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(self->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (unix_stream(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringLoop_unix_dgram(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(self);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(self->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (unix_dgram(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringSocket_dealloc(UringSocket *self)
{
    self->closed = true;
    if (self->loop) {
        Py_XDECREF(self->loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


PyObject*
UringSocket_bind(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *addr_obj;
    int fd;
    int addrlen;

    static char *kwlist[] = {"fd", "addr", "addrlen", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iOi", kwlist, &fd, &addr_obj, &addrlen)) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }
	// int opcode = IORING_OP_BIND;
    int opcode = IORING_OP_SEND;  // TEMP: Now its wrong implementation

    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(self->loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_bind(self->loop->ring, request_idx, fd, addr_obj, (socklen_t)addrlen, buffer) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringSocket_listen(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = 0;
    int backlog = 0;

    static char *kwlist[] = {"fd", "backlog", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, &fd, &backlog))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(self->loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_listen(self->loop->ring, request_idx, fd, backlog) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringSocket_connect(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = 0;
    PyObject *addr_obj;
    int addrlen = 0;

    static char *kwlist[] = {"fd", "addr", "addrlen", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iO", kwlist, &fd, &addr_obj, &addrlen))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_connect(self->loop->ring, request_idx, fd, addr_obj, addrlen) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringSocket_send(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = 0;
    int len = 0;
    int flags = 0;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, &sockfd, &len, &flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_SEND;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_send(self->loop->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

PyObject*
UringSocket_recv(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = 0;
    int len = 0;
    int flags = 0;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, &sockfd, &len, &flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_RECV;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_recv(self->loop->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

PyObject*
UringSocket_accept(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = 0;
    int len = 0;
    int flags = 0;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, &sockfd, &len, &flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_ACCEPT;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_accept(self->loop->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


PyObject*
UringSocket_close(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = 0;

    static char *kwlist[] = {"sockfd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &sockfd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(self->loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (uring_close_close(self->loop->ring, request_idx, sockfd) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}
