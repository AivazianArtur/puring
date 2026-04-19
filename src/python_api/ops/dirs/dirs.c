#include "python_api/ops/dirs/dirs.h"

PyObject*
UringDir_stat(
    // UringDir *self,
    PyObject *args,
    PyObject *kwargs
)
{
    // ASSERT_LOOP_THREAD(self->loop->py_loop);
    // ASSERT_RING_LOOP_IS_CLOSING(self->loop);
    // if (self->closed) {
    //     PyErr_SetString(PyExc_BrokenPipeError, "File is closed");
    //     return NULL;
    // }

    // static char path;
    // int flags = 0;
    // unsigned mask = 0;
    // PyObject *timeout_params_obj = NULL;

    // static char *kwlist[] = {"path", "flags", "mask", "timeout_params", NULL};
    // if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "s|iIO", kwlist, &path, &timeout_params_obj, &flags, &mask))) {
    //     return NULL;
    // }
    // TimeoutParams timeout_params = {0};
    // parse_timeout_params(timeout_params_obj, &timeout_params);

    // PyObject *future = create_future(self->loop);
    // if (!future) {
    //     return NULL;
    // }

    // Py_ssize_t size = 1024;

    // PyObject *buffer = PyBytes_FromStringAndSize(NULL, size);
    // if (!buffer) {
    //     Py_DECREF(future);
    //     return PyErr_NoMemory();
    // }

    // int opcode = IORING_OP_STATX;
    // int request_idx = registry_add(
    //     self->loop->registry,
    //     future,
    //     buffer,
    //     NULL,
    //     opcode,
    //     self,
    //     NULL
    // );
    // if (request_idx < 0) {
    //     Py_DECREF(future);
    //     PyErr_SetString(PyExc_RuntimeError, "Registry is full");
    //     return NULL;
    // }

    // char *buf = PyBytes_AS_STRING(buffer);
    // if (!buf) {
    //     Py_DECREF(future);
    //     registry_remove(self->loop->registry, request_idx);
    //     PyErr_SetString(PyExc_TypeError, "Data in buffer is not byte objects");
    //     return NULL;
    // }

    // int result = uring_stat(
    //     self->loop->ring,
    //     request_idx,
    //     self->fd,
    //     &path,
    //     buf,
    //     flags,
    //     mask,
    //     &timeout_params
    // );
    // return _check_result(result, self, request_idx, future);
}


// openat2(dirfd, path, how)
// statx(dirfd, path, ...)
// openat(dirfd, path, flags, mode)
// mkdirat(dirfd, path, mode)
// linkat(olddirfd, oldpath, newdirfd, newpath, flags)
// symlinkat(target, dirfd, linkpath)
// readlinkat(dirfd, path, buf, size)
// renameat(olddirfd, oldpath, newdirfd, newpath)
// fchmodat(dirfd, path, mode, flags)
// fchownat(dirfd, path, uid, gid, flags)
// faccessat(dirfd, path, mode, flags)
// mknodat(dirfd, path, mode, dev)
