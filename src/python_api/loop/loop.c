#include "loop.h"


PyObject*
PuringLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    int registry_size = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", (char*[]){"registry_size", NULL}, &registry_size)) {
        return NULL;
    }

    RequestRegistry* registry = registry_new(registry_size);
    if (!registry) return PyErr_NoMemory();

    PuringLoop *self = (PuringLoop *)type->tp_alloc(type, 0);
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
    self->initialized = false;
    self->is_closing = false;
    self->is_reader_installed = false;
    self->reader_callback = NULL;
    self->reader_capsule = NULL;

    return (PyObject*)self;
}


int
PuringLoop_init(PuringLoop *self, PyObject *args, PyObject *kwargs)
{

    PyObject *base = (PyObject *)Py_TYPE(self)->tp_base;
    PyObject *init = PyObject_GetAttrString(base, "__init__");
    if (!init) {
        return -1;
    }

    PyObject *res = PyObject_CallOneArg(init, (PyObject *)self);
    Py_DECREF(init);
    if (!res) {
        return -1;
    }
    Py_DECREF(res);

    memory_params mem_par = {0};
    ring_init_params params = {0};

    int ret = ring_init(&mem_par, &params, self->ring);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    set_signals_handler(self->ring);
    self->loop_tid = gettid();
    self->initialized = true;
    return 0;
}


void 
PuringLoop_dealloc(PuringLoop *self) {
    if (self->reader_callback) {
        Py_DECREF(self->reader_callback);
        self->reader_callback = NULL;
    }

    if (self->reader_capsule) {
        Py_DECREF(self->reader_capsule);
        self->reader_capsule = NULL;
    }

    if (self->registry) {
        registry_destroy(self->registry);
    }

    Py_TYPE(self)->tp_free((PyObject *)self);
}


PyObject*
PuringLoop_close_loop(PuringLoop *self, PyObject *args)
{
    PyObject *py_loop = (PyObject *)self;
    ASSERT_LOOP_THREAD(py_loop);

    if (self->is_closing)
        Py_RETURN_NONE;

    self->is_closing = true;

    PyObject *base = (PyObject *)Py_TYPE(self)->tp_base;
    PyObject *res = PyObject_CallMethod(base, "close", "O", py_loop);
    if (!res) {
        return NULL;
    }
    Py_DECREF(res);

    PyObject_CallMethod(py_loop, "remove_reader", "i", self->ring->ring_fd);

    graceful_shutdown(self->ring, self->registry);
    self->ring = NULL;
    self->registry = NULL;
    
    self->is_closing = false;
    Py_RETURN_NONE;
}


PyObject*
PuringLoop_add_reader(PuringLoop *self, PyObject *args)
{
    if (!self->is_reader_installed) {
        int res = uring_loop_register_fd(self);
        if (res == 1) {
            self->is_reader_installed = true;
        }
    }
    Py_RETURN_NONE;
}


PyObject*
PuringLoop_remove_reader(PuringLoop *self, PyObject *args)
{;}


PyObject*
PuringLoop_add_writer(PuringLoop *self, PyObject *args)
{;}


PyObject*
PuringLoop_remove_writer(PuringLoop *self, PyObject *args)
{;}


PyObject*
PuringLoop_run_once(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{;}

PyObject*
PuringLoop_write_to_self(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{;}

PyObject*
PuringLoop_process_events(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{;}

PyObject*
PuringLoop_make_socket_transport(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{;}

PyObject*
PuringLoop_make_read_pipe_transport(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{;}

PyObject*
PuringLoop_make_write_pipe_transport(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{;}
