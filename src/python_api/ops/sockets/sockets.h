#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <arpa/inet.h>
#include <liburing.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "macroses.h"
#include "ops/sockets/sockets.h"
#include "registry/registry.h"
#include "buffer_controllers/buffer_controllers.h"
#include "python_api/loop/loop.h"
#include "python_macroses.h"

extern PyTypeObject *AioUringLoopType;
extern PyTypeObject AioUringSocketType;

typedef struct AioUringSocket {
    PyObject_HEAD

        int sock_fd;
    AioUringLoop *loop;
    int domain;
    struct sockaddr *addr;
    bool closed;
} AioUringSocket;

PyObject *
AioUring_prep_socket(PyObject *module, PyObject *args, PyObject *kwargs);

int
AioUringSocket_traverse(AioUringSocket *self, visitproc visit, void *arg);

int
AioUringSocket_clear(AioUringSocket *self);

PyObject *
AioUringSocket_aenter(AioUringSocket *self, PyObject *Py_UNUSED(ignored));

PyObject *
AioUringSocket_aexit(AioUringSocket *self, PyObject *args, PyObject *kwargs);

void
AioUringSocket_dealloc(AioUringSocket *self);

PyObject *
AioUringSocket_bind(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_connect(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_listen(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_accept(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_close(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_send(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_recv(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_sendto(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_recvfrom(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_sendmsg(AioUringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
AioUringSocket_recvmsg(AioUringSocket *self, PyObject *args, PyObject *kwargs);

int
recv_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int request_idx,
    int is_poll_first,
    TimeoutParams timeout_params
);

int
recvmsg_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int request_idx,
    int is_poll_first,
    struct msghdr *msghdr,
    TimeoutParams timeout_params
);

int
send_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    TransferMode transfer_mode,
    int request_idx,
    int is_poll_first,
    TimeoutParams timeout_params
);


int
sendmsg_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    TransferMode transfer_mode,
    int request_idx,
    const struct sockaddr *addr,
    size_t addrlen,
    int is_poll_first,
    TimeoutParams timeout_params
);


int
accept_dispatcher(
    AioUringSocket *self,
    StreamStrategy stream_strategy,
    int request_idx,
    struct sockaddr *addr,
    socklen_t *len,
    TimeoutParams timeout_params
);

PyObject *
_check_sockets_result(int result, AioUringSocket *socket, int request_idx, PyObject *future);

struct sockaddr_storage *
_serialize_address(const char *host, int port, int domain);

socklen_t
_get_socket_size(int domain);

PyObject *
_raise_socket_exception_group(PyObject *body_exc_type, PyObject *body_exc_val, PyObject *body_exc_tb);
