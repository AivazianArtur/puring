#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>


#define ASSERT_LOOP_THREAD(py_loop) do { \
    PyObject *is_same_thread = PyObject_CallMethod((PyObject *)py_loop, "_check_thread", NULL); \
    if (!is_same_thread) { \
        return NULL; \
    } \
    Py_DECREF(is_same_thread); \
} while (0)


#define ASSERT_RING_LOOP_IS_CLOSING(uring_loop) \
    if (uring_loop->is_closing) {  \
        PyErr_Format(  \
            PyExc_RuntimeError,  \
            "Ring Event Loop is closing - %S",  \
            uring_loop  \
        );  \
        return NULL;  \
    }
