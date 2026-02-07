#include "sockets.h"


static PyObject*
UringLoop_tcp_socket(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
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

static PyObject*
UringLoop_udp_socket(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
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


static PyObject*
UringLoop_unix_stream(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
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


static PyObject*
UringLoop_unix_dgram(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->is_closing) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
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


static PyObject*
UringSocket_dealloc(UringSocket *self)
{
    self->closing = true;
    if (self->py_loop) {
        Py_XDECREF(self->py_loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject*
UringSocket_bind(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = NULL;
    int addr = NULL;
    int addrlen = NULL;

    static char *kwlist[] = {"fd", "addr", "addrlen", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, fd, addr, addrlen))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_BIND;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (bind(self->ring, request_idx, fd, addr, (socklen_t)addrlen) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_listen(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = NULL;
    int backlog = NULL;

    static char *kwlist[] = {"fd", "backlog", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, fd, backlog))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (listen(self->ring, request_idx, fd, backlog) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_connect(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int fd = NULL;
    const struct sockaddr *addr = NULL;
    int addrlen = NULL;

    static char *kwlist[] = {"fd", "addr", "addrlen", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "ioi", kwlist, fd, &addr, addrlen))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (connect(self->ring, request_idx, fd, *addr, addrlen) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_send(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = NULL;
    int len = NULL;
    int flags = NULL;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, sockfd, len, flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_SEND;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (send(self->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

static PyObject*
UringSocket_recv(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = NULL;
    int len = NULL;
    int flags = NULL;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, sockfd, len, flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_RECV;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (recv(self->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

static PyObject*
UringSocket_accept(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = NULL;
    int len = NULL;
    int flags = NULL;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, sockfd, len, flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_ACCEPT;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (accept(self->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_close(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);
    if (self->closed) {
        PyErr_SetString(PyExc_RuntimeError, "Loop is closing");
        return NULL;
    }

    int sockfd = NULL;

    static char *kwlist[] = {"sockfd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, sockfd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (close(self->ring, request_idx, sockfd) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}
