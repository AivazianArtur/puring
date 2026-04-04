#include "timer/timer.h"


int parse_timer_params(PyObject *obj, TimerParams *out)
{
    if (!obj && !PyDict_Check(obj)) {
        return -1;
    }

    PyObject *sec_obj = PyDict_GetItemString(obj, "sec");
    PyObject *nsec_obj = PyDict_GetItemString(obj, "nsec");
    PyObject *count_obj = PyDict_GetItemString(obj, "count");
    PyObject *is_multishot_obj = PyDict_GetItemString(obj, "is_multishot");

    if (
        (sec_obj && PyLong_Check(sec_obj))
        && (nsec_obj && PyLong_Check(nsec_obj))
        && (count_obj && PyLong_Check(count_obj))
    ) {
        out->sec = (int)PyLong_AsLong(sec_obj);
        out->nsec = (int)PyLong_AsLong(nsec_obj);
        out->count = (int)PyLong_AsLong(count_obj);

        if (is_multishot_obj && PyBool_Check(is_multishot_obj)) {
            out->is_multishot = PyObject_IsTrue(is_multishot_obj);
        } else {
            out->is_multishot = NULL;
        }
        return 1;
    }
    return -1;
}


int parse_timeout_params(PyObject *obj, TimeoutParams *out)
{
    if (!obj && !PyDict_Check(obj)) {
        return -1;
    }

    PyObject *sec_obj = PyDict_GetItemString(obj, "sec");
    PyObject *nsec_obj = PyDict_GetItemString(obj, "nsec");
    PyObject *is_required_obj = PyDict_GetItemString(obj, "is_required");

    if (
        (sec_obj && PyLong_Check(sec_obj))
        && (nsec_obj && PyLong_Check(nsec_obj))
        && (is_required_obj && PyBool_Check(is_required_obj))
    )
    {
        out->sec = (int)PyLong_AsLong(sec_obj);
        out->nsec = (int)PyLong_AsLong(nsec_obj);
        out->is_required = PyObject_IsTrue(is_required_obj);
    }

    return 1;
}

