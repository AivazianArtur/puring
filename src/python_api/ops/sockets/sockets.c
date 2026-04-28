#include "sockets.h"


PyObject*
UringLoop_prep_socket(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    UringSocket *sock = PyObject_New(UringSocket, &UringSocketType);
    if (!sock) {
        return PyErr_NoMemory();
    }
    sock->closed = false;
    sock->loop = self;
    Py_INCREF(self);

    int domain = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"domain", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|O", kwlist, &domain, &timeout_params_obj)) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self);
    if (!future) {
        Py_DECREF(sock);
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    sock->domain=domain;

    int request_idx = registry_add(
        self->registry, future, buffer, NULL, opcode, NULL, sock
    );
    if (request_idx < 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = prep_socket(self->ring, request_idx, domain, &timeout_params);
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Passed socket domain values are not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        return NULL;
    }

    return future;
}


void 
UringSocket_dealloc(UringSocket *self)
{
    self->closed = true;
    if (self->loop) {
        Py_XDECREF(self->loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


PyObject*
UringSocket_bind(UringSocket *self, PyObject *args, PyObject *kwargs)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    const char *host;
    int port;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"host", "port", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si|O", kwlist, &host, &port, &timeout_params_obj)) {
        return NULL;
    }

    struct sockaddr *addr = serialize_address(host, port, self->domain);
    if (!addr) {
        return NULL;
    }
    socklen_t addrlen = get_socket_size(self->domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) { 
        free(addr); 
        return NULL;
    }

    int opcode = IORING_OP_BIND;
    PyObject *buffer = NULL;

    int request_idx = registry_add(
        self->loop->registry, future, buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addr);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_bind(
        self->loop->ring, 
        request_idx,
        self->sock_fd,
        addr,
        addrlen,
        self->state,
        &timeout_params
    );
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        free(addr);
        registry_remove(self->loop->registry, request_idx);
        return NULL;
    }
    free(addr);
    return future;
}


PyObject*
UringSocket_connect(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    const char *host;
    int port;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"host", "port", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si|O", kwlist, &host, &port, &timeout_params_obj)) {
        return NULL;
    }

    struct sockaddr *addr = serialize_address(host, port, self->domain);
    if (!addr) {
        return NULL;
    }
    socklen_t addrlen = get_socket_size(self->domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(
        self->loop->registry, future, buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_connect(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        addr,
        addrlen,
        self->state,
        &timeout_params
    );
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    int backlog = 0;
    PyObject *timeout_params_obj = NULL;

    static char *kwlist[] = {"backlog", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i|O", kwlist, &backlog, &timeout_params_obj))) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_LISTEN;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(
        self->loop->registry, future, buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_listen(
        self->loop->ring, request_idx, self->sock_fd, backlog, self->state, &timeout_params
    ); 
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    unsigned int len = 1024;
    const char *host;
    int port;
    char domain;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"host", "port", "domain", "len", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "sis|iiO",
        kwlist,
        &host,
        &port,
        &domain,
        &len,
        &flags,
        &timeout_params_obj
    ))) {
        return NULL;
    }

    struct sockaddr *addr = NULL;
    addr = serialize_address(host, port, domain);
    socklen_t addrlen = get_socket_size(domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_ACCEPT;
    // TEMP: no buffer
    int request_idx = registry_add(
        self->loop->registry, future, NULL, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_accept(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        addr,
        (socklen_t *)len,
        flags,
        self->state,
        &timeout_params
    );
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    PyObject* timeout_params_obj = NULL;

    static char *kwlist[] = {"timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj))) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    // TEMP: no buffer
    PyObject *buffer = NULL;
    int request_idx = registry_add(
        self->loop->registry, future, buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_close_socket(self->loop->ring, request_idx, self->sock_fd, &timeout_params);
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    PyObject *data = NULL;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"data", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "O|iO", kwlist, &data, &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SEND;
    int request_idx = registry_add(
        self->loop->registry, future, data, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    char *buffer = PyBytes_AS_STRING(data);
    if (!buffer) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
        return NULL;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    int result = uring_send(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer,
        (size_t)size,
        flags,
        self->state,
        &timeout_params
    );
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    int bufsize = 1024;
    PyObject *buffer_obj = NULL;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"bufsize", "buffer", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|iOiO", kwlist, &bufsize, &buffer_obj, &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    BufferResult *buffer_result = get_buffer(buffer_obj, bufsize);
    if (buffer_result) {
        return NULL;
    }
    
    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_RECV;

    int request_idx = registry_add(
        self->loop->registry, future, (PyObject*)buffer_result->buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyMem_Free(buffer_result->buffer);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    int result = uring_recv(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_result->buffer,
        buffer_result->buffer_len,
        flags,
        self->state,
        &timeout_params
    );

    if (buffer_result->buffer_flag == 0) {
        PyBuffer_Release(buffer_result->view);
    } else {
        PyMem_Free(buffer_result->buffer);
    }

    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        return NULL;
    }
    return future;
}


PyObject* 
UringSocket_sendto(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    PyObject *data = NULL;
    const char *host;
    int port;
    char domain;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"data", "host", "port", "domain", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "Osis|iO",
        kwlist,
        &data,
        &host,
        &port,
        &domain,
        &flags,
        &timeout_params_obj
    ))) {
        return NULL;
    }

    struct sockaddr *addr = NULL;
    addr = serialize_address(host, port, domain);
    socklen_t addrlen = get_socket_size(domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SENDMSG;
    int request_idx = registry_add(
        self->loop->registry, future, data, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    char *buffer = PyBytes_AS_STRING(data);
    if (!buffer) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
        return NULL;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    int result = uring_sendto(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer,
        (size_t)size,
        addr,
        addrlen,
        flags,
        &timeout_params
    );
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        return NULL;
    }

    return future;
}


PyObject* 
UringSocket_recvfrom(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    const char *host;
    int port;
    char domain;
    unsigned int bufsize = 1024;
    PyObject *buffer_obj = NULL;
    int flags = 0;

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"bufsize", "buffer", "host", "port", "domain","flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|iOsisiO",
        kwlist,
        &bufsize,
        &buffer_obj,
        &host,
        &port,
        &domain,
        &flags,
        &timeout_params_obj
    ))) {
        return NULL;
    }

    struct sockaddr *addr = NULL;
    addr = serialize_address(host, port, domain);
    socklen_t addrlen = get_socket_size(domain);

    BufferResult *buffer_result = get_buffer(buffer_obj, bufsize);
    if (buffer_result) {
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};

    int opcode = IORING_OP_RECV;

    int request_idx = registry_add(
        self->loop->registry, future, (PyObject*)buffer_result->buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        if (buffer_result->buffer_flag == 0) {
            PyBuffer_Release(buffer_result->view);
        } else {
            PyMem_Free(buffer_result->buffer);
        }
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    int result = uring_recvfrom(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_result->buffer,
        buffer_result->buffer_len,
        addr,
        addrlen,
        flags,
        &timeout_params
    );

    if (buffer_result->buffer_flag == 0) {
        PyBuffer_Release(buffer_result->view);
    } else {
        PyMem_Free(buffer_result->buffer);
    }

    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        return NULL;
    }

    return future;
}


PyObject* 
UringSocket_sendmsg(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }
 
    PyObject *buffers_obj;
    const char *host;
    int port;
    char domain;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"buffers", "host", "port", "domain", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "O|sisiO",
        kwlist,
        &buffers_obj,
        &host,
        &port,
        &domain,
        &flags,
        &timeout_params_obj
    ))) {
        return NULL;
    }

    struct sockaddr *addr = NULL;
    addr = serialize_address(host, port, domain);
    socklen_t addrlen = get_socket_size(domain);

    IovecsResult *iovecs_result = serialize_iovecs_buffer(buffers_obj);
    if (iovecs_result) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SENDMSG;
    int request_idx = registry_add(
        self->loop->registry, future, NULL, iovecs_result->iovecs_buf, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_sendmsg(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        iovecs_result->iovecs,
        iovecs_result->nr_vecs,
        addr,
        addrlen,
        flags,
        &timeout_params
    );

    free(iovecs_result);

    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        return NULL;
    }

    return future;
}


PyObject* 
UringSocket_recvmsg(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    PyObject *buffers_obj;
    int flags = 0;

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"buffers", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "O|iO",
        kwlist,
        &buffers_obj,
        &flags,
        &timeout_params_obj
    ))) {
        return NULL;
    }

    IovecsResult *iovecs_result = serialize_iovecs_buffer(buffers_obj);
    if (iovecs_result) {
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};

    int opcode = IORING_OP_RECV;

    int request_idx = registry_add(
        self->loop->registry, future, NULL, iovecs_result->iovecs_buf, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    int result = uring_recvmsg(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        iovecs_result->iovecs,
        iovecs_result->nr_vecs,
        flags,
        &timeout_params
    );

    free(iovecs_result);

    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        return NULL;
    }

    return future;
}


struct sockaddr* serialize_address(const char *host, int port, int domain) {
    struct sockaddr *addr;
    if (domain == AF_INET) {
        struct sockaddr_in *temp_addr = malloc(sizeof(*temp_addr));
        if (!temp_addr) {
            PyErr_NoMemory();
            return NULL;
        }
    
        temp_addr->sin_family=AF_INET;
        temp_addr->sin_port = htons(port);
        if (inet_pton(AF_INET, host, &temp_addr->sin_addr) != 1) {
            free(temp_addr);
            PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IP address");
            return NULL;
        }
        addr = (struct sockaddr*)temp_addr;
        free(temp_addr);
    } else if (domain == AF_INET6) {
        struct sockaddr_in6 *temp_addr = malloc(sizeof(*temp_addr));
        if (!addr) {
            PyErr_NoMemory();
            return NULL;
        }
    
        temp_addr->sin6_family=AF_INET;
        temp_addr->sin6_port = htons(port);
        if (inet_pton(AF_INET, host, &temp_addr->sin6_addr) != 1) {
            free(temp_addr);
            PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IP address");
            return NULL;
        }
        addr = (struct sockaddr*)temp_addr;
        free(temp_addr);
    } else {
        return NULL;
    }
    memset(addr, 0, sizeof(*addr));
    return addr;
}


socklen_t get_socket_size(int domain) {
    switch (domain) {
        case AF_INET6:
            return sizeof(struct sockaddr_in6);
        default:
            return sizeof(struct sockaddr_in);
    }
}

IovecsResult* serialize_iovecs_buffer(PyObject *buffers_obj) {
    PyObject *seq = PySequence_Fast(buffers_obj, "Buffers must be a sequence");
    if (!seq) {
        return NULL;
    }
    Py_ssize_t nr_vecs = PySequence_Fast_GET_SIZE(seq);
    PyObject **items = PySequence_Fast_ITEMS(seq);

    struct iovec *iovecs = PyMem_Malloc(sizeof(struct iovec) * nr_vecs);
    if (!iovecs) {
        Py_DECREF(seq);
        PyErr_NoMemory();
        return NULL;
    }

    Py_buffer *iovecs_buf = PyMem_Malloc(sizeof(Py_buffer) * nr_vecs);

    for (Py_ssize_t i = 0; i < nr_vecs; i++) {
        if (PyObject_GetBuffer(items[i], &iovecs_buf[i], PyBUF_SIMPLE) < 0) {
            for (Py_ssize_t j = 0; j < i; j++) {
                PyBuffer_Release(&iovecs_buf[j]);
            }
            PyMem_Free(iovecs_buf);
            PyMem_Free(iovecs);
            Py_DECREF(seq);
            
            return NULL;
        }

        iovecs[i].iov_base = iovecs_buf[i].buf;
        iovecs[i].iov_len  = iovecs_buf[i].len;
    }

    IovecsResult *result = malloc(sizeof(IovecsResult));
    result->nr_vecs = nr_vecs;
    result->iovecs = iovecs;
    result->iovecs_buf = iovecs_buf;
    return result;
}

BufferResult* get_buffer(PyObject *buffer_obj, int bufsize) {
    void *buffer = NULL;
    size_t buffer_len;

    Py_buffer view;
    int buffer_flag;
    if (buffer_obj && buffer_obj != Py_None) {
        if (PyObject_GetBuffer(buffer_obj, &view, PyBUF_WRITABLE) < 0) {
            return NULL;
        }
        buffer_flag = 0;
        buffer = view.buf;
        buffer_len = view.len;
    } else {
        buffer = PyMem_Malloc(bufsize);
        if (!buffer) {
            PyErr_NoMemory();
            return NULL;
        }
        buffer_len = (size_t)bufsize;
        buffer_flag = 1;
    }

    BufferResult *result = malloc(sizeof(BufferResult));
    result->buffer = buffer;
    result->buffer_len = buffer_len;
    result->view = &view;
    result->buffer_flag = buffer_flag;
    return result;
}
