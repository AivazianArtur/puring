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
#include "registry/registry.h"
#include "python_macroses.h"


extern PyTypeObject UringSocketType;


typedef enum SOCKET_STATES {
  NEW,
  BOUND,
  LISTENING,
  CONNECTED,
  CLOSED
} SOCKET_STATES;


typedef struct UringSocket {
    PyObject_HEAD

    int sock_fd;
    UringLoop *loop;
    int domain;
    int type;
    SOCKET_STATES state;
    bool closed;
} UringSocket;


PyObject* 
UringLoop_prep_socket(
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
