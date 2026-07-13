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
#include "python_api/future/future.h"
#include "python_api/loop/loop.h"
#include "python_macroses.h"

extern PyTypeObject *PuringLoopType;
extern PyTypeObject PuringSocketType;

typedef struct PuringSocket {
    PyObject_HEAD

        int sock_fd;
    PuringLoop *loop;
    int domain;
    SOCKET_STATES state;
    struct sockaddr *addr;
    bool closed;
} PuringSocket;

PyObject *
Puring_prep_socket(PyObject *module, PyObject *args, PyObject *kwargs);

void
PuringSocket_dealloc(PuringSocket *self);

PyObject *
PuringSocket_bind(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_connect(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_listen(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_accept(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_close(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_send(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_recv(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_sendto(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_recvfrom(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_sendmsg(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringSocket_recvmsg(PuringSocket *self, PyObject *args, PyObject *kwargs);

PyObject *
_check_sockets_result(int result, PuringSocket *socket, int request_idx, PyObject *future);
struct sockaddr *
_serialize_address(const char *host, int port, int domain);
socklen_t
_get_socket_size(int domain);


int
recv_dispatcher(
    PuringSocket *self, BufferPayload *buffer_payload, int request_idx, int is_poll_first, TimeoutParams timeout_params
);

int
send_dispatcher(
    PuringSocket *self, BufferPayload *buffer_payload, int request_idx, int is_poll_first, TimeoutParams timeout_params
);


int
sendmsg_dispatcher(
    PuringSocket *self,
    BufferPayload *buffer_payload,
    int offset,
    const struct sockaddr *addr,
    size_t addrlen,
    int is_poll_first,
    TimeoutParams timeout_params
);


int
recvmsg_dispatcher(
    PuringSocket *self, BufferPayload *buffer_payload, int request_idx, int is_poll_first, TimeoutParams timeout_params
);
