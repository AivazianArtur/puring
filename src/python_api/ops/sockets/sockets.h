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


static PyObject* 
UringLoop_tcp_socket(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringLoop_udp_socket(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringLoop_unix_stream(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringLoop_unix_dgram(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);


static PyObject*
UringSocket_dealloc(UringSocket *self);


static PyObject* 
UringSocket_bind(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringSocket_listen(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringSocket_connect(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringSocket_send(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringSocket_recv(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringSocket_accept(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);

static PyObject* 
UringSocket_close(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
);
