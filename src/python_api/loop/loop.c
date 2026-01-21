#include "loop.h"


PyObject *UringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
{

}

void UringLoop_dealloc(UringLoop *self);
{

}


int UringLoop_init(UringLoop *self, PyObject *args, PyObject *kwargs);
{
    ASSERT_LOOP_THREAD(self);
}



PyObject *UringLoop_run(UringLoop self*, PyObject *args);
{
    ASSERT_LOOP_THREAD(self);
}

PyObject *UringLoop_stop(UringLoop *self, PyObject *args);
{
    ASSERT_LOOP_THREAD(self);
}

PyObject *UringLoop_close(UringLoop *self, PyObject *args);
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
    if (PyType_Ready(&UringType) < 0) {
        return -1;
    }

    if (PyModule_AddObjectRef(m, "uring", (PyObject *) &UringType) < 0) {
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

