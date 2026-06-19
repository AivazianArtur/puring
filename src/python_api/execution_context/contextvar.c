#include "execution_context.h"

static PyObject *execution_context_var = NULL;

PyObject *
ContextVar_init(void) {
    PyObject *contextvars = PyImport_ImportModule("contextvars");
    if (!contextvars) {
        return NULL;
    }

    PyObject *ContextVar = PyObject_GetAttrString(contextvars, "ContextVar");
    Py_DECREF(contextvars);

    if (!ContextVar) {
        return NULL;
    }

    execution_context_var = PyObject_CallFunction(ContextVar, "s", "execution_context");

    Py_DECREF(ContextVar);

    if (!execution_context_var) {
        return NULL;
    }

    ExecutionContext *execution_context = malloc(sizeof(ExecutionContext));
    if (!execution_context) {
        PyErr_NoMemory();
        Py_DECREF(execution_context_var);
        execution_context_var = NULL;
        return NULL;
    }

    execution_context->buffer_mode = NORMAL_BUFFER;
    execution_context->stream = ONESHOT;
    execution_context->transfer_mode = NORMAL_TRANSFER;

    PyObject *capsule = PyCapsule_New(execution_context, "ExecutionContext", NULL);

    if (!capsule) {
        free(execution_context);

        Py_DECREF(execution_context_var);
        execution_context_var = NULL;

        return NULL;
    }

    PyObject *method_name = PyUnicode_FromString("set");
    if (!method_name) {
        Py_DECREF(capsule);

        Py_DECREF(execution_context_var);
        execution_context_var = NULL;

        return NULL;
    }

    PyObject *token = PyObject_CallMethodObjArgs(execution_context_var, method_name, capsule, NULL);

    Py_DECREF(method_name);
    Py_DECREF(capsule);

    if (!token) {
        Py_DECREF(execution_context_var);
        execution_context_var = NULL;
        return NULL;
    }

    Py_DECREF(token);

    return execution_context_var;
}

PyObject *
ContextVar_set(PyObject *value) {
    if (!execution_context_var) {
        PyErr_SetString(PyExc_RuntimeError, "ContextVar not initialized");
        return NULL;
    }

    PyObject *method_name = PyUnicode_FromString("set");
    if (!method_name) {
        return NULL;
    }
    PyObject *token = PyObject_CallMethodObjArgs(execution_context_var, method_name, value, NULL);
    Py_DECREF(method_name);
    if (!token) {
        return NULL;
    }

    return token;
}

int
ContextVar_reset(PyObject *token)
{
    if (!execution_context_var) {
        PyErr_SetString(PyExc_RuntimeError, "ContextVar not initialized");
        return -1;
    }
    PyObject *method_name = PyUnicode_FromString("reset");
    if (!method_name) {
        return -1;
    }
    PyObject *result = PyObject_CallMethodObjArgs(execution_context_var, method_name, token, NULL);
    Py_DECREF(method_name);
    if (!result) {
        return -1;
    }
    Py_DECREF(result);
    return 0;
}

ExecutionContext *
ContextVar_get(PyObject *default_val) {
    if (!execution_context_var) {
        PyErr_SetString(PyExc_RuntimeError, "ContextVar not initialized");
        return NULL;
    }

    PyObject *execution_context_obj;
    PyObject *method_name = PyUnicode_FromString("get");
    if (!method_name) {
        return NULL;
    }
    if (default_val) {
        execution_context_obj = PyObject_CallMethodObjArgs(execution_context_var, method_name, default_val, NULL);
    } else {
        execution_context_obj = PyObject_CallMethodNoArgs(execution_context_var, method_name);
    }
    return (ExecutionContext *)execution_context_obj;
}

void
ContextVar_dealloc(void) {
    Py_CLEAR(execution_context_var);
}
