#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>


#define ASSERT_LOOP_THREAD(loop) \
    do { \
        if ((loop)->loop_tid != gettid()) { \
            PyErr_SetString(PyExc_RuntimeError, "io_uring operation called from wrong thread"); \
            return NULL; \
        } \
    } while(0)


#define ASSERT_PYTHON_THREAD(loop) \
    do { \
        PyObject *thread_id = PyObject_GetAttrString((PyObject *)self, "_thread_id");  \
        if (!thread_id)  \
            return NULL;  \
        \
        long loop_thread = PyLong_AsLong(thread_id);  \
        Py_DECREF(thread_id);  \
        \
        PyObject *threading = PyImport_ImportModule("threading");  \
        if (!threading)  \
            return NULL;  \
        \
        PyObject *ident = PyObject_CallMethod(threading, "get_ident", NULL);  \
        Py_DECREF(threading);  \
        if (!ident) \
            return NULL;  \
        \
        long current_thread = PyLong_AsLong(ident);  \
        Py_DECREF(ident);  \
        \
        if (!(loop_thread == current_thread)) {  \
            PyErr_SetString(PyExc_RuntimeError, "Event loop called from wrong thread");  \
            return NULL;  \
        }  \
    } while(0)



#define ASSERT_RING_LOOP_IS_CLOSING(loop) \
    PyObject *closed = PyObject_GetAttrString((PyObject *)loop, "_closed");  \
    bool is_closed = closed && PyObject_IsTrue(closed);  \
    Py_XDECREF(closed);  \
    if (is_closed) {  \
        PyErr_Format(  \
            PyExc_RuntimeError,  \
            "Ring Event Loop is closing - %S",  \
            loop  \
        );  \
        return NULL;  \
    }
