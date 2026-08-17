#pragma clang diagnostic push
#if defined(__clang__) && (__clang_major__ >= 19)
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <structmember.h>
#include <liburing.h>

#include "python_api/loop/loop.h"
#include "python_api/ops/files/files.h"
#include "python_api/ops/sockets/sockets.h"
#include "python_api/timer/timer.h"
#include "python_api/execution_context/execution_context.h"


PyMODINIT_FUNC
PyInit_aio_uring(void);

static PyMethodDef aio_uring_module_methods[] = {
    {"timer", (PyCFunction)AioUringLoop_timer, METH_VARARGS | METH_KEYWORDS, "Sets a timer"},
    {"open_file", (PyCFunction)AioUring_open, METH_VARARGS | METH_KEYWORDS, "Opens file and instantiate File object"},
    {"prep_socket",
     (PyCFunction)AioUring_prep_socket,
     METH_VARARGS | METH_KEYWORDS,
     "Opens socket and instantiate Socket object"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef aio_uring_loop_methods[] = {
    {"close", (PyCFunction)AioUringLoop_close, METH_VARARGS, "Close loop"},
    {"buffer_mode",
     (PyCFunction)AioUringLoop_buffer_mode,
     METH_VARARGS | METH_KEYWORDS,
     "Init context to run loop with specific buffer mode"},
    {"stream_strategy",
     (PyCFunction)AioUringLoop_stream_strategy,
     METH_VARARGS | METH_KEYWORDS,
     "Init context to run loop with specific stream strategy"},
    {"transfer_mode",
     (PyCFunction)AioUringLoop_transfer_mode,
     METH_VARARGS | METH_KEYWORDS,
     "Init context to run loop with specific transfer mode"},
    {"execution_context",
     (PyCFunction)AioUringLoop_execution_context,
     METH_VARARGS | METH_KEYWORDS,
     "Init context to run loop with specific execution context settings"},

    {"_run_once", (PyCFunction)AioUringLoop_run_once, METH_NOARGS, "Run one full iteration of the event loop"},
    {"_write_to_self",
     (PyCFunction)AioUringLoop_write_to_self,
     METH_NOARGS,
     "Write a byte to self-pipe, to wake up the event loop"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef aio_uring_socket_methods[] = {
    {"bind", (PyCFunction)AioUringSocket_bind, METH_VARARGS | METH_KEYWORDS, "Bind socket"},
    {"connect", (PyCFunction)AioUringSocket_connect, METH_VARARGS | METH_KEYWORDS, "Connect"},
    {"listen", (PyCFunction)AioUringSocket_listen, METH_VARARGS | METH_KEYWORDS, "Listen socket"},
    {"accept", (PyCFunction)AioUringSocket_accept, METH_VARARGS | METH_KEYWORDS, "Accept"},
    {"close", (PyCFunction)AioUringSocket_close, METH_VARARGS | METH_KEYWORDS, "Close"},
    {"send", (PyCFunction)AioUringSocket_send, METH_VARARGS | METH_KEYWORDS, "Send"},
    {"recv", (PyCFunction)AioUringSocket_recv, METH_VARARGS | METH_KEYWORDS, "Recv"},
    {"sendto", (PyCFunction)AioUringSocket_sendto, METH_VARARGS | METH_KEYWORDS, "Sendto"},
    {"recvfrom", (PyCFunction)AioUringSocket_recvfrom, METH_VARARGS | METH_KEYWORDS, "Recvfrom"},
    {"sendmsg", (PyCFunction)AioUringSocket_sendmsg, METH_VARARGS | METH_KEYWORDS, "Sendmsg"},
    {"recvmsg", (PyCFunction)AioUringSocket_recvmsg, METH_VARARGS | METH_KEYWORDS, "Recvmsg"},

    {"__aenter__", (PyCFunction)AioUringSocket_aenter, METH_NOARGS, "Entering async context manager"},
    {"__aexit__", (PyCFunction)AioUringSocket_aexit, METH_VARARGS | METH_KEYWORDS, "Closing async context manager"},
    {NULL, NULL, 0, NULL}
};

static PyMethodDef aio_uring_file_methods[] = {
    // TODO: DOCS: Describe that short read/write handling is responsibility of
    // client
    // TODO: DOCS: Describe that because of async nature, we should explicitly
    // send offsets

    {"read", (PyCFunction)AioUringFile_read, METH_VARARGS | METH_KEYWORDS, "Read file"},
    {"readv", (PyCFunction)AioUringFile_readv, METH_VARARGS | METH_KEYWORDS, "Read file, vectorized"},
    {"readv_raw",
     (PyCFunction)AioUringFile_readv_raw,
     METH_VARARGS | METH_KEYWORDS,
     "Read file, vectorized with custom iovecs"},
    {"write", (PyCFunction)AioUringFile_write, METH_VARARGS | METH_KEYWORDS, "Write file"},
    {"writev", (PyCFunction)AioUringFile_writev, METH_VARARGS | METH_KEYWORDS, "Write file, vectorized"},
    {"writev_raw",
     (PyCFunction)AioUringFile_writev_raw,
     METH_VARARGS | METH_KEYWORDS,
     "Write file, vectorized with custom iovecs"},
    {"close", (PyCFunction)AioUringFile_close, METH_VARARGS | METH_KEYWORDS, "Close file"},
    {"fsync", (PyCFunction)AioUringFile_fsync, METH_VARARGS | METH_KEYWORDS, "Flush file buffer to file"},
    {"fdatasync",
     (PyCFunction)AioUringFile_fdatasync,
     METH_VARARGS | METH_KEYWORDS,
     "Flush file buffer to file with in fdatasync mode"},
    {"splice", (PyCFunction)AioUringFile_splice, METH_VARARGS | METH_KEYWORDS, "Splicing two file pipes"},

    {"__aenter__", (PyCFunction)AioUringFile_aenter, METH_NOARGS, "Entering async context manager"},
    {"__aexit__", (PyCFunction)AioUringFile_aexit, METH_VARARGS | METH_KEYWORDS, "Closing async context manager"},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef aio_uring_file_members[] = {
    {
        .name = "fd",
        .type = Py_T_INT,
        .offset = offsetof(AioUringFile, fd),
        .flags = READONLY,
        .doc = "file descriptor",
    },
    {0},
};

static PyMethodDef aio_uring_buffer_mode_ctx_methods[] = {
    {"__enter__", (PyCFunction)BufferModeCtx_enter, METH_NOARGS, "Entering context manager"},
    {"__exit__", (PyCFunction)BufferModeCtx_exit, METH_VARARGS, "Closing context manager"},

    {NULL, NULL, 0, NULL}
};

static PyMethodDef aio_uring_stream_strategy_ctx_methods[] = {
    {"__enter__", (PyCFunction)StreamStrategyCtx_enter, METH_NOARGS, "Entering context manager"},
    {"__exit__", (PyCFunction)StreamStrategyCtx_exit, METH_VARARGS, "Closing context manager"},

    {NULL, NULL, 0, NULL}
};

static PyMethodDef aio_uring_transfer_mode_ctx_methods[] = {
    {"__enter__", (PyCFunction)TransferModeCtx_enter, METH_NOARGS, "Entering context manager"},
    {"__exit__", (PyCFunction)TransferModeCtx_exit, METH_VARARGS, "Closing context manager"},

    {NULL, NULL, 0, NULL}
};

static PyMethodDef aio_uring_execution_context_ctx_methods[] = {
    {"__enter__", (PyCFunction)ExecutionContextCtx_enter, METH_NOARGS, "Entering context manager"},
    {"__exit__", (PyCFunction)ExecutionContextCtx_exit, METH_VARARGS, "Closing context manager"},

    {NULL, NULL, 0, NULL}
};

PyTypeObject *AioUringLoopType = NULL;

static PyType_Slot AioUringLoop_slots[] = {
    {Py_tp_doc, (void *)PyDoc_STR("Rings with python loop")},
    {Py_tp_new, AioUringLoop_new},
    {Py_tp_init, AioUringLoop_init},
    {Py_tp_dealloc, AioUringLoop_dealloc},
    {Py_tp_traverse, AioUringLoop_traverse},
    {Py_tp_clear, AioUringLoop_clear},
    {Py_tp_methods, aio_uring_loop_methods},
    {0, NULL}
};

static PyType_Spec AioUringLoop_spec = {
    .name = "aio_uring.src.python_api.loop.AioUringLoop",
    .basicsize = sizeof(AioUringLoop),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = AioUringLoop_slots,
};

PyTypeObject AioUringFileType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "aio_uring.File",
    .tp_doc = PyDoc_STR("aio_uring file adapter"),
    .tp_basicsize = sizeof(AioUringFile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_traverse = (traverseproc)AioUringFile_traverse,
    .tp_clear = (inquiry)AioUringFile_clear,
    .tp_dealloc = (destructor)AioUringFile_dealloc,
    .tp_methods = aio_uring_file_methods,
    .tp_members = aio_uring_file_members,
};

PyTypeObject AioUringSocketType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "aio_uring.Socket",
    .tp_doc = PyDoc_STR("aio_uring socket adapter"),
    .tp_basicsize = sizeof(AioUringSocket),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_traverse = (traverseproc)AioUringSocket_traverse,
    .tp_clear = (inquiry)AioUringSocket_clear,
    .tp_dealloc = (destructor)AioUringSocket_dealloc,
    .tp_methods = aio_uring_socket_methods,
};

PyTypeObject AioUringBufferModeCtxType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "aio_uring.src.python_api.execution_context.BufferModeCtx",
    .tp_doc = PyDoc_STR("Buffer Mode helper for context manager"),
    .tp_basicsize = sizeof(BufferModeCtx),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)BufferModeCtx_dealloc,
    .tp_methods = aio_uring_buffer_mode_ctx_methods,
};

PyTypeObject AioUringStreamStrategyCtxType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "aio_uring.src.python_api.execution_context.StreamStrategyCtx",
    .tp_doc = PyDoc_STR("Stream Strategy helper for context manager"),
    .tp_basicsize = sizeof(StreamStrategyCtx),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)StreamStrategyCtx_dealloc,
    .tp_methods = aio_uring_stream_strategy_ctx_methods,
};

PyTypeObject AioUringTransferModeCtxType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "aio_uring.src.python_api.execution_context.TransferModeCtx",
    .tp_doc = PyDoc_STR("Transfer Mode helper for context manager"),
    .tp_basicsize = sizeof(TransferModeCtx),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)TransferModeCtx_dealloc,
    .tp_methods = aio_uring_transfer_mode_ctx_methods,
};

PyTypeObject AioUringExecutionContextCtxType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0).tp_name = "aio_uring.src.python_api.execution_context.ExecutionContextCtx",
    .tp_doc = PyDoc_STR("Execution Context helper for context manager"),
    .tp_basicsize = sizeof(ExecutionContextCtx),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = NULL,
    .tp_dealloc = (destructor)ExecutionContextCtx_dealloc,
    .tp_methods = aio_uring_execution_context_ctx_methods,
};

static int
aio_uring_module_exec(PyObject *m) {
    PyObject *asyncio = PyImport_ImportModule("asyncio");
    if (!asyncio)
        return -1;

    PyObject *base = PyObject_GetAttrString(asyncio, "BaseEventLoop");
    Py_DECREF(asyncio);
    if (!base)
        return -1;

    PyObject *bases = PyTuple_Pack(1, base);
    Py_DECREF(base);
    if (!bases)
        return -1;

    PyObject *type = PyType_FromSpecWithBases(&AioUringLoop_spec, bases);
    Py_DECREF(bases);
    if (!type)
        return -1;

    AioUringLoopType = (PyTypeObject *)type;

    if (PyType_Ready(&AioUringSocketType) < 0)
        return -1;

    if (PyType_Ready(&AioUringFileType) < 0)
        return -1;

    if (PyType_Ready(&AioUringBufferModeCtxType) < 0)
        return -1;

    if (PyType_Ready(&AioUringStreamStrategyCtxType) < 0)
        return -1;

    if (PyType_Ready(&AioUringTransferModeCtxType) < 0)
        return -1;

    if (PyType_Ready(&AioUringExecutionContextCtxType) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "AioUringLoop", type) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "File", (PyObject *)&AioUringFileType) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "Socket", (PyObject *)&AioUringSocketType) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "BufferModeCtx", (PyObject *)&AioUringBufferModeCtxType) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "StreamStrategyCtx", (PyObject *)&AioUringStreamStrategyCtxType) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "TransferModeCtx", (PyObject *)&AioUringTransferModeCtxType) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "ExecutionContextCtx", (PyObject *)&AioUringExecutionContextCtxType) < 0)
        return -1;

    PyObject *resolve_flags = create_resolve_enum();
    if (!resolve_flags)
        return -1;

    if (PyModule_AddObject(m, "ResolveFlags", resolve_flags) < 0) {
        Py_DECREF(resolve_flags);
        return -1;
    }

    PyObject *statx_flags = create_statx_flags_enum();
    if (!statx_flags)
        return -1;

    if (PyModule_AddObject(m, "StatxFlags", statx_flags) < 0) {
        Py_DECREF(statx_flags);
        return -1;
    }

    PyObject *statx_mask = create_statx_mask_enum();
    if (!statx_mask)
        return -1;

    if (PyModule_AddObject(m, "StatxMask", statx_mask) < 0) {
        Py_DECREF(statx_mask);
        return -1;
    }

    PyObject *buffer_mode = create_buffer_mode_enum();
    if (!buffer_mode)
        return -1;

    if (PyModule_AddObject(m, "BUFFER_MODE", buffer_mode) < 0) {
        Py_DECREF(buffer_mode);
        return -1;
    }

    PyObject *stream_strategy = create_stream_strategy_enum();
    if (!stream_strategy)
        return -1;

    if (PyModule_AddObject(m, "STREAM_STRATEGY", stream_strategy) < 0) {
        Py_DECREF(stream_strategy);
        return -1;
    }

    PyObject *transfer_mode = create_transfer_mode_enum();
    if (!transfer_mode)
        return -1;

    if (PyModule_AddObject(m, "TRANSFER_MODE", transfer_mode) < 0) {
        Py_DECREF(transfer_mode);
        return -1;
    }

    PyObject *payload_type = create_payload_type_enum();
    if (!payload_type)
        return -1;

    if (PyModule_AddObject(m, "PAYLOAD_TYPE", payload_type) < 0) {
        Py_DECREF(payload_type);
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot aio_uring_module_slots[] = {
    {Py_mod_exec, aio_uring_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {0, NULL}
};

static PyModuleDef aio_uring_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "aio_uring",
    .m_doc = "Module contains uring based loop",
    .m_size = 0,
    .m_methods = aio_uring_module_methods,
    .m_slots = aio_uring_module_slots,
};

PyMODINIT_FUNC
PyInit_aio_uring(void) // cppcheck-suppress unusedFunction
{
    return PyModuleDef_Init(&aio_uring_module);
}
