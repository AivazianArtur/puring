#include "buffers.h"


IovecsResult* _serialize_iovecs_buffer(PyObject *buffers_obj) {
    PyObject *seq = PySequence_Fast(buffers_obj, "Buffers must be a sequence");
    if (!seq) {
        return NULL;
    }
    Py_ssize_t nr_vecs = PySequence_Fast_GET_SIZE(seq);
    PyObject **items = PySequence_Fast_ITEMS(seq);

    struct iovec *iovecs = PyMem_Malloc(sizeof(struct iovec) * nr_vecs);
    if (!iovecs) {
        Py_DECREF(seq);
        PyErr_NoMemory();
        return NULL;
    }

    Py_buffer *iovecs_buf = PyMem_Malloc(sizeof(Py_buffer) * nr_vecs);

    for (Py_ssize_t i = 0; i < nr_vecs; i++) {
        if (PyObject_GetBuffer(items[i], &iovecs_buf[i], PyBUF_SIMPLE) < 0) {
            for (Py_ssize_t j = 0; j < i; j++) {
                PyBuffer_Release(&iovecs_buf[j]);
            }
            PyMem_Free(iovecs_buf);
            PyMem_Free(iovecs);
            Py_DECREF(seq);
            
            return NULL;
        }

        iovecs[i].iov_base = iovecs_buf[i].buf;
        iovecs[i].iov_len  = iovecs_buf[i].len;
    }

    IovecsResult *result = malloc(sizeof(IovecsResult));
    result->nr_vecs = nr_vecs;
    result->iovecs = iovecs;
    result->iovecs_buf = iovecs_buf;
    return result;
}


BufferResult* _get_buffer(PyObject *buffer_obj, int bufsize) {
    void *buffer = NULL;
    size_t buffer_len;

    Py_buffer view;
    int buffer_flag;
    if (buffer_obj && buffer_obj != Py_None) {
        if (PyObject_GetBuffer(buffer_obj, &view, PyBUF_WRITABLE) < 0) {
            return NULL;
        }
        buffer_flag = 0;
        buffer = view.buf;
        buffer_len = view.len;
    } else {
        buffer = PyMem_Malloc(bufsize);
        if (!buffer) {
            PyErr_NoMemory();
            return NULL;
        }
        buffer_len = (size_t)bufsize;
        buffer_flag = 1;
    }

    BufferResult *result = malloc(sizeof(BufferResult));
    result->buffer = buffer;
    result->buffer_len = buffer_len;
    result->view = &view;
    result->buffer_flag = buffer_flag;
    return result;
}
