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
        buffer,
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
        (struct sockaddr *)addr,
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

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_ACCEPT;
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

    const char* data;
    Py_ssize_t data_len;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"data", "data_len", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "y#|iO", kwlist, &data, &data_len, &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SEND;
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

    int result = uring_send(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        data,
        (socklen_t)data_len,
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

    unsigned int bufsize = 1024;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"bufsize", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|iiO", kwlist, &bufsize, &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_RECV;
    void *buffer = PyMem_Malloc(bufsize);
    if (!buffer) {
        Py_DECREF(future);
        PyErr_NoMemory();
        return NULL;
    }

    int request_idx = registry_add(
        self->loop->registry, future, (PyObject*)buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyMem_Free(buffer);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    int result = uring_recv(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer,
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

    const char* data;
    Py_ssize_t data_len;
    const char *host;
    int port;
    char domain;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"data", "data_len", "host", "port", "domain", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "y#sis|iO",
        kwlist,
        &data,
        &data_len,
        &host,
        &port,
        &domain,
        &flags,
        &timeout_params_obj
    ))) {
        return NULL;
    }


    struct sockaddr *addr = NULL;
    addr = serialize_address(host, port, self->domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SENDMSG;
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

    int result = uring_sendto(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        data,
        (socklen_t)data_len,
        addr,
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

    unsigned int bufsize = 1024;
    int flags = 0;

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"bufsize", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|iiO", kwlist, &bufsize , &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_RECV;
    char *buffer = PyMem_Malloc(bufsize);
    if (!buffer) {
        Py_DECREF(future);
        PyErr_NoMemory();
        return NULL;
    }

    int request_idx = registry_add(
        self->loop->registry, future, (PyObject*)buffer, NULL, opcode, NULL, self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyMem_Free(buffer);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    
    int result = uring_recvfrom(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer,
        (size_t)bufsize,
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
UringSocket_sendmsg(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {

}


PyObject* 
UringSocket_recvmsg(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {

}


PyObject* 
UringSocket_gesockname(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {

}


PyObject* 
UringSocket_getpeername(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {

}


PyObject* 
UringSocket_setsockopt(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {

}


PyObject* 
UringSocket_getsockopt(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
) {

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
