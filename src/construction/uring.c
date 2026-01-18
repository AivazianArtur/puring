#include "liburing.h"
#include "uring.h"


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
    /**
        TODO: Write description
    */

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
