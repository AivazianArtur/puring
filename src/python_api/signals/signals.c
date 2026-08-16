#include "signals.h"

int
set_signals_handler(struct io_uring *ring) {
    int pipefd[2];
    pipe2(pipefd, O_NONBLOCK);

    PyObject *signal_module = PyImport_ImportModule("signal");
    if (!signal_module) {
        return 0;
    }
    PyObject *result = PyObject_CallMethod(signal_module, "set_wakeup_fd", "i", pipefd[1]);
    Py_DECREF(signal_module);
    if (!result)
        return 0;
    Py_DECREF(result);

    struct io_uring_sqe *sqe = create_sqe(ring);
    io_uring_prep_poll_add(sqe, pipefd[0], POLLIN);

    SignalsData *signals_data = malloc(sizeof(SignalsData));
    if (!signals_data) {
        PyErr_NoMemory();
        return 0;
    }
    signals_data->fd = pipefd[0];

    io_uring_sqe_set_data(sqe, signals_data);
    return 1;
}
