#include "execution_context.h"

BufferModeCtx *
PuringLoop_buffer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char mode = NORMAL_BUFFER;
    static const char *kwlist[] = {"mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", (char **)kwlist, &mode)) {
        return NULL;
    }

    BufferMode buffer_mode;
    switch (mode) {
    case NORMAL_BUFFER:
        buffer_mode = NORMAL_BUFFER;
        break;
    case FIXED:
        buffer_mode = FIXED;
        break;
    case PROVIDED:
        buffer_mode = PROVIDED;
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for buffer mode");
        return NULL;
    }

    BufferModeCtx *buffer_mode_ctx = malloc(sizeof(BufferModeCtx));
    if (!buffer_mode_ctx) {
        PyErr_NoMemory();
        return NULL;
    }
    buffer_mode_ctx->loop = self;
    buffer_mode_ctx->payload = buffer_mode;
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

    StreamStrategyCtx *stream_strategy_ctx = malloc(sizeof(StreamStrategyCtx));
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

    TransferModeCtx *transfer_mode_ctx = malloc(sizeof(TransferModeCtx));
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

    ExecutionContextCtx *execution_context_ctx = malloc(sizeof(ExecutionContextCtx));
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
BufferMode_aenter(BufferModeCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    current_context->buffer_mode = self->payload;
    PyObject *current_context_obj = (PyObject *)current_context;
    ContextVar_set(current_context_obj);
}

void
BufferMode_aexit(BufferModeCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    current_context->buffer_mode = NORMAL_BUFFER;
    PyObject *current_context_obj = (PyObject *)current_context;
    Py_DECREF(self);
    free(self);
    ContextVar_set(current_context_obj);
}

void
StreamStrategy_aenter(StreamStrategyCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    current_context->stream = self->payload;
    PyObject *current_context_obj = (PyObject *)current_context;
    ContextVar_set(current_context_obj);
}

void
StreamStrategy_aexit(StreamStrategyCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    current_context->stream = ONESHOT;
    PyObject *current_context_obj = (PyObject *)current_context;
    Py_DECREF(self);
    free(self);
    ContextVar_set(current_context_obj);
}

void
TransferMode_aenter(TransferModeCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    current_context->transfer_mode = self->payload;
    PyObject *current_context_obj = (PyObject *)current_context;
    ContextVar_set(current_context_obj);
}

void
TransferMode_aexit(TransferModeCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    current_context->transfer_mode = NORMAL_TRANSFER;
    PyObject *current_context_obj = (PyObject *)current_context;
    Py_DECREF(self);
    free(self);
    ContextVar_set(current_context_obj);
}

void
ExecutionContext_aenter(ExecutionContextCtx *self) {
    PyObject *current_context_obj = (PyObject *)self;
    ContextVar_set(current_context_obj);
}

void
ExecutionContext_aexit(ExecutionContextCtx *self) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    current_context->buffer_mode = NORMAL_BUFFER;
    current_context->stream = ONESHOT;
    current_context->transfer_mode = NORMAL_TRANSFER;
    PyObject *current_context_obj = (PyObject *)current_context;
    free(self->payload);
    Py_DECREF(self);
    free(self);
    ContextVar_set(current_context_obj);
}
