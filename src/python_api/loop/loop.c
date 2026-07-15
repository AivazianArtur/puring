#include "loop.h"

PyObject *
PuringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    int registry_size = 0;

    static const char *kwlist[] = {"registry_size", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", (char **)kwlist, &registry_size))
        return NULL;

    RequestRegistry *registry = registry_new((unsigned int)(registry_size));
    if (!registry)
        return PyErr_NoMemory();

    PuringLoop *self = (PuringLoop *)type->tp_alloc(type, 0);
    if (!self) {
        registry_destroy(registry);
        return PyErr_NoMemory();
    }

    self->ring = calloc(1, sizeof(struct io_uring));
    if (!self->ring) {
        registry_destroy(registry);
        Py_TYPE(self)->tp_free((PyObject *)self);
        return PyErr_NoMemory();
    }

    self->registry = registry;
    self->initialized = false;

    return (PyObject *)self;
}

int
PuringLoop_init(
    PuringLoop *self
    // PyObject *args,
    // PyObject *kwargs
) {
    PyObject *base = (PyObject *)Py_TYPE(self)->tp_base;
    PyObject *init = PyObject_GetAttrString(base, "__init__");
    if (!init)
        return -1;

    PyObject *res = PyObject_CallOneArg(init, (PyObject *)self);
    Py_DECREF(init);
    if (!res)
        return -1;

    Py_DECREF(res);

    // memory_params mem_par = {0};
    // ring_init_params params = {0};

    int ret = ring_init(
        // &mem_par,
        // &params,
        self->ring
    );
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    set_signals_handler(self->ring);

    self->readers = PyDict_New();
    self->writers = PyDict_New();

    self->wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (self->wakeup_fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    self->wakeup_buf = 0;

    if (ring_prep_wakeup_read(self->ring, self->wakeup_fd, &self->wakeup_buf) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to register wakeup fd");
        return -1;
    }

    self->loop_tid = gettid();
    PyObject *contextvar = ContextVar_init();
    if (!contextvar) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to register required contextvar");
        return -1;
    }
    self->execution_context_var = contextvar;
    self->initialized = true;
    return 0;
}

void
PuringLoop_dealloc(PuringLoop *self) {
    Py_CLEAR(self->readers);
    Py_CLEAR(self->writers);

    if (self->registry) {
        registry_destroy(self->registry);
    }

    if (self->wakeup_fd >= 0) {
        close(self->wakeup_fd);
        self->wakeup_fd = -1;
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

PyObject *
PuringLoop_close(PuringLoop *self, PyObject *Py_UNUSED(ignored)) {
    ASSERT_LOOP_THREAD(self);
    if (self->ring == NULL)
        Py_RETURN_NONE;

    PyObject *base = (PyObject *)Py_TYPE(self)->tp_base;

    PyObject *res = PyObject_CallMethod(base, "close", "O", (PyObject *)self);

    if (!res)
        return NULL;

    Py_DECREF(res);

    graceful_shutdown(self->ring, self->registry);
    self->ring = NULL;
    self->registry = NULL;

    self->execution_context_var = NULL;
    ContextVar_dealloc();

    Py_RETURN_NONE;
}

PyObject *
PuringLoop_run_once(PuringLoop *self) {
    ASSERT_PYTHON_THREAD(self);
    struct __kernel_timespec ts = compute_timeout(self);

    struct io_uring_cqe *cqe;
    io_uring_submit_and_wait_timeout(self->ring, &cqe, 1, &ts, NULL);

    on_uring_ready(self);

    promote_scheduled(self);
    drain_ready(self);
    Py_RETURN_NONE;
}

PyObject *
PuringLoop_write_to_self(PuringLoop *self) {
    if (self->wakeup_fd < 0) {
        PyErr_SetString(PyExc_RuntimeError, "wakeup_fd is not initialized");
        return NULL;
    }

    uint64_t val = 1;
    ssize_t ret = write(self->wakeup_fd, &val, sizeof(val));
    if (ret < 0 && errno != EAGAIN) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    } // cppcheck-suppress missingReturn

    Py_RETURN_NONE;
}
