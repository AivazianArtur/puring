#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>

#include "python_api/loop/loop.h"
#include "python_api/ops/files/files.h"
#include "python_api/ops/dirs/dirs.h"
#include "python_api/ops/sockets/sockets.h"
#include "python_api/timer/timer.h"


static PyMethodDef puring_module_methods[] = {
    {"timer", (PyCFunction)UringLoop_timer, METH_VARARGS | METH_KEYWORDS, "Sets a timer"},
    {NULL, NULL, 0, NULL}
};


static PyMethodDef puring_loop_methods[] = {
    {"add_reader", (PyCFunction)UringLoop_add_reader, METH_NOARGS,"Register FD with UringLoop"},
    {"close_loop", (PyCFunction)UringLoop_close_loop, METH_VARARGS, "Close loop"},

    // Create File
    {"open", (PyCFunction)UringLoop_open, METH_VARARGS | METH_KEYWORDS, "Opens file"},

    // Create Socket
    {"prep_socket", (PyCFunction)UringLoop_prep_socket, METH_VARARGS | METH_KEYWORDS, "Opens socket"},
    {NULL, NULL, 0, NULL}
};


static PyMethodDef puring_socket_methods[] = {
    {"bind", (PyCFunction)UringSocket_bind, METH_VARARGS | METH_KEYWORDS, "Bind socket"},
    {"connect", (PyCFunction)UringSocket_connect, METH_VARARGS | METH_KEYWORDS, "Connect"},
    {"listen", (PyCFunction)UringSocket_listen, METH_VARARGS | METH_KEYWORDS, "Listen socket"},
    {"accept", (PyCFunction)UringSocket_accept, METH_VARARGS | METH_KEYWORDS, "Accept"},
    {"close", (PyCFunction)UringSocket_close, METH_VARARGS | METH_KEYWORDS, "Close"},
    {"send", (PyCFunction)UringSocket_send, METH_VARARGS | METH_KEYWORDS, "Send"},
    {"recv", (PyCFunction)UringSocket_recv, METH_VARARGS | METH_KEYWORDS, "Recv"},
    {"sendto", (PyCFunction)UringSocket_sendto, METH_VARARGS | METH_KEYWORDS, "Sendto"},
    {"recvfrom", (PyCFunction)UringSocket_recvfrom, METH_VARARGS | METH_KEYWORDS, "Recvfrom"},
    {"sendmsg", (PyCFunction)UringSocket_sendmsg, METH_VARARGS | METH_KEYWORDS, "Sendmsg"},
    {"recvmsg", (PyCFunction)UringSocket_recvmsg, METH_VARARGS | METH_KEYWORDS, "Recvmsg"},

    {NULL, NULL, 0, NULL}
};

static PyMethodDef puring_file_methods[] = {
    // TODO: DOCS: Describe that short read/write handling is responsibility of client
    // TODO: DOCS: Describe that because of async nature, we should explicitly send offsets

    {"read", (PyCFunction)UringFile_read, METH_VARARGS | METH_KEYWORDS, "Read file"},
    {"readv", (PyCFunction)UringFile_readv, METH_VARARGS | METH_KEYWORDS, "Read file, vectorized"},
    {"readv_raw", (PyCFunction)UringFile_readv_raw, METH_VARARGS | METH_KEYWORDS, "Read file, vectorized with custom iovecs"},
    {"write", (PyCFunction)UringFile_write, METH_VARARGS | METH_KEYWORDS, "Write file"},
    {"writev", (PyCFunction)UringFile_writev, METH_VARARGS | METH_KEYWORDS, "Write file, vectorized"},
    {"writev_raw", (PyCFunction)UringFile_writev_raw, METH_VARARGS | METH_KEYWORDS, "Write file, vectorized with custom iovecs"},
    {"close", (PyCFunction)UringFile_close, METH_VARARGS | METH_KEYWORDS, "Close file"},
    {"fsync", (PyCFunction)UringFile_fsync, METH_VARARGS | METH_KEYWORDS, "Flush file buffer to file"},
    {"fdatasync", (PyCFunction)UringFile_fdatasync, METH_VARARGS | METH_KEYWORDS, "Flush file buffer to file with in fdatasync mode"},
    {"splice", (PyCFunction)UringFile_splice, METH_VARARGS | METH_KEYWORDS, "Splicing two file pipes"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef puring_dir_methods[] = {
    {"stat", (PyCFunction)UringDir_stat, METH_VARARGS | METH_KEYWORDS, "Directory info"},
    {NULL, NULL, 0, NULL}
};


PyTypeObject UringLoopType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.src.python_api.loop.UringLoop",
    .tp_doc = PyDoc_STR("Rings with python loop"),
    .tp_basicsize = sizeof(UringLoop),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = UringLoop_new,
    .tp_init = (initproc)UringLoop_init,
    .tp_dealloc = (destructor)UringLoop_dealloc,
    .tp_methods = puring_loop_methods,
};

PyTypeObject UringFileType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.src.python_api.ops.files.UringFile",
    .tp_doc = PyDoc_STR("Puring file adapter"),
    .tp_basicsize = sizeof(UringFile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)UringFile_dealloc,
    .tp_methods = puring_file_methods,
};

PyTypeObject UringSocketType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.src.python_api.ops.sockets.UringSocket",
    .tp_doc = PyDoc_STR("Puring socket adapter"),
    .tp_basicsize = sizeof(UringSocket),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)UringSocket_dealloc,
    .tp_methods = puring_socket_methods,
};


static int
puring_module_exec(PyObject *m)
{
    if (PyType_Ready(&UringLoopType) < 0) {
        return -1;
    }
    if (PyType_Ready(&UringSocketType) < 0) {
        return -1;
    }
    if (PyType_Ready(&UringFileType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "uring", (PyObject *) &UringLoopType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "file", (PyObject *) &UringFileType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "socket", (PyObject *) &UringSocketType) < 0) {
        return -1;
    }

    PyObject *resolve_flags = create_resolve_enum();
    if (!resolve_flags) {
        return -1;
    }
    if (PyModule_AddObject(m, "ResolveFlags", resolve_flags) < 0) {
        Py_DECREF(resolve_flags);
        return -1;
    }

    PyObject *statx_flags = create_statx_flags_enum();
    if (!statx_flags) {
        return -1;
    }
    if (PyModule_AddObject(m, "StatxFlags", statx_flags) < 0) {
        Py_DECREF(statx_flags);
        return -1;
    }

    PyObject *statx_mask = create_statx_mask_enum();
    if (!statx_mask) {
        return -1;
    }
    if (PyModule_AddObject(m, "StatxMask", statx_mask) < 0) {
        Py_DECREF(statx_mask);
        return -1;
    }

    return 0;
}


static PyModuleDef_Slot puring_module_slots[] = {
    {Py_mod_exec, puring_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};


static PyModuleDef puring_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "puring",        
    .m_doc = "Module contains uring based loop",
    .m_size = 0,
    .m_methods = puring_module_methods,
    .m_slots = puring_module_slots,
};


PyMODINIT_FUNC
PyInit_puring(void)
{
    return PyModuleDef_Init(&puring_module);
}
