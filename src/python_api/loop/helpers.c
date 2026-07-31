#include "loop.h"

void
fast_shutdown(struct io_uring *ring, RequestRegistry *reg) {
    ring_destroy(ring);
    registry_destroy(reg);
}

void
graceful_shutdown(struct io_uring *ring, RequestRegistry *reg) {
    struct io_uring_cqe *cqe;
    struct __kernel_timespec ts;

    ts.tv_sec = 0;
    ts.tv_nsec = 100 * 1000 * 1000; // 100ms

    while (io_uring_wait_cqe_timeout(ring, &cqe, &ts) == 0) {
        if (cqe->user_data == WAKEUP_FD_TAG) {
            io_uring_cqe_seen(ring, cqe);
            continue;
        }
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(reg, index);
        if (!slot) {
            io_uring_cqe_seen(ring, cqe);
            continue;
        }
        PyObject *future = slot->future;
        if (future) {
            PyObject *exc = PyObject_CallFunction(PyExc_RuntimeError, "s", "Event loop is closed");

            if (exc) {
                PyObject_CallMethod(future, "set_exception", "O", exc);
                Py_DECREF(exc);
            }
        }
        registry_remove(reg, index);
        io_uring_cqe_seen(ring, cqe);
    }
    registry_destroy(reg);
    PyObject *token = ContextVar_set(Py_None);
    if (token) {
        Py_DECREF(token);
    } else {
        PyErr_Clear();
    }
    ring_destroy(ring);
}

struct __kernel_timespec
compute_timeout(PuringLoop *self) {
    struct __kernel_timespec ts = {0, 0};

    PyObject *ready = PyObject_GetAttrString((PyObject *)self, "_ready");
    PyObject *scheduled = PyObject_GetAttrString((PyObject *)self, "_scheduled");
    PyObject *stopping = PyObject_GetAttrString((PyObject *)self, "_stopping");

    if (!ready || !scheduled || !stopping) {
        Py_XDECREF(ready);
        Py_XDECREF(scheduled);
        Py_XDECREF(stopping);
        return ts;
    }

    bool has_ready = PySequence_Length(ready) > 0;
    bool is_stopping = PyObject_IsTrue(stopping);

    if (has_ready || is_stopping) {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
    } else if (PySequence_Length(scheduled) > 0) {
        PyObject *first = PySequence_GetItem(scheduled, 0);
        if (first) {
            PyObject *when = PyObject_GetAttrString(first, "_when");
            PyObject *loop_time = PyObject_CallMethod((PyObject *)self, "time", NULL);
            if (when && loop_time) {
                double delay = PyFloat_AsDouble(when) - PyFloat_AsDouble(loop_time);
                if (delay < 0) {
                    delay = 0;
                }
                if (delay > 86400) {
                    delay = 86400;
                }
                ts.tv_sec = (long)delay;
                ts.tv_nsec = (long)((delay - (double)delay) * 1e9);
            }
            Py_XDECREF(when);
            Py_XDECREF(loop_time);
            Py_DECREF(first);
        }
    } else {
        ts.tv_sec = 86400;
        ts.tv_nsec = 0;
    }

    Py_DECREF(ready);
    Py_DECREF(scheduled);
    Py_DECREF(stopping);
    return ts;
}

void
promote_scheduled(PuringLoop *self) {
    PyObject *heapq = PyImport_ImportModule("heapq");
    if (!heapq) {
        return;
    }

    PyObject *scheduled = PyObject_GetAttrString((PyObject *)self, "_scheduled");
    PyObject *ready = PyObject_GetAttrString((PyObject *)self, "_ready");
    PyObject *loop_time = PyObject_CallMethod((PyObject *)self, "time", NULL);
    PyObject *clock_res = PyObject_GetAttrString((PyObject *)self, "_clock_resolution");

    if (!scheduled || !ready || !loop_time || !clock_res) {
        Py_XDECREF(scheduled);
        Py_XDECREF(ready);
        Py_XDECREF(loop_time);
        Py_XDECREF(clock_res);
        Py_DECREF(heapq);
        return;
    }

    double end_time = PyFloat_AsDouble(loop_time) + PyFloat_AsDouble(clock_res);
    while (PySequence_Length(scheduled) > 0) {
        PyObject *handle = PySequence_GetItem(scheduled, 0);
        if (!handle)
            break;

        PyObject *when = PyObject_GetAttrString(handle, "_when");
        if (!when) {
            Py_DECREF(handle);
            break;
        }

        if (PyFloat_AsDouble(when) >= end_time) {
            Py_DECREF(when);
            Py_DECREF(handle);
            break;
        }
        Py_DECREF(when);

        PyObject *popped = PyObject_CallMethod(heapq, "heappop", "O", scheduled);
        if (!popped) {
            Py_DECREF(handle);
            break;
        }

        PyObject_SetAttrString(popped, "_scheduled", Py_False);

        PyObject *res = PyObject_CallMethod(ready, "append", "O", popped);
        Py_XDECREF(res);
        Py_DECREF(popped);
        Py_DECREF(handle);
    }
}

void
drain_ready(PuringLoop *self) {
    PyObject *ready = PyObject_GetAttrString((PyObject *)self, "_ready");
    if (!ready)
        return;

    Py_ssize_t ntodo = PySequence_Length(ready);
    for (Py_ssize_t i = 0; i < ntodo; i++) {
        PyObject *handle = PyObject_CallMethod(ready, "popleft", NULL);
        if (!handle)
            break;

        PyObject *cancelled = PyObject_GetAttrString(handle, "_cancelled");
        if (cancelled && !PyObject_IsTrue(cancelled)) {
            PyObject *res = PyObject_CallMethod(handle, "_run", NULL);
            Py_XDECREF(res);
        }
        Py_XDECREF(cancelled);
        Py_DECREF(handle);
    }

    Py_DECREF(ready);
}
