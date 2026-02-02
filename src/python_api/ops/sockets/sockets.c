#include "sockets.h"


static PyObject*
UringLoop_tcp_socket(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (tcp_socket(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

static PyObject*
UringLoop_udp_socket(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (udp_socket(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringLoop_unix_stream(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (unix_stream(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringLoop_unix_dgram(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Can't create future");
        return NULL;
    }

    int opcode = IORING_OP_SOCKET;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (unix_dgram(self->ring, request_idx) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_dealloc(UringSocket *self)
{
    self->closing = true;
    if (self->py_loop) {
        Py_XDECREF(self->py_loop);
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject*
UringSocket_bind(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int fd = NULL;
    int addr = NULL;
    int addrlen = NULL;

    static char *kwlist[] = {"fd", "addr", "addrlen", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, fd, addr, addrlen))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_BIND;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (bind(self->ring, request_idx, fd, addr, (socklen_t)addrlen) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_listen(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int fd = NULL;
    int backlog = NULL;

    static char *kwlist[] = {"fd", "backlog", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, fd, backlog))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (listen(self->ring, request_idx, fd, backlog) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_connect(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int fd = NULL;
    const struct sockaddr *addr = NULL;
    int addrlen = NULL;

    static char *kwlist[] = {"fd", "addr", "addrlen", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "ioi", kwlist, fd, &addr, addrlen))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CONNECT;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (connect(self->ring, request_idx, fd, *addr, addrlen) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_send(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int sockfd = NULL;
    int len = NULL;
    int flags = NULL;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, sockfd, len, flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_SEND;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (send(self->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

static PyObject*
UringSocket_recv(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int sockfd = NULL;
    int len = NULL;
    int flags = NULL;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, sockfd, len, flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_RECV;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (recv(self->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}

static PyObject*
UringSocket_accept(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int sockfd = NULL;
    int len = NULL;
    int flags = NULL;

    static char *kwlist[] = {"sockfd", "len", "flags", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "iii", kwlist, sockfd, len, flags))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_ACCEPT;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (accept(self->ring, request_idx, sockfd, buffer, (size_t)len, flags) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


static PyObject*
UringSocket_close(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs,
)
{
    ASSERT_LOOP_THREAD(self);

    int sockfd = NULL;

    static char *kwlist[] = {"sockfd", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, sockfd))) {
        PyErr_SetString(PyExc_RuntimeError, "No required params\n");
        return NULL;
    }

    PyObject *future = create_future(loop);
    if (!future) {
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    int opcode = IORING_OP_CLOSE;
    // For now whoile puring without buffer, we'll do it in next v.
    int buffer = 0;
    int request_idx = registry_add(loop->registry, future, buffer, opcode);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is not awailable\n");
        return NULL;
    }

    if (close(self->ring, request_idx, sockfd) < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        return NULL;
    }
    return future;
}


//
/* Python adaptation*/
//
static PyTypeObject UringSocketType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.src.python_api.ops.sockets.UringSocket",
    .tp_doc = PyDoc_STR("Puring socket adapter"),
    .tp_basicsize = sizeof(UringSocket),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)UringSocket_dealloc,
    .tp_methods = uring_socket_methods,
};


// Method Registration
// Method Table
static PyMethodDef uring_socket_methods[] = {
    {"bind", (PyCFunction)UringSocket_bind, METH_NOARGS,  "Bind socket"},
    {"listen", (PyCFunction)UringSocket_listen, METH_NOARGS,  "Listen socket"},
    {"connect", (PyCFunction)UringSocket_connect, METH_NOARGS,  "Connect"},
    {"send", (PyCFunction)UringSocket_send, METH_NOARGS,  "Send"},
    {"recv", (PyCFunction)UringSocket_recv, METH_NOARGS,  "Recv"},
    {"accept", (PyCFunction)UringSocket_accept, METH_NOARGS,  "Accept"},
    {"close", (PyCFunction)UringSocket_close, METH_NOARGS,  "Close"},

    {NULL, NULL, 0, NULL}
};


// Initialization of module
static int
uring_sock_module_exec(PyObject *m)
{
    if (PyType_Ready(&UringSocketType) < 0) {
        return -1;
    }

    if (PyModule_AddObjectRef(m, "socket", (PyObject *) &UringSocketType) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot uring_sock_module_slots[] = {
    {Py_mod_exec, uring_sock_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};


static PyModuleDef uring_sock_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = 'socket',
    .m_doc = 'Module contains socket wrapped for uring',
    .m_size = 0,
    .m_slots = uring_sock_module_slots,
    .m_methods = NULL,
};

PyMODINIT_FUNC
PyInit_custom(void)
{
    return PyModuleDef_Init(&uring_sock_module);
}
