#include "python_api/execution_context/execution_context.h"

PyObject *
create_buffer_mode_enum(void) {
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module)
        return NULL;

    PyObject *StrFlag = PyObject_GetAttrString(enum_module, "StrFlag");
    Py_DECREF(enum_module);
    if (!StrFlag)
        return NULL;

    PyObject *members = Py_BuildValue(
        "{s:s, s:s, s:s}", "NORMAL", NORMAL_BUFFER, FIXED, FIXED, PROVIDED, PROVIDED
    );
    if (!members) {
        Py_DECREF(StrFlag);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(StrFlag, "sO", "BUFFER_MODE", members);
    Py_DECREF(StrFlag);
    Py_DECREF(members);
    return result;
}

PyObject *
create_stream_strategy_enum(void) {
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module)
        return NULL;

    PyObject *StrFlag = PyObject_GetAttrString(enum_module, "StrFlag");
    Py_DECREF(enum_module);
    if (!StrFlag)
        return NULL;

    PyObject *members = Py_BuildValue("{s:s, s:s}", ONESHOT, ONESHOT, MULTISHOT, MULTISHOT);
    if (!members) {
        Py_DECREF(StrFlag);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(StrFlag, "sO", "STREAM_STRATEGY", members);
    Py_DECREF(StrFlag);
    Py_DECREF(members);
    return result;
}

PyObject *
create_transfer_mode_enum(void) {
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module)
        return NULL;

    PyObject *StrFlag = PyObject_GetAttrString(enum_module, "StrFlag");
    Py_DECREF(enum_module);
    if (!StrFlag)
        return NULL;

    PyObject *members = Py_BuildValue(
        "{s:s, s:s, s:s}", "NORMAL", NORMAL_TRANSFER, ZERO_COPY, ZERO_COPY, BUFFER_POOL, BUFFER_POOL
    );
    if (!members) {
        Py_DECREF(StrFlag);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(StrFlag, "sO", "TRANSFER_MODE", members);
    Py_DECREF(StrFlag);
    Py_DECREF(members);
    return result;
}