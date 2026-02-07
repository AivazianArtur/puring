#include "loop.h"

PyObject* 
_get_loop(void)
{
    PyObject *asyncio = PyImport_ImportModule("asyncio");
    if (!asyncio) return NULL;

    PyObject *get_loop_fn = PyObject_GetAttrString(asyncio, "get_running_loop");
    PyObject *loop = PyObject_CallNoArgs(get_loop_fn);

    Py_DECREF(get_loop_fn);
    Py_DECREF(asyncio);
    if (!loop) return NULL;
    return loop;
}


int _parse_memory_params(PyObject *obj, memory_params *out)
{
    PyObject *temp;

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "memory_params must be dict");
        return 0;
    }

    temp = PyDict_GetItemString(obj, "user_buf");
    out->user_buf = (temp && temp != Py_None) ? PyLong_AsVoidPtr(temp) : NULL;

    temp = PyDict_GetItemString(obj, "user_buf_size");
    if (!temp || !PyLong_Check(temp)) {
        PyErr_SetString(PyExc_KeyError, "user_buf_size required");
        return 0;
    }
    out->user_buf_size = PyLong_AsSize_t(temp);

    temp = PyDict_GetItemString(obj, "numa_node");
    out->numa_node = temp ? (int)PyLong_AsLong(temp) : -1;

    temp = PyDict_GetItemString(obj, "huge_pages");
    out->huge_pages = temp ? PyObject_IsTrue(temp) : false;

    temp = PyDict_GetItemString(obj, "mlock");
    out->mlock = temp ? PyObject_IsTrue(temp) : false;

    temp = PyDict_GetItemString(obj, "alignment");
    out->alignment = temp ? PyLong_AsSize_t(temp) : 0;

    temp = PyDict_GetItemString(obj, "shared");
    out->shared = temp ? PyObject_IsTrue(temp) : false;

    return 1;
}


int _parse_ring_init_params(PyObject *obj, ring_init_params *out)
{
    PyObject *temp;

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "ring_init_params must be dict");
        return 0;
    }

    temp = PyDict_GetItemString(obj, "flags");
    out->flags = temp ? (ring_flags)PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "sq_thread_cpu");
    out->sq_thread_cpu = temp ? PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "sq_thread_idle");
    out->sq_thread_idle = temp ? PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "features");
    out->features = temp ? (ring_features)PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "linked_ring");
    out->linked_ring = temp ? PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "sqring_offsets");
    if (temp && !_parse_offset(temp, &out->sqring_offsets))
        // Meybe return here not 0 but 1, we'll think
        return 0;

    temp = PyDict_GetItemString(obj, "cqring_offsets");
    if (temp && !_parse_offset(temp, &out->cqring_offsets))
        return 0;

    return 1;
}


int _parse_offset(PyObject *obj, offset *out)
{
    PyObject *temp;

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "offset must be a dict");
        return 0;
    }

    temp = PyDict_GetItemString(obj, "head");
    if (!temp || !PyLong_Check(temp)) goto type_error;
    out->head = (uint32_t)PyLong_AsUnsignedLong(temp);

    temp = PyDict_GetItemString(obj, "tail");
    if (!temp || !PyLong_Check(temp)) goto type_error;
    out->tail = (uint32_t)PyLong_AsUnsignedLong(temp);

    temp = PyDict_GetItemString(obj, "ring_mask");
    if (!temp || !PyLong_Check(temp)) goto type_error;
    out->ring_mask = (uint32_t)PyLong_AsUnsignedLong(temp);

    temp = PyDict_GetItemString(obj, "ring_entries");
    if (!temp || !PyLong_Check(temp)) goto type_error;
    out->ring_entries = (uint32_t)PyLong_AsUnsignedLong(temp);

    temp = PyDict_GetItemString(obj, "overflow");
    out->overflow = temp ? (uint32_t)PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "cqes");
    out->cqes = temp ? (uint32_t)PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "flags");
    out->flags = temp ? (uint32_t)PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "resv1");
    out->resv1 = temp ? (uint32_t)PyLong_AsUnsignedLong(temp) : 0;

    temp = PyDict_GetItemString(obj, "user_addr");
    out->user_addr = temp ? (uint32_t)PyLong_AsUnsignedLong(temp) : 0;

    return 1;

type_error:
    PyErr_SetString(PyExc_KeyError, "missing or invalid offset field");
    return 0;
}

void graceful_shutdown(io_uring* ring, RequestRegistry *reg) {
    ring_destroy(ring);
    registry_destroy(reg);

    while (io_uring_peek_cqe(&loop→ring, &cqe) == 0) {
        io_uring_cqe_seen(&loop→ring, cqe);
    }
}


void fast_shutdown(io_uring* ring, RequestRegistry *reg) {
    ring_destroy(ring);
    registry_destroy(reg);
}