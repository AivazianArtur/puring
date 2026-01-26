#include "loop.h"
#include "macroses.c"
#include "core/core.h"


static *PyObject
PyObject *UringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"registry_size", NULL};
    int registry_size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &registry_size))
        registry_size = NULL;

    io_uring* ring = ring_new();
    if (!uring_loop) {
        perror("Error while allocating memory for ring");
        return NULL;
    }

    RequestRegistry* registry = registry_new(registry_size);
    if (!uring_loop) {
        perror("Error while allocating memory for registry");
        ring_destroy(void);
        return NULL;
    }

    UringLoop *uring_loop = (UringLoop *)PyObject_New(UringLoop, &UringLoopType);
    if (!uring_loop) {
        perror("Error while allocating memory for loop wrapper");
        ring_destroy(void);
        registry_destroy(registry);
        return NULL;
    }

    uring_loop->ring = ring;
    uring_loop->registry = registry;
    uring_loop->initializaed = false;
    uring_loop->closing = false;

    return (PyObject *)uring_loop;
}


void UringLoop_dealloc(UringLoop *self)
{
    Py_TYPE(self)->tp_free((PyObject *)self);
    self->py_loop = loop;
    Py_DECREF(loop);
}


int UringLoop_init(UringLoop *self, PyObject *args, PyObject *kwargs)
{
    ///
    // self->py_loop = loop;
    // Py_INCREF(loop);
    ///
    ASSERT_LOOP_THREAD(self);

    memory_params memory_params = NULL;
    ring_initialization_params *params = NULL,

    static char *kwlist[] = {"memory_params", "ring_initialization_parans", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Ii", kwlist, &memory_params, &ring_initialization_params))
        return -1;

    // TODO: 1 - Serialize args
    // TODO: 2 - Initialize the Shadow Registry, if err - PyErr_NoMemory();
    // TODO: 3 - Route initialization

    if (memory_params) {
        // TODO
        if (!params) {
            params = {0}
        }
        result = io_uring_queue_init_mem(),  // TODO: Good name, not `resutlt`
    }
    else if (params) {
        // TODO
        result = io_uring_queue_init_params(8, ring, params);
    }
    else {
        result = io_uring_queue_init(0, ring, 0);
    }
    
    if (result < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    self->initialized = true;
    return result; // TEMP: OR return 0?
}


PyObject *UringLoop_run(UringLoop self*, PyObject *args)
{
    ASSERT_LOOP_THREAD(self);

    // io_uring_wait_cqe
    // registry_get
    // io_uring_cqe_seen();
    // Future.set_result();
}

PyObject *UringLoop_stop(UringLoop *self, PyObject *args)
{
    ASSERT_LOOP_THREAD(self);
}

PyObject *UringLoop_close(UringLoop *self, PyObject *args)
{
    ASSERT_LOOP_THREAD(self);
}


//
/* Python adaptation*/
//
static PyTypeObject UringLoopType = {
    // TODO: Set properly
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.src.python_api.loop.UringLoop",
    .tp_doc = PyDoc_STR("Rings with python loop"),
    .tp_basicsize = sizeof(UringLoop),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = UringLoop_new,
    .tp_init = (initproc)UringLoop_init,
    .tp_dealloc = (destructor)UringLoop_dealloc,
};


// Method Registration
// Method Table
static PyMethodDef uring_loop_methods[] = {
    {'new',   uring_new,   METH_VARARGS, 'Create new uring'},
    {'init',  uring_init,  METH_VARARGS, 'Init uring'},
    {'run',   uring_run, METH_VARARGS, 'Run loop with uring'},
    {'stop',   uring_stop, METH_VARARGS, 'Stop loop with uring'},
    {'close',   uring_close, METH_VARARGS, 'Close loop with uring'},
    {NULL, NULL, 0, NULL}        /* Sentinel */
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
    .m_methods = uring_loop_methods,  // TEMP: Maybe now its module's funcs, need to be Class methods
};

PyMODINIT_FUNC
PyInit_custom(void)
{
    return PyModuleDef_Init(&uring_loop_module);
}
