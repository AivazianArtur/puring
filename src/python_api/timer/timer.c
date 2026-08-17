#include "python_api/timer/timer.h"

PyObject *
AioUringLoop_timer(
    PyObject *Py_UNUSED(module), // cppcheck-suppress funcArgNamesDifferent
    PyObject *args,
    PyObject *kwargs
) {
    PyObject *loop_obj;
    PyObject *timer_params_obj = NULL;
    static const char *kwlist[] = {"uring_loop", "timer_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", (char **)kwlist, &loop_obj, &timer_params_obj))) {
        return NULL;
    }
    AioUringLoop *loop = (AioUringLoop *)loop_obj;
    ASSERT_LOOP_THREAD(loop);
    ASSERT_RING_LOOP_IS_CLOSING(loop);

    TimerParams timer_params = {0};
    StreamStrategy stream_strategy = get_stream_strategy();
    if (timer_params_obj && timer_params_obj != Py_None) {
        if (parse_timer_params(timer_params_obj, &timer_params, stream_strategy) < 0) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_TypeError, "timer_params requires int fields: sec, nsec, count");
            }
            return NULL;
        }
    }

    PyObject *future = create_future(loop);
    if (!future)
        return NULL;

    int opcode = IORING_OP_TIMEOUT;
    int request_idx = registry_add(loop->registry, future, NULL, ONESHOT, opcode, NULL, NULL, NULL, NULL);
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = timer(loop->ring, request_idx, &timer_params);
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(loop->registry, request_idx);
        return NULL;
    }

    return future;
}