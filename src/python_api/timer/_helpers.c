#include "python_api/timer/timer.h"

int
parse_timer_params(PyObject *obj, TimerParams *out, StreamStrategy stream_strategy) {
    if (!obj || obj == Py_None) {
        return -1;
    }

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "timer_params must be a dict");
        return -1;
    }

    PyObject *sec_obj = PyDict_GetItemString(obj, "sec");
    PyObject *nsec_obj = PyDict_GetItemString(obj, "nsec");
    PyObject *count_obj = PyDict_GetItemString(obj, "count");
    PyObject *is_multishot_obj = PyDict_GetItemString(obj, "is_multishot");

    if ((sec_obj && PyLong_Check(sec_obj)) && (nsec_obj && PyLong_Check(nsec_obj)) &&
        (count_obj && PyLong_Check(count_obj))) {
        out->sec = (int)PyLong_AsLong(sec_obj);
        out->nsec = (int)PyLong_AsLong(nsec_obj);
        out->count = (int)PyLong_AsLong(count_obj);

        if (is_multishot_obj && PyBool_Check(is_multishot_obj)) {
            if (stream_strategy == MULTISHOT) {
                out->is_multishot = PyObject_IsTrue(is_multishot_obj);
            } else {
                out->is_multishot = false;
            }
        } else {
            out->is_multishot = false;
        }
        return 1;
    }
    PyErr_SetString(PyExc_TypeError, "timer_params requires int fields: sec, nsec, count");
    return -1;
}

int
parse_timeout_params(PyObject *obj, TimeoutParams *out) {
    if (!obj || obj == Py_None) {
        return 0;
    }

    if (!PyDict_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "timeout_params must be a dict");
        return -1;
    }

    PyObject *sec_obj = PyDict_GetItemString(obj, "sec");
    PyObject *nsec_obj = PyDict_GetItemString(obj, "nsec");
    PyObject *is_required_obj = PyDict_GetItemString(obj, "is_required");

    if (!sec_obj || !PyLong_Check(sec_obj)) {
        PyErr_SetString(PyExc_TypeError, "timeout_params['sec'] is required and must be an int");
        return -1;
    }
    if (!nsec_obj || !PyLong_Check(nsec_obj)) {
        PyErr_SetString(PyExc_TypeError, "timeout_params['nsec'] is required and must be an int");
        return -1;
    }
    if (!is_required_obj || !PyBool_Check(is_required_obj)) {
        PyErr_SetString(PyExc_TypeError, "timeout_params['is_required'] is required and must be a bool");
        return -1;
    }

    out->sec = (int)PyLong_AsLong(sec_obj);
    out->nsec = (int)PyLong_AsLong(nsec_obj);
    out->is_required = PyObject_IsTrue(is_required_obj);
    return 0;
}
