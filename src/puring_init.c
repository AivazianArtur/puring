#pragma clang diagnostic push
#if defined(__clang__) && (__clang_major__ >= 19)
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>

#include "python_api/loop/loop.h"
#include "python_api/ops/files/files.h"
#include "python_api/ops/sockets/sockets.h"
#include "python_api/timer/timer.h"
#include "python_api/execution_context/execution_context.h"


PyMODINIT_FUNC
PyInit_puring(void);

static PyMethodDef puring_module_methods[] = {
    {"timer", (PyCFunction)PuringLoop_timer, METH_VARARGS | METH_KEYWORDS, "Sets a timer"},
    {"open_file",
     (PyCFunction)PuringLoop_open,
     METH_VARARGS | METH_KEYWORDS,
     "Opens file and instantiate File object"},
    {"prep_socket",
     (PyCFunction)PuringLoop_prep_socket,
     METH_VARARGS | METH_KEYWORDS,
     "Opens socket and instantiate Socket object"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef puring_loop_methods[] = {
    {"close", (PyCFunction)PuringLoop_close, METH_VARARGS, "Close loop"},

    {"_run_once",
     (PyCFunction)PuringLoop_run_once,
     METH_NOARGS,
     "Run one full iteration of the event loop"},
    {"_write_to_self",
     (PyCFunction)PuringLoop_write_to_self,
     METH_NOARGS,
     "Write a byte to self-pipe, to wake up the event loop"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef puring_socket_methods[] = {
    {"bind", (PyCFunction)PuringSocket_bind, METH_VARARGS | METH_KEYWORDS, "Bind socket"},
    {"connect", (PyCFunction)PuringSocket_connect, METH_VARARGS | METH_KEYWORDS, "Connect"},
    {"listen", (PyCFunction)PuringSocket_listen, METH_VARARGS | METH_KEYWORDS, "Listen socket"},
    {"accept", (PyCFunction)PuringSocket_accept, METH_VARARGS | METH_KEYWORDS, "Accept"},
    {"close", (PyCFunction)PuringSocket_close, METH_VARARGS | METH_KEYWORDS, "Close"},
    {"send", (PyCFunction)PuringSocket_send, METH_VARARGS | METH_KEYWORDS, "Send"},
    {"recv", (PyCFunction)PuringSocket_recv, METH_VARARGS | METH_KEYWORDS, "Recv"},
    {"sendto", (PyCFunction)PuringSocket_sendto, METH_VARARGS | METH_KEYWORDS, "Sendto"},
    {"recvfrom", (PyCFunction)PuringSocket_recvfrom, METH_VARARGS | METH_KEYWORDS, "Recvfrom"},
    {"sendmsg", (PyCFunction)PuringSocket_sendmsg, METH_VARARGS | METH_KEYWORDS, "Sendmsg"},
    {"recvmsg", (PyCFunction)PuringSocket_recvmsg, METH_VARARGS | METH_KEYWORDS, "Recvmsg"},

    {NULL, NULL, 0, NULL}
};

static PyMethodDef puring_file_methods[] = {
    // TODO: DOCS: Describe that short read/write handling is responsibility of
    // client
    // TODO: DOCS: Describe that because of async nature, we should explicitly
    // send offsets

    {"read", (PyCFunction)PuringFile_read, METH_VARARGS | METH_KEYWORDS, "Read file"},
    {"readv", (PyCFunction)PuringFile_readv, METH_VARARGS | METH_KEYWORDS, "Read file, vectorized"},
    {"readv_raw",
     (PyCFunction)PuringFile_readv_raw,
     METH_VARARGS | METH_KEYWORDS,
     "Read file, vectorized with custom iovecs"},
    {"write", (PyCFunction)PuringFile_write, METH_VARARGS | METH_KEYWORDS, "Write file"},
    {"writev",
     (PyCFunction)PuringFile_writev,
     METH_VARARGS | METH_KEYWORDS,
     "Write file, vectorized"},
    {"writev_raw",
     (PyCFunction)PuringFile_writev_raw,
     METH_VARARGS | METH_KEYWORDS,
     "Write file, vectorized with custom iovecs"},
    {"close", (PyCFunction)PuringFile_close, METH_VARARGS | METH_KEYWORDS, "Close file"},
    {"fsync",
     (PyCFunction)PuringFile_fsync,
     METH_VARARGS | METH_KEYWORDS,
     "Flush file buffer to file"},
    {"fdatasync",
     (PyCFunction)PuringFile_fdatasync,
     METH_VARARGS | METH_KEYWORDS,
     "Flush file buffer to file with in fdatasync mode"},
    {"splice",
     (PyCFunction)PuringFile_splice,
     METH_VARARGS | METH_KEYWORDS,
     "Splicing two file pipes"},
    {NULL, NULL, 0, NULL}
};

PyTypeObject *PuringLoopType = NULL;

static PyType_Slot PuringLoop_slots[] = {
    {Py_tp_doc, (void *)PyDoc_STR("Rings with python loop")},
    {Py_tp_new, PuringLoop_new},
    {Py_tp_init, PuringLoop_init},
    {Py_tp_dealloc, PuringLoop_dealloc},
    {Py_tp_methods, puring_loop_methods},
    {0, NULL}
};

static PyType_Spec PuringLoop_spec = {
    .name = "puring.src.python_api.loop.PuringLoop",
    .basicsize = sizeof(PuringLoop),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = PuringLoop_slots,
};

PyTypeObject PuringFileType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
                   .tp_name = "puring.src.python_api.ops.files.PuringFile",
    .tp_doc = PyDoc_STR("Puring file adapter"),
    .tp_basicsize = sizeof(PuringFile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)PuringFile_dealloc,
    .tp_methods = puring_file_methods,
};

PyTypeObject PuringSocketType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
                   .tp_name = "puring.src.python_api.ops.sockets.PuringSocket",
    .tp_doc = PyDoc_STR("Puring socket adapter"),
    .tp_basicsize = sizeof(PuringSocket),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)PuringSocket_dealloc,
    .tp_methods = puring_socket_methods,
};

static int
puring_module_exec(PyObject *m) {
    PyObject *asyncio = PyImport_ImportModule("asyncio");
    if (!asyncio) {
        return -1;
    }

    PyObject *base = PyObject_GetAttrString(asyncio, "BaseEventLoop");
    Py_DECREF(asyncio);
    if (!base) {
        return -1;
    }

    PyObject *bases = PyTuple_Pack(1, base);
    Py_DECREF(base);
    if (!bases) {
        return -1;
    }

    PyObject *type = PyType_FromSpecWithBases(&PuringLoop_spec, bases);
    Py_DECREF(bases);
    if (!type) {
        return -1;
    }

    PuringLoopType = (PyTypeObject *)type;

    if (PyType_Ready(&PuringSocketType) < 0) {
        return -1;
    }
    if (PyType_Ready(&PuringFileType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "PuringLoop", type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "File", (PyObject *)&PuringFileType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "Socket", (PyObject *)&PuringSocketType) < 0) {
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

    PyObject *buffer_mode = create_buffer_mode_enum();
    if (!buffer_mode) {
        return -1;
    }
    if (PyModule_AddObject(m, "BUFFER_MODE", buffer_mode) < 0) {
        Py_DECREF(buffer_mode);
        return -1;
    }

    PyObject *stream_strategy = create_stream_strategy_enum();
    if (!stream_strategy) {
        return -1;
    }
    if (PyModule_AddObject(m, "STREAM_STRATEGY", stream_strategy) < 0) {
        Py_DECREF(stream_strategy);
        return -1;
    }

    PyObject *transfer_mode = create_transfer_mode_enum();
    if (!transfer_mode) {
        return -1;
    }
    if (PyModule_AddObject(m, "TRANSFER_MODE", transfer_mode) < 0) {
        Py_DECREF(transfer_mode);
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
PyInit_puring(void) // cppcheck-suppress unusedFunction
{
    return PyModuleDef_Init(&puring_module);
}
