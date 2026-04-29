#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <fcntl.h>


typedef struct IovecsResult {
    struct iovec *iovecs;
    Py_ssize_t nr_vecs;
    Py_buffer *iovecs_buf;
} IovecsResult;


typedef struct BufferResult {
    void *buffer;
    size_t buffer_len;
    Py_buffer *view;
    int buffer_flag;
} BufferResult;


IovecsResult* _serialize_iovecs_buffer(PyObject *buffers_obj);
BufferResult* _get_buffer(PyObject *buffer_obj, int bufsize);
