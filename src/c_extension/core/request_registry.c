#include "registry.h"

int registry_init(RequestRegistry *reg, unsigned int size)
{
    reg->objects = calloc(size, sizeof(PyObject*));
    if (!reg->objects) return -1;
    reg->size = size;
    return 0;
}

void registry_free(RequestRegistry *reg)
{
    if (reg->objects) {
        for (unsigned int i = 0; i < reg->size; i++) {
            Py_XDECREF(reg->objects[i]);
        }
        free(reg->objects);
    }
}

// TODO: Make faster than O(1)
int registry_add(RequestRegistry *reg, PyObject *obj)
{
    for (unsigned int i = 0; i < reg->size; i++) {
        if (reg->objects[i] == NULL) {
            reg->objects[i] = obj;
            Py_INCREF(obj);
            return i;
        }
    }
    return -1;
}

PyObject* registry_get(RequestRegistry *reg, int index)
{
    if (index < 0 || index >= reg->size) return NULL;
    return reg->objects[index];
}

void registry_remove(RequestRegistry *reg, int index)
{
    if (index >= 0 && index < reg->size) {
        Py_XDECREF(reg->objects[index]);
        reg->objects[index] = NULL;
    }
}

//
/* Python adaptation*/
//
static PyTypeObject RequestRegistryType = {
    // TODO: Set properly
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.construction.registry.RequestReqistry",
    .tp_doc = PyDoc_STR("Shadow Registry of Features"),
    .tp_basicsize = sizeof(RequestRegistry),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};

// Method Registration
// Method Table
static PyMethodDef registry_methods[] = {
    {"init",  registry_init, METH_VARARGS, "Initialize the array of pointers to NULL"},
    {"free",  registry_free, METH_VARARGS, "Remove registry"},
    {"add",  registry_add, METH_VARARGS, "Find a free slot, store the object, return the index"},
    {"get",  registry_get, METH_VARARGS, "Get the object"},
    {"remove",  registry_remove, METH_VARARGS, "Clear the slot"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};



// Initialization of module
static int
registry_module_exec(PyObject *m)
{
    if (PyType_Ready(&RequestRegistryType) < 0) {
        return -1;
    }

    if (PyModule_AddObjectRef(m, "registry", (PyObject *) &RequestRegistryType) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot registry_module_slots[] = {
    {Py_mod_exec, registry_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};


static PyModuleDef registry_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "registry",
    .m_doc = "Module, that contains Shadow registry for features",
    .m_size = 0,
    .m_slots = registry_module_slots,
    .m_methods = registry_methods,  // TEMP: Maybe now its module's funcs, need to be Class methods
};

PyMODINIT_FUNC
PyInit_custom(void)
{
    return PyModuleDef_Init(&registry_module);
}
