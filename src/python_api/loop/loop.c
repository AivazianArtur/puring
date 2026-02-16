#include "loop.h"


static PyObject*
UringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"registry_size", NULL};
    int registry_size = 0;
    // TODO add loop_tid;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &registry_size))
        return NULL;

    RequestRegistry* registry = registry_new(registry_size);
    if (!registry) {
        PyErr_NoMemory();
        return NULL;
    }

    UringLoop *self = (UringLoop *)type->tp_alloc(type, 0);
    if (!self) {
        PyErr_NoMemory();
        registry_destroy(registry);
        return NULL;
    }

    struct io_uring *uring = calloc(1, sizeof(struct io_uring));
    if (!uring){
        PyErr_SetString(PyExc_TypeError, "Error while creating loop");
        registry_destroy(registry);
        Py_TYPE(self)->tp_free((PyObject *)self);
        return NULL;
    }

    PyObject* python_loop = _get_loop();
    if (!python_loop) {
        PyErr_SetString(PyExc_TypeError, "Error while creating loop");
        registry_destroy(registry);
        free(uring);
        Py_TYPE(self)->tp_free((PyObject *)self);
        return NULL;
    }
    Py_INCREF(python_loop);

    self->ring = uring;
    self->registry = registry;
    self->py_loop = python_loop;
    PyObject *capsule = PyCapsule_New(self, "uring_loop", NULL);

    self->initialized = false;
    self->is_closing = false;

    return (PyObject *)self;
}

static int
UringLoop_init(UringLoop *self, PyObject *args, PyObject *kwargs)
{
    ASSERT_LOOP_THREAD(self);

    // PyObject *memory_params_obj = NULL;
    // PyObject *ring_init_params_obj = NULL;

    memory_params memory_params = {0};
    ring_init_params params = {0};

    // static char *kwlist[] = {"memory_params", "ring_init_params", NULL};
    // if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", kwlist, &memory_params_obj, &ring_init_params_obj))
    //     return -1;

    // if (!_parse_memory_params(memory_params_obj, &memory_params))
    //     return -1;

    // if (!_parse_ring_init_params(ring_init_params_obj, &params))
    //     return -1;

    if (ring_init(&memory_params, &params, self->ring) < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    self->initialized = true;
    return 0;
}


static void
UringLoop_dealloc(UringLoop *self)
{
    if (self->py_loop) {
        Py_XDECREF(self->py_loop);
    }

    if (self->ring) {
        ring_destroy(self->ring);
        free(self->ring);
    }
    if (self->registry) {
        registry_destroy(self->registry);
    }

    Py_TYPE(self)->tp_free((PyObject *)self);
}


PyObject*
UringLoop_close_loop(UringLoop *self, PyObject *args)
{
    ASSERT_LOOP_THREAD(self);

    if (self->is_closing)
        Py_RETURN_NONE;

    self->is_closing = true;

    PyObject_CallMethod(self->py_loop, "remove_reader", "i", self->ring->ring_fd);

    // TODO: Somehow to move to ring_destoy and registry_destroy
    struct io_uring_cqe *cqe;
    while (io_uring_peek_cqe(self->ring, &cqe) == 0) {
        int index = (int)(uintptr_t)cqe->user_data;
        registry_remove(self->registry, index);  // TODO: set future exception
        io_uring_cqe_seen(self->ring, cqe);
    }

    ring_destroy(self->ring);
    self->ring = NULL;

    registry_destroy(self->registry);
    self->registry = NULL;

    Py_RETURN_NONE;
}


static PyObject *
py_uring_loop_register_fd(PyObject *self, PyObject *args)
{
    PyObject *py_loop;

    if (!PyArg_ParseTuple(args, "O", &py_loop)) {
        PyErr_SetString(PyExc_ValueError, "Invalid input arguments");
        return NULL;
    }

    UringLoop *loop = (UringLoop *)py_loop;
    if (!loop) {
        PyErr_SetString(PyExc_ValueError, "Invalid UringLoop object");
        return NULL;
    }

    uring_loop_register_fd(loop);
    Py_RETURN_NONE;
}


// In next versions
// PyObject*
// UringLoop_run_forever(UringLoop *self, PyObject *args)
// {
//     ASSERT_LOOP_THREAD(self);
// }

// PyObject*
// UringLoop_stop(UringLoop *self, PyObject *args)
// {
//     ASSERT_LOOP_THREAD(self);
// }

// PyObject*
// UringLoop_call_soon(UringLoop *self, PyObject *args)
// {
//     ASSERT_LOOP_THREAD(self);
// }

// PyObject*
// UringLoop_call_later(UringLoop *self, PyObject *args)
// {
//     ASSERT_LOOP_THREAD(self);
// }


//
/* Python adaptation*/
//


// Method Registration
// Method Table
static PyMethodDef uring_loop_methods[] = {
    // LOOP
    {"close_loop", (PyCFunction)UringLoop_close_loop, METH_VARARGS,  "Close loop"},
    // {"run",   (PyCFunction)UringLoop_run,   METH_NOARGS,  "Run loop"},
    // {"stop",  (PyCFunction)UringLoop_stop,  METH_NOARGS,  "Stop loop"},

    // OPS
    // Files
    {"open", (PyCFunction)UringLoop_open, METH_VARARGS | METH_KEYWORDS},
    {"read", (PyCFunction)UringLoop_read, METH_VARARGS | METH_KEYWORDS,  "Read file"},
    {"write", (PyCFunction)UringLoop_write, METH_VARARGS | METH_KEYWORDS, "Write file"},
    {"close", (PyCFunction)UringLoop_close, METH_VARARGS | METH_KEYWORDS,  "Close file"},
    {"stat", (PyCFunction)UringLoop_stat, METH_VARARGS | METH_KEYWORDS,  "File info"},
    {"fsync", (PyCFunction)UringLoop_fsync, METH_VARARGS | METH_KEYWORDS,  "Flush file buffer to file"},
    // Sockets
    {"tcp_socket", (PyCFunction)UringLoop_tcp_socket, METH_VARARGS | METH_KEYWORDS,  "Opens tcp-socket"},
    {"udp_socket", (PyCFunction)UringLoop_udp_socket, METH_VARARGS | METH_KEYWORDS,  "Opens udp-socket"},
    {"stream_socket", (PyCFunction)UringLoop_unix_stream, METH_VARARGS | METH_KEYWORDS,  "Opens unix stream-socket"},
    {"dgram_socket", (PyCFunction)UringLoop_unix_dgram, METH_VARARGS | METH_KEYWORDS,  "Opens unix dgram-socket"},
    {NULL, NULL, 0, NULL}
};


static PyMethodDef uring_socket_methods[] = {
    // TEMP: IN bind Always hitting OSE22 on WSL, seems like some socket ops not supported on WSL2. Later i'll try on proper setting to debug. 
    {"bind", (PyCFunction)UringSocket_bind, METH_VARARGS | METH_KEYWORDS,  "Bind socket"},
    {"listen", (PyCFunction)UringSocket_listen, METH_VARARGS | METH_KEYWORDS,  "Listen socket"},
    {"connect", (PyCFunction)UringSocket_connect, METH_VARARGS | METH_KEYWORDS,  "Connect"},
    {"send", (PyCFunction)UringSocket_send, METH_VARARGS | METH_KEYWORDS,  "Send"},
    {"recv", (PyCFunction)UringSocket_recv, METH_VARARGS | METH_KEYWORDS,  "Recv"},
    {"accept", (PyCFunction)UringSocket_accept, METH_VARARGS | METH_KEYWORDS,  "Accept"},
    {"close", (PyCFunction)UringSocket_close, METH_VARARGS | METH_KEYWORDS,  "Close"},

    {NULL, NULL, 0, NULL}
};

PyTypeObject UringLoopType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.src.python_api.loop.UringLoop",
    .tp_doc = PyDoc_STR("Rings with python loop"),
    .tp_basicsize = sizeof(UringLoop),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = UringLoop_new,
    .tp_init = (initproc)UringLoop_init,
    .tp_dealloc = (destructor)UringLoop_dealloc,
    .tp_methods = uring_loop_methods,
};


PyTypeObject UringSocketType = {
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


// Initialization of module
static int
uring_loop_module_exec(PyObject *m)
{
    if (PyType_Ready(&UringLoopType) < 0) {
        return -1;
    }
    if (PyType_Ready(&UringSocketType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "uring", (PyObject *) &UringLoopType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "socket", (PyObject *) &UringSocketType) < 0) {
        return -1;
    }
    return 0;
}


static PyModuleDef_Slot uring_loop_module_slots[] = {
    {Py_mod_exec, uring_loop_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};

static PyMethodDef uring_methods[] = {
    {"add_uring_reader", py_uring_loop_register_fd, METH_VARARGS, "Register FD with UringLoop"},
    {NULL, NULL, 0, NULL}
};


static PyModuleDef uring_loop_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "loop",
    .m_doc = "Module contains loop with uring",
    .m_size = 0,
    .m_slots = uring_loop_module_slots,
    .m_methods = uring_methods,
};

PyMODINIT_FUNC
PyInit_puring(void)
{
    return PyModuleDef_Init(&uring_loop_module);
}
