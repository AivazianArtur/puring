#include "liburing.h"
#include "uring.h"
#include "registry.h"


PyObject* uring_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    Uring *self = (Uring *) type->tp_alloc(type, 0);
    
    if (self != NULL) {
        self->initialized = false;
        
        memset(&self->ring, 0, sizeof(struct io_uring));
        
        self->registry.objects = NULL;
        self->registry.size = 0;
    }
    
    return (PyObject *) self;  //  <--TypeCasting
}


int uring_init(Uring *self, PyObject *args, PyObject *kwds) {
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

void uring_close(Uring *self) {
    // io_uring_queue_exit(ring);
}


//
/* Python adaptation*/
//
static PyTypeObject UringType = {
    // TODO: Set properly
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.construction.uring.Uring",
    .tp_doc = PyDoc_STR("Main Uring object"),
    .tp_basicsize = sizeof(Uring),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};


// Method Registration
// Method Table
static PyMethodDef uring_methods[] = {
    {"init",  uring_new, METH_VARARGS, "Create new uring"},
    {"free",  uring_init, METH_VARARGS, "Init uring"},
    {"add",  uring_close, METH_VARARGS, "Close uring"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


// Initialization of module
static int
uring_module_exec(PyObject *m)
{
    if (PyType_Ready(&UringType) < 0) {
        return -1;
    }

    if (PyModule_AddObjectRef(m, "uring", (PyObject *) &UringType) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot uring_module_slots[] = {
    {Py_mod_exec, uring_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};


static PyModuleDef uring_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "uring",
    .m_doc = "Module with main uring object",
    .m_size = 0,
    .m_slots = uring_module_slots,
    .m_methods = uring_methods,  // TEMP: Maybe now its module's funcs, need to be Class methods
};

PyMODINIT_FUNC
PyInit_custom(void)
{
    return PyModuleDef_Init(&uring_module);
}
