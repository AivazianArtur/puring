#include "python_api/ops/sockets/sockets.h"

PyObject *
Puring_prep_socket(PyObject *Py_UNUSED(module), PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_IS_PURING();
    ASSERT_LOOP_THREAD(running_loop);
    ASSERT_RING_LOOP_IS_CLOSING(running_loop);

    PuringSocket *sock = PyObject_GC_New(PuringSocket, &PuringSocketType);
    if (!sock) {
        return PyErr_NoMemory();
    }
    sock->closed = false;
    sock->loop = running_loop;
    sock->addr = NULL;
    Py_INCREF(running_loop);
    PyObject_GC_Track(sock);

    int domain = AF_INET;
    int socktype = SOCK_STREAM;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"domain", "socktype", "timeout_params", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iiO", (char **)kwlist, &domain, &socktype, &timeout_params_obj)) {
        Py_DECREF(sock);
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0) {
        Py_DECREF(sock);
        return NULL;
    }

    PyObject *future = create_future(running_loop);
    if (!future) {
        Py_DECREF(sock);
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    sock->domain = domain;

    int request_idx = registry_add(running_loop->registry, future, NULL, ONESHOT, opcode, NULL, sock, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(sock);
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = prep_socket(running_loop->ring, request_idx, domain, socktype, timeout_params);
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

int
PuringSocket_traverse(PuringSocket *self, visitproc visit, void *arg) {
    Py_VISIT(self->loop);
    return 0;
}

int
PuringSocket_clear(PuringSocket *self) {
    Py_CLEAR(self->loop);
    return 0;
}

PyObject *
PuringSocket_aenter(PuringSocket *self, PyObject *Py_UNUSED(ignored)) {
    PyObject *future = create_future(self->loop);
    if (!future)
        return NULL;

    PyObject *res_call = PyObject_CallMethod(future, "set_result", "O", self);
    Py_XDECREF(res_call);
    return future;
}

PyObject *
PuringSocket_aexit(PuringSocket *self, PyObject *args, PyObject *kwargs) {
    PyObject *exc_type = NULL;
    PyObject *exc_val = NULL;
    PyObject *exc_tb = NULL;

    static const char *kwlist[] = {"exc_type", "exc_val", "exc_tb", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO", (char **)kwlist, &exc_type, &exc_val, &exc_tb))
        return NULL;

    int had_exception = (exc_type != Py_None);

    TimeoutParams timeout_params = {0};
    PyObject *future = create_future(self->loop);
    if (!future) {
        if (had_exception) {
            return _raise_socket_exception_group(exc_type, exc_val, exc_tb);
        }
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, NULL, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        if (had_exception) {
            return _raise_socket_exception_group(exc_type, exc_val, exc_tb);
        }
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    self->closed = 1;

    int result = puring_close_socket(self->loop->ring, request_idx, self->sock_fd, timeout_params);
    PyObject *validated_result = _check_sockets_result(result, self, request_idx, future);
    if (!validated_result) {
        self->closed = 0;
        if (had_exception) {
            return _raise_socket_exception_group(exc_type, exc_val, exc_tb);
        }
        return NULL;
    }
    return validated_result;
}

void
PuringSocket_dealloc(PuringSocket *self) {
    PyObject_GC_UnTrack(self);
    self->closed = true;
    if (self->addr) {
        free(self->addr);
        self->addr = NULL;
    }
    PuringSocket_clear(self);
    freefunc free_func = PyType_GetSlot(Py_TYPE(self), Py_tp_free);
    if (free_func) {
        free_func(self);
    } else {
        PyObject_GC_Del(self);
    }
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
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    struct sockaddr_storage *addr = _serialize_address(host, port, self->domain);
    if (!addr)
        return NULL;

    socklen_t addrlen = _get_socket_size(self->domain);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        return NULL;
    }

    int opcode = IORING_OP_BIND;

    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, NULL, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addr);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_bind(
        self->loop->ring, request_idx, self->sock_fd, (struct sockaddr *)addr, addrlen, timeout_params
    );

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
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    struct sockaddr_storage *addr = _serialize_address(host, port, self->domain);
    if (!addr)
        return NULL;

    socklen_t addrlen = _get_socket_size(self->domain);

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, NULL, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        free(addr);
        return NULL;
    }

    int result = puring_connect(
        self->loop->ring, request_idx, self->sock_fd, (struct sockaddr *)addr, addrlen, timeout_params
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

    int backlog = 1;
    PyObject *timeout_params_obj = NULL;

    static const char *kwlist[] = {"backlog", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|iO", (char **)kwlist, &backlog, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_LISTEN;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, NULL, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_listen(self->loop->ring, request_idx, self->sock_fd, backlog, timeout_params);
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

    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char **)kwlist, &timeout_params_obj))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

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

    StreamStrategy stream_strategy = get_stream_strategy();

    int opcode = IORING_OP_ACCEPT;
    int request_idx = registry_add(
        self->loop->registry, future, NULL, stream_strategy, opcode, NULL, self, peer_addr, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addrlen);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    int result = accept_dispatcher(
        self, stream_strategy, request_idx, (struct sockaddr *)peer_addr, addrlen, timeout_params
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
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    PyObject *future = create_future(self->loop);
    if (!future)
        return NULL;

    int opcode = IORING_OP_CLOSE;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, NULL, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    self->closed = true;
    int result = puring_close_socket(self->loop->ring, request_idx, self->sock_fd, timeout_params);
    PyObject *validated_result = _check_sockets_result(result, self, request_idx, future);
    if (!validated_result) {
        self->closed = false;
    }
    return validated_result;
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
    int is_poll_first = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"data", "is_poll_first", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "O|pO", (char **)kwlist, &data, &is_poll_first, &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    BufferPayload *buffer_payload = create_buffer_payload_from_data(data);
    if (!buffer_payload)
        return NULL;

    TransferMode transfer_mode = get_transfer_mode();

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_SEND;
    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, NULL, self, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    int result = send_dispatcher(self, buffer_payload, transfer_mode, request_idx, is_poll_first, timeout_params);
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
    int is_poll_first = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"bufsize", "buffer", "is_poll_first", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "|iOpO", (char **)kwlist, &bufsize, &buffer_obj, &is_poll_first, &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    BufferPayload *buffer_payload = get_or_create_linear_buffer(buffer_obj, bufsize);
    if (!buffer_payload)
        return NULL;

    StreamStrategy stream_strategy = get_stream_strategy();

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_RECV;

    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, stream_strategy, opcode, NULL, self, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    int result = recv_dispatcher(self, buffer_payload, stream_strategy, request_idx, is_poll_first, timeout_params);
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
    int domain;
    int is_poll_first = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"data", "host", "port", "domain", "is_poll_first", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "Osii|pO", (char **)kwlist, &data, &host, &port, &domain, &is_poll_first, &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    struct sockaddr_storage *addr = NULL;
    addr = _serialize_address(host, port, domain);
    if (!addr)
        return NULL;

    socklen_t addrlen = _get_socket_size(domain);

    BufferPayload *buffer_payload = create_buffer_payload_from_data(data);
    if (!buffer_payload) {
        free(addr);
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        free(addr);
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_SENDMSG;
    int request_idx = registry_add(self->loop->registry, future, NULL, ONESHOT, opcode, NULL, self, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        free(addr);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_sendto(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_payload->linear->buffer,
        (unsigned)buffer_payload->linear->len,
        (struct sockaddr *)addr,
        addrlen,
        is_poll_first,
        timeout_params
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
    int domain;
    unsigned int bufsize = 1024;
    PyObject *buffer_obj = NULL;
    int is_poll_first = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {
        "bufsize", "buffer", "host", "port", "domain", "is_poll_first", "timeout_params", NULL
    };
    if (!(PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "|iOsiipO",
            (char **)kwlist,
            &bufsize,
            &buffer_obj,
            &host,
            &port,
            &domain,
            &is_poll_first,
            &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    struct sockaddr_storage *addr = _serialize_address(host, port, domain);
    if (!addr)
        return NULL;

    socklen_t addrlen = _get_socket_size(domain);

    BufferPayload *buffer_payload = get_or_create_linear_buffer(buffer_obj, (int)bufsize);
    if (!buffer_payload) {
        free(addr);
        return NULL;
    }

    struct msghdr *msg = malloc(sizeof(struct msghdr) + sizeof(struct iovec));
    if (!msg) {
        free_buffer_payload(buffer_payload, false);
        free(addr);
        PyErr_NoMemory();
        return NULL;
    }

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        free(addr);
        free(msg);
        return NULL;
    }

    int opcode = IORING_OP_RECVMSG;
    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, NULL, self, addr, msg
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        free(addr);
        free(msg);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = puring_recvfrom(
        self->loop->ring,
        request_idx,
        self->sock_fd,
        buffer_payload->linear->buffer,
        buffer_payload->linear->len,
        (struct sockaddr *)addr,
        addrlen,
        is_poll_first,
        msg,
        timeout_params
    );

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

    PyObject *buffers_obj = NULL;
    const char *host = NULL;
    int port = 0;
    int domain = 0;
    int is_poll_first = 0;
    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"buffers", "host", "port", "domain", "is_poll_first", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "O|siipO",
            (char **)kwlist,
            &buffers_obj,
            &host,
            &port,
            &domain,
            &is_poll_first,
            &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    struct sockaddr_storage *addr = NULL;
    if (host) {
        addr = _serialize_address(host, port, domain);
        if (!addr)
            return NULL;
    }
    socklen_t addrlen = addr ? _get_socket_size(domain) : 0;

    BufferPayload *buffer_payload = get_or_create_vectored_buffer(buffers_obj);
    if (!buffer_payload)
        return NULL;

    TransferMode transfer_mode = get_transfer_mode();

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        free(addr);
        return NULL;
    }

    int opcode = IORING_OP_SENDMSG;
    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, ONESHOT, opcode, NULL, self, NULL, NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        free(addr);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    int result = sendmsg_dispatcher(
        self,
        buffer_payload,
        transfer_mode,
        request_idx,
        (struct sockaddr *)addr,
        addrlen,
        is_poll_first,
        timeout_params
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
    int is_poll_first = 0;

    PyObject *timeout_params_obj = NULL;
    static const char *kwlist[] = {"buffers", "is_poll_first", "timeout_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(
            args, kwargs, "|OpO", (char **)kwlist, &buffers_obj, &is_poll_first, &timeout_params_obj
        ))) {
        return NULL;
    }
    TimeoutParams timeout_params = {0};
    if (parse_timeout_params(timeout_params_obj, &timeout_params) < 0)
        return NULL;

    BufferPayload *buffer_payload = get_or_create_vectored_buffer(buffers_obj);
    if (!buffer_payload)
        return NULL;

    StreamStrategy stream_strategy = get_stream_strategy();

    PyObject *future = create_future(self->loop);
    if (!future) {
        free_buffer_payload(buffer_payload, false);
        return NULL;
    }

    int opcode = IORING_OP_RECVMSG;

    struct msghdr *msghdr = malloc(sizeof(struct msghdr));

    int request_idx = registry_add(
        self->loop->registry, future, buffer_payload, stream_strategy, opcode, NULL, self, NULL, msghdr
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        free_buffer_payload(buffer_payload, false);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }
    int result = recvmsg_dispatcher(
        self, buffer_payload, stream_strategy, request_idx, is_poll_first, msghdr, timeout_params
    );
    return _check_sockets_result(result, self, request_idx, future);
}
