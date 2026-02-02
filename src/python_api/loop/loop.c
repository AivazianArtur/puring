#include "loop.h"
#include "macroses.c"
#include "core/core.h"


static PyObject*
UringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"registry_size", NULL};
    int registry_size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &registry_size))
        registry_size = 0;

    io_uring* ring = malloc(sizeof(io_uring));
    if (!ring) {
        PyErr_NoMemory("Error while allocating memory for ring");
        return NULL;
    }

    RequestRegistry* registry = registry_new(registry_size);
    if (!uring_loop) {
        PyErr_NoMemory("Error while allocating memory for registry");
        ring_destroy(ring);
        return NULL;
    }

    UringLoop *uring_loop = (UringLoop *)PyObject_New(UringLoop, &UringLoopType);
    if (!uring_loop) {
        PyErr_NoMemory("Error while allocating memory for loop wrapper");
        ring_destroy(ring);
        registry_destroy(registry);
        return NULL;
    }

    PyObject* python_loop = _set_loop();
    if (!python_loop) {
        PyErr_SetString(PyExc_TypeError, "Error while creating loop");
        ring_destroy(ring);
        registry_destroy(registry);
        Py_TYPE(uring_loop)->tp_free((PyObject *)uring_loop);
        return NULL;
    }
    Py_INCREF(python_loop);

    uring_loop->ring = ring;
    uring_loop->registry = registry;
    uring_loop->py_loop = python_loop;
    uring_loop->initialized = false;
    uring_loop->closing = false;

    return (PyObject *)uring_loop;
}

static PyObject*
UringLoop_init(UringLoop *self, PyObject *args, PyObject *kwargs)
{
    ASSERT_LOOP_THREAD(self);

    PyObject *memory_params_obj = NULL;
    PyObject *ring_init_params_obj = NULL;

    static char *kwlist[] = {"memory_params", "ring_init_params", NULL};
    PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &memory_params_obj, &ring_init_params_obj);

    memory_params *memory_params = NULL;
    ring_init_params *params = NULL;

    _parse_memory_params(memory_params_obj, &memory_params);
    _parse_ring_init_params(ring_init_params_obj, &params);
   
    if (ring_init(&memory_params, &params) < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    self->initialized = true;
    return 0;
}


static PyObject*
UringLoop_dealloc(UringLoop *self)
{
    self->closing = true;
    if (self->py_loop) {
        Py_XDECREF(self->py_loop);
    }

    if (self->ring) {
        ring_destroy(self->ring);
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
}


//
/* Python adaptation*/
//
static PyTypeObject UringLoopType = {
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


// Method Registration
// Method Table
static PyMethodDef uring_loop_methods[] = {
    // LOOP
    {"close_loop", (PyCFunction)UringLoop_close_loop, METH_NOARGS,  "Close loop"},
    // {"run",   (PyCFunction)UringLoop_run,   METH_NOARGS,  "Run loop"},
    // {"stop",  (PyCFunction)UringLoop_stop,  METH_NOARGS,  "Stop loop"},

    // OPS
    // Files
    {"open", UringLoop_open, METH_VARARGS | METH_KEYWORDS}
    {"read", (PyCFunction)UringLoop_read, METH_NOARGS,  "Read file"},
    {"close", (PyCFunction)UringLoop_close, METH_NOARGS,  "Close file"},
    {"stat", (PyCFunction)UringLoop_stat, METH_NOARGS,  "File info"},
    {"fsync", (PyCFunction)UringLoop_fsync, METH_NOARGS,  "Flush file buffer to file"},
    // Sockets
    {"tcp_socket", (PyCFunction)UringLoop_tcp_socket, METH_VARARGS | METH_KEYWORDS,  "Opens tcp-socket"},
    {"udp_socket", (PyCFunction)UringLoop_udp_socket, METH_VARARGS | METH_KEYWORDS,  "Opens udp-socket"},
    {"stream_socket", (PyCFunction)UringLoop_unix_stream, METH_VARARGS | METH_KEYWORDS,  "Opens unix stream-socket"},
    {"dgram_socket", (PyCFunction)UringLoop_unix_dgram, METH_VARARGS | METH_KEYWORDS,  "Opens unix dgram-socket"},
    {NULL, NULL, 0, NULL}
};


// Initialization of module
static int
uring_loop_module_exec(PyObject *m)
{
    if (PyType_Ready(&UringLoopType) < 0) {
        return -1;
    }

    if (PyModule_AddObjectRef(m, "uring", (PyObject *) &UringLoopType) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot uring_loop_module_slots[] = {
    {Py_mod_exec, uring_loop_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};


static PyModuleDef uring_loop_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = 'loop',
    .m_doc = 'Module contains loop with uring',
    .m_size = 0,
    .m_slots = uring_loop_module_slots,
    .m_methods = NULL,
};

PyMODINIT_FUNC
PyInit_custom(void)
{
    return PyModuleDef_Init(&uring_loop_module);
}
