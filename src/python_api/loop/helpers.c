#include "loop.h"

PyObject* _set_loop(void)
{
    PyObject *asyncio = PyImport_ImportModule("asyncio");
    if (!asyncio) return NULL;

    PyObject *new_loop_fn = PyObject_GetAttrString(asyncio, "new_event_loop");
    PyObject *loop = PyObject_CallNoArgs(new_loop_fn);
    Py_DECREF(new_loop_fn);
    Py_DECREF(asyncio);

    if (!loop) return NULL;

    PyObject *set_loop_fn = PyObject_GetAttrString(PyImport_ImportModule("asyncio"), "set_event_loop");
    PyObject *args = PyTuple_Pack(1, loop);
    PyObject *res = PyObject_CallObject(set_loop_fn, args);
    Py_DECREF(set_loop_fn);
    Py_DECREF(args);
    Py_XDECREF(res);

    return loop;
}


int _parse_memory_params(PyObject *obj, memory_params *out)
{
    PyObject *v;

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "memory_params must be dict");
        return 0;
    }

    v = PyDict_GetItemString(obj, "user_buf");
    out->user_buf = (v && v != Py_None) ? PyLong_AsVoidPtr(v) : NULL;

    v = PyDict_GetItemString(obj, "user_buf_size");
    if (!v || !PyLong_Check(v)) {
        PyErr_SetString(PyExc_KeyError, "user_buf_size required");
        return 0;
    }
    out->user_buf_size = PyLong_AsSize_t(v);

    v = PyDict_GetItemString(obj, "numa_node");
    out->numa_node = v ? (int)PyLong_AsLong(v) : -1;

    v = PyDict_GetItemString(obj, "huge_pages");
    out->huge_pages = v ? PyObject_IsTrue(v) : false;

    v = PyDict_GetItemString(obj, "mlock");
    out->mlock = v ? PyObject_IsTrue(v) : false;

    v = PyDict_GetItemString(obj, "alignment");
    out->alignment = v ? PyLong_AsSize_t(v) : 0;

    v = PyDict_GetItemString(obj, "shared");
    out->shared = v ? PyObject_IsTrue(v) : false;

    return 1;
}


int _parse_ring_init_params(PyObject *obj, ring_initialization_params *out)
{
    PyObject *v;

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "ring_init_params must be dict");
        return 0;
    }

    v = PyDict_GetItemString(obj, "flags");
    out->flags = v ? (ring_flags)PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "sq_thread_cpu");
    out->sq_thread_cpu = v ? PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "sq_thread_idle");
    out->sq_thread_idle = v ? PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "features");
    out->features = v ? (ring_features)PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "linked_ring");
    out->linked_ring = v ? PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "sqring_offsets");
    if (v && !_parse_offset(v, &out->sqring_offsets))
        // Meybe return here not 0 but 1, we'll think
        return 0;

    v = PyDict_GetItemString(obj, "cqring_offsets");
    if (v && !_parse_offset(v, &out->cqring_offsets))
        return 0;

    return 1;
}


int _parse_offset(PyObject *obj, offset *out)
{
    PyObject *v;

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "offset must be a dict");
        return 0;
    }

    v = PyDict_GetItemString(obj, "head");
    if (!v || !PyLong_Check(v)) goto type_error;
    out->head = (uint32_t)PyLong_AsUnsignedLong(v);

    v = PyDict_GetItemString(obj, "tail");
    if (!v || !PyLong_Check(v)) goto type_error;
    out->tail = (uint32_t)PyLong_AsUnsignedLong(v);

    v = PyDict_GetItemString(obj, "ring_mask");
    if (!v || !PyLong_Check(v)) goto type_error;
    out->ring_mask = (uint32_t)PyLong_AsUnsignedLong(v);

    v = PyDict_GetItemString(obj, "ring_entries");
    if (!v || !PyLong_Check(v)) goto type_error;
    out->ring_entries = (uint32_t)PyLong_AsUnsignedLong(v);

    v = PyDict_GetItemString(obj, "overflow");
    out->overflow = v ? (uint32_t)PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "cqes");
    out->cqes = v ? (uint32_t)PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "flags");
    out->flags = v ? (uint32_t)PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "resv1");
    out->resv1 = v ? (uint32_t)PyLong_AsUnsignedLong(v) : 0;

    v = PyDict_GetItemString(obj, "user_addr");
    out->user_addr = v ? (uint32_t)PyLong_AsUnsignedLong(v) : 0;

    return 1;

type_error:
    PyErr_SetString(PyExc_KeyError, "missing or invalid offset field");
    return 0;
}
