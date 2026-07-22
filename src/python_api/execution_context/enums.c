#include "python_api/execution_context/execution_context.h"

PyObject *
create_buffer_mode_enum(void) {
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module)
        return NULL;

    PyObject *IntEnum = PyObject_GetAttrString(enum_module, "IntEnum");
    Py_DECREF(enum_module);
    if (!IntEnum)
        return NULL;

    PyObject *members = Py_BuildValue(
        "{s:i, s:i, s:i, s:i}", "NORMAL", NORMAL_BUF, "FIXED", FIXED, "PROVIDED", PROVIDED, "BUF_RING", BUF_RING
    );
    if (!members) {
        Py_DECREF(IntEnum);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(IntEnum, "sO", "BUFFER_MODE", members);
    Py_DECREF(IntEnum);
    Py_DECREF(members);
    return result;
}

PyObject *
create_stream_strategy_enum(void) {
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module)
        return NULL;

    PyObject *IntEnum = PyObject_GetAttrString(enum_module, "IntEnum");
    Py_DECREF(enum_module);
    if (!IntEnum)
        return NULL;

    PyObject *members = Py_BuildValue("{s:i, s:i}", "ONESHOT", ONESHOT, "MULTISHOT", MULTISHOT);
    if (!members) {
        Py_DECREF(IntEnum);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(IntEnum, "sO", "STREAM_STRATEGY", members);
    Py_DECREF(IntEnum);
    Py_DECREF(members);
    return result;
}

PyObject *
create_transfer_mode_enum(void) {
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module)
        return NULL;

    PyObject *IntEnum = PyObject_GetAttrString(enum_module, "IntEnum");
    Py_DECREF(enum_module);
    if (!IntEnum)
        return NULL;

    PyObject *members = Py_BuildValue("{s:i, s:i}", "NORMAL", NORMAL_TRANSFER, "ZERO_COPY", ZERO_COPY);
    if (!members) {
        Py_DECREF(IntEnum);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(IntEnum, "sO", "TRANSFER_MODE", members);
    Py_DECREF(IntEnum);
    Py_DECREF(members);
    return result;
}