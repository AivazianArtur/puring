#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ops/sockets/sockets.h"
#include "python_api/loop/loop.h"
#include "python_api/future/future.h"


typedef struct UringLoop UringLoop;

typedef struct UringSocket {
    PyObject_HEAD

    int sock_fd;
    UringLoop *loop;
    bool closed;
} UringSocket;


PyObject* 
UringLoop_tcp_socket(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringLoop_udp_socket(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringLoop_unix_stream(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringLoop_unix_dgram(
    UringLoop *self,
    PyObject *args,
    PyObject *kwargs
);


void
UringSocket_dealloc(UringSocket *self);


PyObject* 
UringSocket_bind(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringSocket_listen(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringSocket_connect(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringSocket_send(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringSocket_recv(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringSocket_accept(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
);

PyObject* 
UringSocket_close(
    UringSocket *self,
    PyObject *args,
    PyObject *kwargs
);
