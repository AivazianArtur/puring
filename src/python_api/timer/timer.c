#include "python_api/timer/timer.h"


PyObject*
UringLoop_timer(
    PyObject *self,
    PyObject *args,
    PyObject *kwargs
)
{
    PyObject *loop_obj;
    PyObject *timer_params_obj = NULL;
    static char *kwlist[] = {"uring_loop", "timer_params", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &loop_obj, &timer_params_obj))) {
        return NULL;
    }
    UringLoop *loop = (UringLoop *)loop_obj;

    ASSERT_LOOP_THREAD(loop->py_loop);
    if (loop->is_closing) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Ring Event Loop is closing - %S",
            loop->py_loop
        );
        return NULL;
    }

    TimerParams timer_params = {0};
    parse_timer_params(timer_params_obj, &timer_params);

    PyObject *future = create_future(loop);
    if (!future) {
        return NULL;
    }

    int opcode = IORING_OP_TIMEOUT;
    
    // For now whoile puring without buffer, we'll do it in next v.
    PyObject *buffer = NULL;
    int request_idx = registry_add(
        loop->registry,
        future,
        buffer,
        NULL,
        opcode,
        NULL,
        NULL
    );
    if (request_idx < 0) {
        Py_DECREF(future);
        PyErr_SetString(PyExc_RuntimeError, "Registry is full");
        return NULL;
    }

    int result = timer(loop->ring, &timer_params);
    if (result < 0) {   
        return NULL;
    }

    return PyLong_FromLong(1);
}