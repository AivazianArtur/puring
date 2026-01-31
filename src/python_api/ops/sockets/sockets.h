#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>
#include <sys/types.h>
#include <stdbool.h>


#include "loop.h"


typedef struct {
    PyObject_HEAD

    int sock_fd;
    UringLoop *loop;
    bool closed;
} UringSocket;


UringSocket *UringLoop_tcp_socket(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

UringSocket *UringLoop_udp_socket(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

UringSocket *UringLoop_unix_stream(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

UringSocket *UringLoop_unix_dgram(
    UringLoop* loop, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);


static PyObject *UringSocket_bind(
    UringSocket* socket, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject *UringSocket_listen(
    UringSocket* socket, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject *UringSocket_connect(
    UringSocket* socket, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject *UringSocket_send(
    UringSocket* socket, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject *UringSocket_recv(
    UringSocket* socket, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject *UringSocket_accept(
    UringSocket* socket, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

static PyObject *UringSocket_close(
    UringSocket* socket, 
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
);

