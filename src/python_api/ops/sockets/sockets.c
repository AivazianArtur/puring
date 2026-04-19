#include "sockets.h"


PyObject*
UringLoop_tcp_socket(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
)
{
    ASSERT_LOOP_THREAD(self->py_loop);
    if (self->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->py_loop
        );
        return NULL;
    }

    UringSocket *sock = PyObject_New(UringSocket, &UringSocketType);
    if (!sock) {
        return PyErr_NoMemory();
    }
    sock->closed = false;
    sock->loop = self;
    Py_INCREF(self);

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj)) {
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

    int request_idx = registry_add(
        self->registry, 
        future,
        buffer,
        NULL,
        opcode,
        NULL,
        sock
    );
    if (request_idx < 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = tcp_socket(self->ring, request_idx, &timeout_params);
    if (result == -1) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->py_loop);
    if (self->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->py_loop
        );
        return NULL;
    }

    UringSocket *sock = PyObject_New(UringSocket, &UringSocketType);
    if (!sock) {
        return PyErr_NoMemory();
        return NULL;
    }

    sock->closed = false;
    sock->loop = self;
    Py_INCREF(self);

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj)) {
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

    int request_idx = registry_add(
        self->registry, 
        future, 
        buffer, 
        NULL,
        opcode, 
        NULL,
        sock
    );
    if (request_idx < 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = udp_socket(self->ring, request_idx, &timeout_params);
    if (result == -1) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->py_loop);
    if (self->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->py_loop
        );
        return NULL;
    }

    UringSocket *sock = PyObject_New(UringSocket, &UringSocketType);
    if (!sock) {
        return PyErr_NoMemory();
        return NULL;
    }

    sock->closed = false;
    sock->loop = self;
    Py_INCREF(self);

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj)) {
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

    int request_idx = registry_add(
        self->registry, 
        future,
        buffer,
        NULL,
        opcode,
        NULL,
        sock
    );
    if (request_idx < 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = unix_stream(self->ring, request_idx, &timeout_params);
    if (result == -1) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->py_loop);
    if (self->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->py_loop
        );
        return NULL;
    }

    UringSocket *sock = PyObject_New(UringSocket, &UringSocketType);
    if (!sock) {
        return PyErr_NoMemory();
        return NULL;
    }

    sock->closed = false;
    sock->loop = self;
    Py_INCREF(self);

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &timeout_params_obj)) {
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

    int request_idx = registry_add(
        self->registry, 
        future, 
        buffer, 
        NULL,
        opcode, 
        NULL,
        sock
    );
    if (request_idx < 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = unix_dgram(self->ring, request_idx, &timeout_params);
    if (result == -1) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        registry_remove(self->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
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

    struct sockaddr_in *addr = malloc(sizeof(*addr));
    if (!addr) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(addr, 0, sizeof(*addr));

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr->sin_addr) != 1) {
        free(addr);
        PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IP address");
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
        self->loop->registry,
        future, 
        buffer,
        NULL,
        opcode,
        NULL,
        self
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
        (struct sockaddr *)addr,
        sizeof(*addr),
        buffer,
        &timeout_params
    );
    if (result == -1) {
        Py_DECREF(future);
        free(addr);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        free(addr);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    free(addr);
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
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
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

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;

    int request_idx = registry_add(
        self->loop->registry, 
        future, 
        buffer, 
        NULL,
        opcode, 
        NULL,
        self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_listen(self->loop->ring, request_idx, self->sock_fd, backlog, &timeout_params); 
    if (result == -1) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    ASSERT_LOOP_THREAD(self->loop->py_loop);
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
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

    struct sockaddr_in *addr = malloc(sizeof(*addr));
    if (!addr) { PyErr_NoMemory(); return NULL; }
    memset(addr, 0, sizeof(*addr));

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr->sin_addr) != 1) {
        free(addr);
        PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IP address");
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
        self->loop->registry, 
        future, 
        buffer, 
        NULL,
        opcode, 
        NULL,
        self
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
        sizeof(*addr),
        &timeout_params
    );
    if (result == -1) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    const char* bytes_buf;
    Py_ssize_t bytes_len;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"bytes_buf", "bytes_len", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "y#|iO", kwlist, &bytes_buf, &bytes_len, &flags, &timeout_params_obj))) {
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
        self->loop->registry,
        future,
        buffer,
        NULL,
        opcode, 
        NULL,
        self
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
        bytes_buf,
        (socklen_t)bytes_len,
        flags,
        &timeout_params
    );
    if (result == -1) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    unsigned int len = 1024;
    int flags = 0;

    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"len", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|iiO", kwlist, &len, &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_RECV;
    // For now whoile puring without real buffer, we'll do it in next v.
    char *buffer = PyMem_Malloc(len);
    if (!buffer) {
        Py_DECREF(future);
        PyErr_NoMemory();
        return NULL;
    }

    int request_idx = registry_add(
        self->loop->registry,
        future,
        (PyObject*)buffer,
        NULL,
        opcode,
        NULL,
        self
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
        (size_t)len,
        flags,
        &timeout_params
    );
    if (result == -1) {
        Py_DECREF(future);
        PyMem_Free(buffer);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        PyMem_Free(buffer);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    unsigned int len = 1024;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static char *kwlist[] = {"len", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|iiO", kwlist, &len, &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_ACCEPT;
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(
        self->loop->registry, 
        future, 
        buffer, 
        NULL,
        opcode, 
        NULL,
        self
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
        buffer,
        &len,
        flags,
        &timeout_params
    );
    if (result == -1) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
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
    if (self->loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            self->loop->py_loop
        );
        return NULL;
    }
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
        self->loop->registry, 
        future, 
        buffer, 
        NULL,
        opcode, 
        NULL,
        self
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_close_socket(self->loop->ring, request_idx, self->sock_fd, &timeout_params);
    if (result == -1) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    } else if (result == 0) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_RuntimeError, "SQE submission failed");
        return NULL;
    }
    return future;
}
