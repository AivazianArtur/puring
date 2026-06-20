#include "execution_context.h"
#include <stdlib.h>

#include "loop.h"

BufferModeCtx *
PuringLoop_buffer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char mode = NORMAL_BUFFER;
    PyObject *buffers_obj;
    static const char *kwlist[] = {"mode", "buffers", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|so", (char **)kwlist, &mode, &buffers_obj)) {
        return NULL;
    }

    PayloadOrigin payload_origin;
    PayloadType payload_type;
    PayloadType buffer_type = PAYLOAD_LINEAR;
    int len;
    int bufsize;
    if (buffers_obj) {
        if (!PySequence_Check(buffers_obj)) {
            PyErr_SetString(PyExc_TypeError, "Buffers must be a sequence");
            return NULL;
        }
        Py_ssize_t py_len = PySequence_Length(buffers_obj);
        if (py_len < 0) {
            return NULL;
        }
        len = (int)py_len;
        payload_origin = PAYLOAD_USER;
    } else {
        payload_origin = PAYLOAD_RUNTIME;
        // TODO: in next version need to get values from puring parameters(puring initialization)
        len = 3;
        bufsize = 1024;
    }

    BufferPayload *buffer_payload = malloc(sizeof(BufferPayload));
    if (!buffer_payload) {
        PyErr_NoMemory();
        return NULL;
    }

    switch (mode) {
    case NORMAL_BUFFER:
        if (payload_origin == PAYLOAD_RUNTIME) {
            buffer_payload = make_buffers(len, bufsize, buffer_payload);
        } else {
            buffer_payload = serialize_buffers(buffers_obj, len, buffer_payload);
        }
        if (!buffer_payload)
            return NULL;
        break;
    case FIXED:
        payload_type = PAYLOAD_LINEAR;
        LinearBuffer *buffers;
        if (payload_origin == PAYLOAD_RUNTIME) {
            buffers = create_linear_buffers(len, bufsize, buffer_payload);
        } else {
            buffers = serialize_linear_buffers(buffers_obj, len, buffer_payload);
        }
        if (!buffers)
            return NULL;

        buffer_payload->payload_type = payload_type;
        break;
    case PROVIDED:
        payload_type = PAYLOAD_LINEAR;
        LinearBuffer *buffers;
        if (payload_origin == PAYLOAD_RUNTIME) {
            buffers = create_linear_buffers(len, bufsize, buffer_payload);
        } else {
            buffers = serialize_linear_buffers(buffers_obj, len, buffer_payload);
        }
        if (!buffers)
            return NULL;

        buffer_payload->payload_type = payload_type;
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for buffer mode");
        return NULL;
    }

    buffer_payload->len = len;
    buffer_payload->payload_origin = payload_origin;

    BufferModeCtx *buffer_mode_ctx = PyObject_New(BufferModeCtx, &PuringBufferModeCtxType);
    if (!buffer_mode_ctx) {
        PyErr_NoMemory();
        return NULL;
    }
    buffer_mode_ctx->loop = self;
    buffer_mode_ctx->payload = (BufferMode)mode;
    buffer_mode_ctx->token = NULL;
    buffer_mode_ctx->buffer_payload = buffer_payload;
    return buffer_mode_ctx;
}

StreamStrategyCtx *
PuringLoop_stream_strategy(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char stream = ONESHOT;
    static const char *kwlist[] = {"stream", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", (char **)kwlist, &stream)) {
        return NULL;
    }

    StreamStrategy stream_strategy;
    switch (stream) {
    case ONESHOT:
        stream_strategy = ONESHOT;
        break;
    case MULTISHOT:
        stream_strategy = MULTISHOT;
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for stream strategy");
        return NULL;
    }

    StreamStrategyCtx *stream_strategy_ctx = PyObject_New(StreamStrategyCtx, &PuringStreamStrategyCtxType);
    if (!stream_strategy_ctx) {
        PyErr_NoMemory();
        return NULL;
    }
    stream_strategy_ctx->loop = self;
    stream_strategy_ctx->payload = stream_strategy;
    return stream_strategy_ctx;
}

TransferModeCtx *
PuringLoop_transfer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char mode = NORMAL_TRANSFER;
    static const char *kwlist[] = {"mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", (char **)kwlist, &mode)) {
        return NULL;
    }

    TransferMode transfer_mode;
    switch (mode) {
    case NORMAL_TRANSFER:
        transfer_mode = NORMAL_TRANSFER;
        break;
    case ZERO_COPY:
        transfer_mode = ZERO_COPY;
        break;
    case BUFFER_POOL:
        transfer_mode = BUFFER_POOL;
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for transfer mode");
        return NULL;
    }

    TransferModeCtx *transfer_mode_ctx = PyObject_New(TransferModeCtx, &PuringTransferModeCtxType);
    if (!transfer_mode_ctx) {
        PyErr_NoMemory();
        return NULL;
    }
    transfer_mode_ctx->loop = self;
    transfer_mode_ctx->payload = transfer_mode;
    return transfer_mode_ctx;
}

ExecutionContextCtx *
PuringLoop_execution_context(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char buffer_mode = NORMAL_BUFFER;
    char stream_strategy = ONESHOT;
    char transfer_mode = NORMAL_TRANSFER;
    static const char *kwlist[] = {"buffer_mode", "stream_strategy", "transfer_mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|sss", (char **)kwlist, &buffer_mode)) {
        return NULL;
    }

    BufferMode buffer_mode_val;
    switch (buffer_mode) {
    case NORMAL_BUFFER:
        buffer_mode_val = NORMAL_BUFFER;
        break;
    case FIXED:
        buffer_mode_val = FIXED;
        break;
    case PROVIDED:
        buffer_mode_val = PROVIDED;
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for buffer mode");
        return NULL;
    }

    StreamStrategy stream_strategy_val;
    switch (stream_strategy) {
    case ONESHOT:
        stream_strategy_val = ONESHOT;
        break;
    case MULTISHOT:
        stream_strategy_val = MULTISHOT;
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for stream strategy");
        return NULL;
    }

    TransferMode transfer_mode_val;
    switch (transfer_mode) {
    case NORMAL_TRANSFER:
        transfer_mode_val = NORMAL_TRANSFER;
        break;
    case ZERO_COPY:
        transfer_mode_val = ZERO_COPY;
        break;
    case BUFFER_POOL:
        transfer_mode_val = BUFFER_POOL;
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for transfer mode");
        return NULL;
    }

    ExecutionContextCtx *execution_context_ctx = PyObject_New(ExecutionContextCtx, &PuringExecutionContextCtxType);
    if (!execution_context_ctx) {
        PyErr_NoMemory();
        return NULL;
    }

    ExecutionContext *execution_context = malloc(sizeof(ExecutionContext));
    if (!execution_context) {
        PyErr_NoMemory();
        free(execution_context_ctx);
        return NULL;
    }

    execution_context->buffer_mode = buffer_mode_val;
    execution_context->stream = stream_strategy_val;
    execution_context->transfer_mode = transfer_mode_val;
    execution_context_ctx->payload = execution_context;
    return execution_context_ctx;
}

void
BufferModeCtx_aenter(BufferModeCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    if (!current_context) {
        PyErr_SetString(PyExc_ValueError, "Error while getting contextvar");
        return;
    }
    current_context->buffer_mode = self->payload;
    PyObject *current_context_obj = (PyObject *)current_context;
    PyObject *token = ContextVar_set(current_context_obj);
    Py_DECREF(current_context_obj);
    if (token < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while setting contextvar");
        return;
    }
    self->token = token;
}

void
BufferModeCtx_aexit(BufferModeCtx *self) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
    }
}

void
BufferModeCtx_dealloc(BufferModeCtx *self) {
    PyObject_Free(self);
}

void
StreamStrategyCtx_aenter(StreamStrategyCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    if (!current_context) {
        PyErr_SetString(PyExc_ValueError, "Error while getting contextvar");
        return;
    }
    current_context->stream = self->payload;
    PyObject *current_context_obj = (PyObject *)current_context;
    PyObject *token = ContextVar_set(current_context_obj);
    Py_DECREF(current_context_obj);
    if (token < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while setting contextvar");
        return;
    }
    self->token = token;
}

void
StreamStrategyCtx_aexit(StreamStrategyCtx *self) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
    }
}

void
StreamStrategyCtx_dealloc(StreamStrategyCtx *self) {
    PyObject_Free(self);
}

void
TransferModeCtx_aenter(TransferModeCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    if (!current_context) {
        PyErr_SetString(PyExc_ValueError, "Error while getting contextvar");
        return;
    }
    current_context->transfer_mode = self->payload;
    PyObject *current_context_obj = (PyObject *)current_context;
    PyObject *token = ContextVar_set(current_context_obj);
    Py_DECREF(current_context_obj);
    if (token < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while setting contextvar");
        return;
    }
    self->token = token;
}

void
TransferModeCtx_aexit(TransferModeCtx *self) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
    }
}

void
TransferModeCtx_dealloc(TransferModeCtx *self) {
    PyObject_Free(self);
}

void
ExecutionContextCtx_aenter(ExecutionContextCtx *self) {
    PyObject *current_context_obj = (PyObject *)self;
    int token = ContextVar_set(current_context_obj);
    if (token < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while setting contextvar");
        return;
    }
    self->token = token;
}

void
ExecutionContextCtx_aexit(ExecutionContextCtx *self) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
    }
}

void
ExecutionContextCtx_dealloc(ExecutionContextCtx *self) {
    PyObject_Clear(self);
}
