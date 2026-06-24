#include "sockets.h"

PyObject *
Puring_prep_socket(PyObject *Py_UNUSED(module), PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_IS_PURING();
    ASSERT_LOOP_THREAD(running_loop);
    ASSERT_RING_LOOP_IS_CLOSING(running_loop);

    PuringSocket *sock = PyObject_New(PuringSocket, &PuringSocketType);
    if (!sock) {
        return PyErr_NoMemory();
    }
    sock->closed = false;
    sock->loop = running_loop;
    Py_INCREF(running_loop);

    int domain = AF_INET;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"domain", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iO", (char **)kwlist, &domain, &timeout_params_obj)) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(running_loop);
    if (!future) {
        Py_DECREF(sock);
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    sock->domain = domain;

    int request_idx = registry_add(running_loop->registry, future, NULL, opcode, NULL, sock, NULL);
    if (request_idx < 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = prep_socket(running_loop->ring, request_idx, domain, &timeout_params);
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
        registry_remove(running_loop->registry, request_idx);
        return NULL;
    }

    return future;
}

void
PuringSocket_dealloc(PuringSocket *self) {
    self->closed = true;
    if (self->loop) {
        Py_XDECREF(self->loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *
PuringSocket_bind(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    const char *host;
    int port;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"host", "port", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si|O", (char **)kwlist, &host, &port, &timeout_params_obj)) {
        return NULL;
    }

    struct sockaddr *addr = _serialize_address(host, port, self->domain);
    if (!addr) {
        return NULL;
    }
    socklen_t addrlen = _get_socket_size(self->domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        return NULL;
    }

    int opcode = IORING_OP_BIND;

    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addr);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_bind(self->loop->ring, request_idx, self->sock_fd, addr, addrlen, self->state, &timeout_params);

    free(addr);
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_connect(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    const char *host;
    int port;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"host", "port", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si|O", (char **)kwlist, &host, &port, &timeout_params_obj)) {
        return NULL;
    }

    struct sockaddr *addr = _serialize_address(host, port, self->domain);
    if (!addr) {
        return NULL;
    }
    socklen_t addrlen = _get_socket_size(self->domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        free(addr);
        return NULL;
    }

    int result = uring_connect(
        self->loop->ring, request_idx, self->sock_fd, addr, addrlen, self->state, &timeout_params
    );
    free(addr);
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_listen(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    int backlog = 0;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"backlog", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i|O", (char **)kwlist, &backlog, &timeout_params_obj))) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_LISTEN;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_listen(self->loop->ring, request_idx, self->sock_fd, backlog, self->state, &timeout_params);
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_accept(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|iO", (char **)kwlist, &flags, &timeout_params_obj))) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    socklen_t *addrlen = malloc(sizeof(socklen_t));
    if (!addrlen) {
        PyErr_NoMemory();
        return NULL;
    }

    *addrlen = sizeof(struct sockaddr_storage);
    struct sockaddr_storage *peer_addr = calloc(1, sizeof(struct sockaddr_storage));

    int opcode = IORING_OP_ACCEPT;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, NULL, self, peer_addr);
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addrlen);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_accept(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        (struct sockaddr *)peer_addr,
        addrlen,
        flags,
        self->state,
        &timeout_params
    );
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_close(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char **)kwlist, &timeout_params_obj))) {
        return NULL;
    }

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_close_socket(self->loop->ring, request_idx, self->sock_fd, &timeout_params);
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_send(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    PyObject *data = NULL;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"data", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "O|iO", (char **)kwlist, &data, &flags, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_SEND;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    const char *buffer = PyBytes_AS_STRING(data);
    if (!buffer) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
        return NULL;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    int result = uring_send(
        self->loop->ring, request_idx, self->sock_fd, buffer, (size_t)size, flags, self->state, &timeout_params
    );
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_recv(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    int bufsize = 1024;
    PyObject *buffer_obj = NULL;
    int flags = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"bufsize", "buffer", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "|iOiO", (char **)kwlist, &bufsize, &buffer_obj, &flags, &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    BufferMetadata buffer_metadata = get_buffer_metadata(buffer_obj, NULL);
    BufferPayload *buffer_payload = create_buffer_payload(buffer_metadata, buffer_obj);
    if (!buffer_payload)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_RECV;

    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_recv(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_payload->linear->buffer,
        buffer_payload->linear->len,
        flags,
        self->state,
        &timeout_params
    );

    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_sendto(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
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
    static const char *kwlist[] = {"data", "host", "port", "domain", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "Osis|iO", (char **)kwlist, &data, &host, &port, &domain, &flags, &timeout_params_obj
        ))) {
        return NULL;
    }

    struct sockaddr *addr = NULL;
    addr = _serialize_address(host, port, domain);
    socklen_t addrlen = _get_socket_size(domain);

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        return NULL;
    }

    int opcode = IORING_OP_SENDMSG;
    int request_idx = registry_add(self->loop->registry, future, NULL, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addr);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    const char *buffer = PyBytes_AS_STRING(data);
    if (!buffer) {
        Py_DECREF(future);
        registry_remove(self->loop->registry, request_idx);
        free(addr);
        PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
        return NULL;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(data);

    int result = uring_sendto(
        self->loop->ring, request_idx, self->sock_fd, buffer, (size_t)size, addr, addrlen, flags, &timeout_params
    );
    free(addr);
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_recvfrom(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
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
    static const char *kwlist[] = {"bufsize", "buffer", "host", "port", "domain", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "|iOsisiO",
            (char **)kwlist,
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
    addr = _serialize_address(host, port, domain);
    // socklen_t addrlen = _get_socket_size(domain);

    BufferMetadata buffer_metadata = get_buffer_metadata(buffer_obj, NULL);
    BufferPayload *buffer_payload = create_buffer_payload(buffer_metadata, buffer_obj);
    if (!buffer_payload)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        return NULL;
    }
    TimeoutParams timeout_params = {0};

    int opcode = IORING_OP_RECVMSG;

    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addr);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_recvfrom(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_payload->linear->buffer,
        buffer_payload->linear->len,
        addr,
        // addrlen,
        flags,
        &timeout_params
    );

    free(addr);
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_sendmsg(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
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
    static const char *kwlist[] = {"buffers", "host", "port", "domain", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "|OsisiO", (char **)kwlist, &buffers_obj, &host, &port, &domain, &flags, &timeout_params_obj
        ))) {
        return NULL;
    }

    struct sockaddr *addr = NULL;
    addr = _serialize_address(host, port, domain);
    socklen_t addrlen = _get_socket_size(domain);


    BufferMetadata buffer_metadata = get_buffer_metadata(buffers_obj, NULL);
    BufferPayload *buffer_payload = create_buffer_payload(buffer_metadata, buffers_obj);
    if (!buffer_payload)
        return NULL;

    TimeoutParams timeout_params = {0};
    parse_timeout_params(timeout_params_obj, &timeout_params);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        return NULL;
    }

    int opcode = IORING_OP_SENDMSG;
    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addr);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_sendmsg(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_payload->vector->iovecs,
        (unsigned int)buffer_payload->vector->nr_vecs,
        addr,
        addrlen,
        flags,
        &timeout_params
    );

    free(addr);
    return _check_sockets_result(result, self, request_idx, future);
}

PyObject *
PuringSocket_recvmsg(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self->loop);
    ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    if (self->closed) {
        PyErr_SetString(PyExc_BrokenPipeError, "Socket is closed");
        return NULL;
    }

    PyObject *buffers_obj;
    int flags = 0;

    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"buffers", "flags", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "|OiO", (char **)kwlist, &buffers_obj, &flags, &timeout_params_obj
        ))) {
        return NULL;
    }

    BufferMetadata buffer_metadata = get_buffer_metadata(buffers_obj, NULL);
    BufferPayload *buffer_payload = create_buffer_payload(buffer_metadata, buffers_obj);
    if (!buffer_payload)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};

    int opcode = IORING_OP_RECVMSG;

    int request_idx = registry_add(self->loop->registry, future, buffer_payload, opcode, NULL, self, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = uring_recvmsg(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_payload->vector->iovecs,
        (unsigned int)buffer_payload->vector->nr_vecs,
        flags,
        &timeout_params
    );

    return _check_sockets_result(result, self, request_idx, future);
}
