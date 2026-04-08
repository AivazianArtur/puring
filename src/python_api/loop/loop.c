#include "loop.h"


PyObject*
UringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    int registry_size = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", (char*[]){"registry_size", NULL}, &registry_size)) {
        return NULL;
    }

    RequestRegistry* registry = registry_new(registry_size);
    if (!registry) return PyErr_NoMemory();

    UringLoop *self = (UringLoop *)type->tp_alloc(type, 0);
    if (!self) {
        registry_destroy(registry);
        return PyErr_NoMemory();
    }

    self->ring = calloc(1, sizeof(struct io_uring));
    if (!self->ring) {
        registry_destroy(registry);
        Py_TYPE(self)->tp_free((PyObject*)self);
        return PyErr_NoMemory();
    }

    self->registry = registry;
    self->py_loop = NULL;
    self->initialized = false;
    self->is_closing = false;
    self->is_reader_installed = false;
    self->reader_callback = NULL;
    self->reader_capsule = NULL;

    return (PyObject*)self;
}


int
UringLoop_init(UringLoop *self, PyObject *args, PyObject *kwargs)
{
    PyObject* python_loop = _get_loop();
    if (!python_loop)
        return -1;

    Py_INCREF(python_loop);
    self->py_loop = python_loop;

    memory_params mem_par = {0};
    ring_init_params params = {0};

    int ret = ring_init(&mem_par, &params, self->ring);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    set_signals_handler(self->ring);

    self->initialized = true;
    return 0;
}


void 
UringLoop_dealloc(UringLoop *self) {
    if (self->reader_callback) {
        Py_DECREF(self->reader_callback);
        self->reader_callback = NULL;
    }

    if (self->reader_capsule) {
        Py_DECREF(self->reader_capsule);
        self->reader_capsule = NULL;
    }

    if (self->py_loop) {
        Py_XDECREF(self->py_loop);
    }

    if (self->registry) {
        registry_destroy(self->registry);
    }

    Py_TYPE(self)->tp_free((PyObject *)self);
}


PyObject*
UringLoop_close_loop(UringLoop *self, PyObject *args)
{
    ASSERT_LOOP_THREAD(self->py_loop);

    if (self->is_closing)
        Py_RETURN_NONE;

    self->is_closing = true;

    PyObject_CallMethod(self->py_loop, "remove_reader", "i", self->ring->ring_fd);

    struct io_uring_cqe *cqe;
    struct __kernel_timespec ts;
    ts.tv_nsec = 0;
    ts.tv_sec = 3;

     while (io_uring_wait_cqe_timeout(self->ring, &cqe, &ts) == 0) {
        int index = (int)(uintptr_t)cqe->user_data;
        registry_remove(self->registry, index);  // TODO: set future exception
        io_uring_cqe_seen(self->ring, cqe);
    }

    ring_destroy(self->ring);
    self->ring = NULL;

    registry_destroy(self->registry);
    self->registry = NULL;

    Py_RETURN_NONE;
}


PyObject*
UringLoop_add_reader(UringLoop *self, PyObject *args)
{
    if (!self->is_reader_installed) {
        int res = uring_loop_register_fd(self);
        if (res == 1) {
            self->is_reader_installed = true;
        }
    }
    Py_RETURN_NONE;
}


// In next versions

// UringLoop_run_forever(UringLoop *self, PyObject *args)
// UringLoop_stop(UringLoop *self, PyObject *args)
// UringLoop_call_soon(UringLoop *self, PyObject *args)
// UringLoop_call_later(UringLoop *self, PyObject *args)
