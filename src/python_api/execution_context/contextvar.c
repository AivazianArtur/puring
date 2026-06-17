#include "execution_context.h"

static PyObject *execution_context_var = NULL;

PyObject *
ContextVar_init(void) {
    PyObject *contextvars = PyImport_ImportModule("contextvars");
    if (!contextvars) {
        PyErr_SetString(PyExc_RuntimeError, "Error while setting initital contextvar - contextvars import");
        return NULL;
    }

    PyObject *ContextVar = PyObject_GetAttrString(contextvars, "ContextVar");
    Py_DECREF(contextvars);
    if (!ContextVar) {
        PyErr_SetString(PyExc_RuntimeError, "Error while setting initital contextvar - ContextVar");
        return NULL;
    }

    execution_context_var = PyObject_CallFunction(ContextVar, "s", "execution_context");
    Py_DECREF(ContextVar);

    if (!execution_context_var) {
        PyErr_SetString(PyExc_RuntimeError, "Error while setting initital contextvar - execution_context_var");
        return NULL;
    }

    ExecutionContext *execution_context = malloc(sizeof(ExecutionContext));
    if (!execution_context) {
        PyErr_NoMemory();
        return NULL;
    }
    execution_context->buffer_mode = NORMAL_BUFFER;
    execution_context->stream = ONESHOT;
    execution_context->transfer_mode = NORMAL_TRANSFER;

    PyObject const *token = PyObject_CallMethodObjArgs(
        execution_context_var, PyUnicode_FromString("set"), execution_context, NULL
    );
    if (!token) {
        PyErr_SetString(PyExc_ValueError, "Error while setting value for ContextVar");
        return NULL;
    }

    return execution_context_var;
}

int
ContextVar_set(PyObject *value) {
    if (!execution_context_var) {
        PyErr_SetString(PyExc_RuntimeError, "ContextVar not initialized");
        return -1;
    }

    PyObject *token = PyObject_CallMethodObjArgs(execution_context_var, PyUnicode_FromString("set"), value, NULL);
    if (!token) {
        return -1;
    }

    Py_DECREF(token);
    return 0;
}

ExecutionContext *
ContextVar_get(PyObject *default_val) {
    if (!execution_context_var) {
        PyErr_SetString(PyExc_RuntimeError, "ContextVar not initialized");
        return NULL;
    }

    PyObject *execution_context_obj;
    if (default_val) {
        execution_context_obj = PyObject_CallMethodObjArgs(
            execution_context_var, PyUnicode_FromString("get"), default_val, NULL
        );
    } else {
        execution_context_obj = PyObject_CallMethodNoArgs(execution_context_var, PyUnicode_FromString("get"));
    }
    return (ExecutionContext *)execution_context_obj;
}

void
ContextVar_dealloc(void) {
    Py_CLEAR(execution_context_var);
}
