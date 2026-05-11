#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <liburing.h>

#include "python_api/loop/loop.h"
#include "python_api/ops/files/files.h"
#include "python_api/ops/dirs/dirs.h"
#include "python_api/ops/sockets/sockets.h"
#include "python_api/timer/timer.h"


static PyMethodDef puring_module_methods[] = {
    {"timer", (PyCFunction)PuringLoop_timer, METH_VARARGS | METH_KEYWORDS, "Sets a timer"},
    {NULL, NULL, 0, NULL}
};


static PyMethodDef puring_loop_methods[] = {
    {"add_reader", (PyCFunction)PuringLoop_add_reader, METH_NOARGS, "Start monitoring the fd file descriptor for read availability and invoke callback with the specified arguments once fd is available for reading"},
    {"remove_reader", (PyCFunction)PuringLoop_remove_reader, METH_NOARGS, "Stop monitoring the fd file descriptor for read availability. Returns True if fd was previously being monitored for reads"},

    {"add_writer", (PyCFunction)PuringLoop_add_writer, METH_NOARGS, "Start monitoring the fd file descriptor for write availability and invoke callback with the specified arguments args once fd is available for writing"},
    {"remove_writer", (PyCFunction)PuringLoop_remove_writer, METH_NOARGS, "Stop monitoring the fd file descriptor for write availability. Returns True if fd was previously being monitored for writes"},

    {"close_loop", (PyCFunction)PuringLoop_close_loop, METH_VARARGS, "Close loop"},

    // Create File
    {"open", (PyCFunction)PuringLoop_open, METH_VARARGS | METH_KEYWORDS, "Opens file and instantiate File object"},
    // Create Socket
    {"prep_socket", (PyCFunction)PuringLoop_prep_socket, METH_VARARGS | METH_KEYWORDS, "Opens socket and instantiate Socket object"},

    {"_run_once", (PyCFunction)PuringLoop_run_once, METH_NOARGS, "Run one full iteration of the event loop"},
    {"_write_to_self", (PyCFunction)PuringLoop_write_to_self, METH_NOARGS, "Write a byte to self-pipe, to wake up the event loop"},
    {"_process_events", (PyCFunction)PuringLoop_process_events, METH_O, "Do nothing. BaseEventLoop requirement"},
    {"_make_socket_transport", (PyCFunction)PuringLoop_make_socket_transport, METH_VARARGS | METH_KEYWORDS, "Create socket transport"},
    {"_make_read_pipe_transport", (PyCFunction)PuringLoop_make_read_pipe_transport, METH_VARARGS | METH_KEYWORDS, "Create read pipe transport"},
    {"_make_write_pipe_transport",(PyCFunction)PuringLoop_make_write_pipe_transport, METH_VARARGS | METH_KEYWORDS, "Create write pipe transport"},
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
    // TODO: DOCS: Describe that short read/write handling is responsibility of client
    // TODO: DOCS: Describe that because of async nature, we should explicitly send offsets

    {"read", (PyCFunction)PuringFile_read, METH_VARARGS | METH_KEYWORDS, "Read file"},
    {"readv", (PyCFunction)PuringFile_readv, METH_VARARGS | METH_KEYWORDS, "Read file, vectorized"},
    {"readv_raw", (PyCFunction)PuringFile_readv_raw, METH_VARARGS | METH_KEYWORDS, "Read file, vectorized with custom iovecs"},
    {"write", (PyCFunction)PuringFile_write, METH_VARARGS | METH_KEYWORDS, "Write file"},
    {"writev", (PyCFunction)PuringFile_writev, METH_VARARGS | METH_KEYWORDS, "Write file, vectorized"},
    {"writev_raw", (PyCFunction)PuringFile_writev_raw, METH_VARARGS | METH_KEYWORDS, "Write file, vectorized with custom iovecs"},
    {"close", (PyCFunction)PuringFile_close, METH_VARARGS | METH_KEYWORDS, "Close file"},
    {"fsync", (PyCFunction)PuringFile_fsync, METH_VARARGS | METH_KEYWORDS, "Flush file buffer to file"},
    {"fdatasync", (PyCFunction)PuringFile_fdatasync, METH_VARARGS | METH_KEYWORDS, "Flush file buffer to file with in fdatasync mode"},
    {"splice", (PyCFunction)PuringFile_splice, METH_VARARGS | METH_KEYWORDS, "Splicing two file pipes"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef puring_dir_methods[] = {
    {"stat", (PyCFunction)PuringDir_stat, METH_VARARGS | METH_KEYWORDS, "Directory info"},
    {NULL, NULL, 0, NULL}
};


PyTypeObject PuringLoopType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "puring.src.python_api.loop.PuringLoop",
    .tp_doc = PyDoc_STR("Rings with python loop"),
    .tp_basicsize = sizeof(PuringLoop),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PuringLoop_new,
    .tp_init = (initproc)PuringLoop_init,
    .tp_dealloc = (destructor)PuringLoop_dealloc,
    .tp_methods = puring_loop_methods,
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
puring_module_exec(PyObject *m)
{
    if (PyType_Ready(&PuringLoopType) < 0) {
        return -1;
    }
    if (PyType_Ready(&PuringSocketType) < 0) {
        return -1;
    }
    if (PyType_Ready(&PuringFileType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "uring", (PyObject *) &PuringLoopType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "file", (PyObject *) &PuringFileType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "socket", (PyObject *) &PuringSocketType) < 0) {
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


// PyObject *m = PyModule_Create(&puring_module);
// if (!m) return NULL;

// // ── patch tp_bases BEFORE PyType_Ready ──────────────────
// PyObject *asyncio = PyImport_ImportModule("asyncio");
// if (!asyncio) return NULL;

// PyObject *base = PyObject_GetAttrString(asyncio, "BaseEventLoop");
// Py_DECREF(asyncio);
// if (!base) return NULL;

// UringLoopType.tp_bases = PyTuple_Pack(1, base);
// Py_DECREF(base);
// if (!UringLoopType.tp_bases) return NULL;

// // ── now freeze the type ──────────────────────────────────
// if (PyType_Ready(&UringLoopType) < 0) return NULL;

// if (PyModule_AddObjectRef(m, "UringLoop",
//                             (PyObject *)&UringLoopType) < 0) return NULL;

// // ... rest of your module init (socket, file, dir types) ...

// return m;