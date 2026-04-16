#include "files.h"


static PyObject*
create_resolve_enum(void)
{
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module) return NULL;

    PyObject *IntFlag = PyObject_GetAttrString(enum_module, "IntFlag");
    Py_DECREF(enum_module);
    if (!IntFlag) return NULL;

    PyObject *members = Py_BuildValue(
        "{s:i, s:i, s:i, s:i, s:i, s:i}",
        "NO_XDEV",       RESOLVE_NO_XDEV,
        "NO_MAGICLINKS", RESOLVE_NO_MAGICLINKS,
        "NO_SYMLINKS",   RESOLVE_NO_SYMLINKS,
        "BENEATH",       RESOLVE_BENEATH,
        "IN_ROOT",       RESOLVE_IN_ROOT,
        "CACHED",        RESOLVE_CACHED
    );
    if (!members) { 
        Py_DECREF(IntFlag);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(IntFlag, "sO", "ResolveFlags", members);
    Py_DECREF(IntFlag);
    Py_DECREF(members);
    return result;
}
