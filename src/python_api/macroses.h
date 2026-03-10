#pragma once


#define ASSERT_LOOP_THREAD(py_loop) do { \
    PyObject *is_same_thread = PyObject_CallMethod((PyObject *)py_loop, "_check_thread", NULL); \
    if (!is_same_thread) { \
        return NULL; \
    } \
    Py_DECREF(is_same_thread); \
} while (0)


#define GIT_RELEASER() // TODO
