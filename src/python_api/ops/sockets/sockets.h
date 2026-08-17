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

extern PyTypeObject *UringioLoopType;
extern PyTypeObject UringioSocketType;

typedef struct UringioSocket {
    PyObject_HEAD

        int sock_fd;
    UringioLoop *loop;
    int domain;
    struct sockaddr *addr;
    bool closed;
} UringioSocket;

PyObject *
Uringio_prep_socket(PyObject *module, PyObject *args, PyObject *kwargs);

int
UringioSocket_traverse(UringioSocket *self, visitproc visit, void *arg);

int
UringioSocket_clear(UringioSocket *self);

PyObject *
UringioSocket_aenter(UringioSocket *self, PyObject *Py_UNUSED(ignored));

PyObject *
UringioSocket_aexit(UringioSocket *self, PyObject *args, PyObject *kwargs);

void
UringioSocket_dealloc(UringioSocket *self);

PyObject *
UringioSocket_bind(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_connect(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_listen(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_accept(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_close(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_send(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_recv(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_sendto(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_recvfrom(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_sendmsg(UringioSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
UringioSocket_recvmsg(UringioSocket *self, PyObject *args, PyObject *kwargs);

int
recv_dispatcher(
    UringioSocket *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int request_idx,
    int is_poll_first,
    TimeoutParams timeout_params
);

int
recvmsg_dispatcher(
    UringioSocket *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int request_idx,
    int is_poll_first,
    struct msghdr *msghdr,
    TimeoutParams timeout_params
);

int
send_dispatcher(
    UringioSocket *self,
    BufferPayload *buffer_payload,
    TransferMode transfer_mode,
    int request_idx,
    int is_poll_first,
    TimeoutParams timeout_params
);


int
sendmsg_dispatcher(
    UringioSocket *self,
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
    UringioSocket *self,
    StreamStrategy stream_strategy,
    int request_idx,
    struct sockaddr *addr,
    socklen_t *len,
    TimeoutParams timeout_params
);

PyObject *
_check_sockets_result(int result, UringioSocket *socket, int request_idx, PyObject *future);

struct sockaddr_storage *
_serialize_address(const char *host, int port, int domain);

socklen_t
_get_socket_size(int domain);

PyObject *
_raise_socket_exception_group(PyObject *body_exc_type, PyObject *body_exc_val, PyObject *body_exc_tb);
