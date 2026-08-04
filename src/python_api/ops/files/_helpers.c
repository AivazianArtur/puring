#include "python_api/ops/files/files.h"

PyObject *
_raise_file_exception_group(PyObject *body_exc_type, PyObject *body_exc_val, PyObject *body_exc_tb) {
    PyObject *close_type, *close_val, *close_tb;
    PyErr_Fetch(&close_type, &close_val, &close_tb);
    PyErr_NormalizeException(&close_type, &close_val, &close_tb);
    if (close_tb) {
        PyException_SetTraceback(close_val, close_tb);
    }
    Py_XDECREF(close_type);
    Py_XDECREF(close_tb);

    if (!close_val) {
        PyErr_Restore(body_exc_type, body_exc_val, body_exc_tb);
        Py_INCREF(body_exc_type);
        Py_XINCREF(body_exc_val);
        Py_XINCREF(body_exc_tb);
        return NULL;
    }

    if (body_exc_tb && body_exc_tb != Py_None) {
        PyException_SetTraceback(body_exc_val, body_exc_tb);
    }

    PyObject *builtins = PyImport_ImportModule("builtins");
    if (!builtins) {
        Py_DECREF(close_val);
        return NULL;
    }
    PyObject *eg_type = PyObject_GetAttrString(builtins, "ExceptionGroup");
    Py_DECREF(builtins);
    if (!eg_type) {
        Py_DECREF(close_val);
        return NULL;
    }

    PyObject *exc_list = PyList_New(2);
    if (!exc_list) {
        Py_DECREF(eg_type);
        Py_DECREF(close_val);
        return NULL;
    }
    Py_INCREF(body_exc_val);
    PyList_SET_ITEM(exc_list, 0, body_exc_val);
    PyList_SET_ITEM(exc_list, 1, close_val);

    PyObject *eg = PyObject_CallFunction(
        eg_type, "sO", "socket close failed while handling another exception", exc_list
    );
    Py_DECREF(eg_type);
    Py_DECREF(exc_list);
    if (!eg)
        return NULL;

    PyErr_SetObject((PyObject *)Py_TYPE(eg), eg);
    Py_DECREF(eg);
    return NULL;
}
