#include "execution_context.h"
#include <stdlib.h>

#include "python_api/loop/loop.h"

BufferModeCtx *
PuringLoop_buffer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    BufferMode mode = NORMAL_BUF;
    PayloadType payload_type_obj = PAYLOAD_LINEAR;
    PyObject *buffers_obj = NULL;
    int amount = 3;
    int bufsize = 1024;
    static const char *kwlist[] = {"mode", "buffers", "payload_type", "amount", "bufsize", NULL};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "|iOiii", (char **)kwlist, &mode, &buffers_obj, &payload_type_obj, &amount, &bufsize
        )) {
        return NULL;
    }

    BufferMode buffer_mode = _validate_buffer_mode(mode);
    if (buffer_mode == BUF_NO_VAL)
        return NULL;

    PayloadType payload_type = _validate_payload_type(payload_type_obj);
    if (payload_type == PAYLOAD_TYPE_NO_VAL)
        return NULL;

    BufferPayload *buffer_payload = create_buffer_payload(buffer_mode, payload_type, buffers_obj, amount, bufsize);
    if (!buffer_payload)
        return NULL;

    BufferModeCtx *buffer_mode_ctx = PyObject_New(BufferModeCtx, &PuringBufferModeCtxType);
    if (!buffer_mode_ctx) {
        free_buffer_payload(buffer_payload, true);
        PyErr_NoMemory();
        return NULL;
    }
    buffer_mode_ctx->loop = self;
    buffer_mode_ctx->payload = buffer_mode;
    buffer_mode_ctx->token = NULL;
    buffer_mode_ctx->buffer_payload = buffer_payload;
    return buffer_mode_ctx;
}

StreamStrategyCtx *
PuringLoop_stream_strategy(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    StreamStrategy stream = ONESHOT;
    static const char *kwlist[] = {"stream", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", (char **)kwlist, &stream)) {
        return NULL;
    }

    switch (stream) {
    case ONESHOT:
    case MULTISHOT:
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
    stream_strategy_ctx->payload = stream;
    return stream_strategy_ctx;
}

TransferModeCtx *
PuringLoop_transfer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    TransferMode transfer_mode = NORMAL_TRANSFER;
    static const char *kwlist[] = {"mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", (char **)kwlist, &transfer_mode)) {
        return NULL;
    }

    switch (transfer_mode) {
    case NORMAL_TRANSFER:
    case ZERO_COPY:
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

    BufferMode buffer_mode_val = NORMAL_BUF;
    StreamStrategy stream_strategy = ONESHOT;
    TransferMode transfer_mode = NORMAL_TRANSFER;
    PayloadType payload_type_obj = PAYLOAD_LINEAR;
    PyObject *buffers_obj = NULL;
    int amount = 3;
    int bufsize = 1024;

    static const char *kwlist[] = {
        "buffer_mode", "stream_strategy", "transfer_mode", "buffers", "payload_type", "amount", "bufsize", NULL
    };
    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "|iiiOiii",
            (char **)kwlist,
            &buffer_mode_val,
            &stream_strategy,
            &transfer_mode,
            &buffers_obj,
            &payload_type_obj,
            &amount,
            &bufsize
        )) {
        return NULL;
    }

    BufferMode buffer_mode = _validate_buffer_mode(buffer_mode_val);
    if (buffer_mode == BUF_NO_VAL)
        return NULL;

    PayloadType payload_type = _validate_payload_type(payload_type_obj);
    if (payload_type == PAYLOAD_TYPE_NO_VAL)
        return NULL;

    BufferPayload *buffer_payload = create_buffer_payload(buffer_mode, payload_type, buffers_obj, amount, bufsize);
    if (!buffer_payload)
        return NULL;

    switch (stream_strategy) {
    case ONESHOT:
    case MULTISHOT:
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for stream strategy");
        return NULL;
    }

    switch (transfer_mode) {
    case NORMAL_TRANSFER:
    case ZERO_COPY:
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

    execution_context_ctx->loop = self;
    Py_INCREF(self);
    execution_context_ctx->buffer_payload = buffer_payload;
    execution_context_ctx->token = NULL;
    execution_context_ctx->payload = NULL;

    ExecutionContext *execution_context = malloc(sizeof(ExecutionContext));
    if (!execution_context) {
        PyErr_NoMemory();
        Py_DECREF(execution_context_ctx);
        return NULL;
    }

    execution_context->buffer_payload = buffer_payload;
    execution_context->buffer_mode = buffer_mode_val;
    execution_context->stream = stream_strategy;
    execution_context->transfer_mode = transfer_mode;
    execution_context_ctx->payload = execution_context;
    return execution_context_ctx;
}

void
free_exec_context(PyObject *capsule) {
    ExecutionContext *ctx = (ExecutionContext *)PyCapsule_GetPointer(capsule, "ExecutionContext");
    if (ctx) {
        free(ctx);
    }
}

ExecutionContext *
clone_execution_context(const ExecutionContext *src) {
    ExecutionContext *new_ctx = malloc(sizeof(ExecutionContext));
    if (!new_ctx) {
        PyErr_NoMemory();
        return NULL;
    }
    *new_ctx = *src;
    return new_ctx;
}

BufferPayload *
_get_buffer(void) {
    const ExecutionContext *execution_context = ContextVar_get(NULL);
    return execution_context->buffer_payload;
}

TransferMode
get_transfer_mode(void) {
    const ExecutionContext *execution_context = ContextVar_get(NULL);
    return execution_context->transfer_mode;
}

StreamStrategy
get_stream_strategy(void) {
    const ExecutionContext *execution_context = ContextVar_get(NULL);
    return execution_context->stream;
}

BufferModeCtx *
BufferModeCtx_enter(BufferModeCtx *self, PyObject *Py_UNUSED(ignored)) {
    ExecutionContext *current_context = ContextVar_get(NULL);
    if (!current_context)
        return NULL;

    ExecutionContext *new_context = clone_execution_context(current_context);
    if (!new_context)
        return NULL;

    new_context->buffer_mode = self->payload;
    new_context->buffer_payload = self->buffer_payload;

    PyObject *capsule = PyCapsule_New(new_context, "ExecutionContext", free_exec_context);
    if (!capsule)
        return NULL;

    PyObject *token = ContextVar_set(capsule);
    Py_DECREF(capsule);
    if (!token) {
        free(new_context);
        return NULL;
    }

    struct io_uring_buf_ring *buf_ring;
    switch (self->buffer_payload->mode) {
    case FIXED:
        if (init_fixed_mode(
                self->loop->ring, self->buffer_payload->vector->iovecs, self->buffer_payload->vector->nr_vecs
            ) < 0) {
            ContextVar_reset(token);
            Py_DECREF(token);
            PyErr_SetString(PyExc_RuntimeWarning, "Can not initialize fixed buffers");
            return NULL;
        };
        FixedBufferIdxRegistry *buffer_idx_registry = buffer_idx_registry_create(
            (unsigned int)self->buffer_payload->amount
        );
        if (!buffer_idx_registry) {
            ContextVar_reset(token);
            Py_DECREF(token);
            PyErr_SetString(PyExc_RuntimeWarning, "Can not initialize provided buffers");
            return NULL;
        }

        self->buffer_payload->idx_registry = buffer_idx_registry;
        self->buffer_payload->vector = NULL;
        self->buffer_payload->payload_type = PAYLOAD_LINEAR;
        break;
    case PROVIDED:
        if (init_provided_mode(
                self->loop->ring,
                self->buffer_payload->linear->buffer,
                (int)self->buffer_payload->linear->len,
                self->buffer_payload->amount,
                self->buffer_payload->bgid
            ) < 0) {
            PyErr_SetString(PyExc_RuntimeWarning, "Can not initialize provided buffers");
            ContextVar_reset(token);
            Py_DECREF(token);
            return NULL;
        }
        break;
    case BUF_RING:
        buf_ring = init_buf_ring_mode(
            self->loop->ring,
            self->buffer_payload->linear->buffer,
            (int)self->buffer_payload->linear->len,
            self->buffer_payload->amount,
            self->buffer_payload->bgid
        );
        self->buffer_payload->buf_ring = buf_ring;
        break;
    case BUF_NO_VAL:
    case NORMAL_BUF:
    default: // do nothing
        ;
    }

    self->token = token;
    Py_INCREF(self);
    return self;
}

PyObject *
BufferModeCtx_exit(BufferModeCtx *self, PyObject *Py_UNUSED(ignored)) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
        return NULL;
    }

    switch (self->buffer_payload->mode) {
    case FIXED:
        close_fixed_mode(self->loop->ring);
        buffer_idx_registry_destroy(self->buffer_payload->idx_registry);
        break;
    case PROVIDED:
        close_provided_mode(self->loop->ring, self->buffer_payload->amount, self->buffer_payload->bgid);
        break;
    case BUF_RING:
        close_buf_ring_mode(
            self->loop->ring, self->buffer_payload->buf_ring, self->buffer_payload->amount, self->buffer_payload->bgid
        );
        self->buffer_payload->buf_ring = NULL;
        break;
    case BUF_NO_VAL:
    case NORMAL_BUF:
    default: // do nothing
        ;
    }
    Py_RETURN_FALSE;
}

void
BufferModeCtx_dealloc(BufferModeCtx *self) {
    PyObject_Free(self);
}

StreamStrategyCtx *
StreamStrategyCtx_enter(StreamStrategyCtx *self, PyObject *Py_UNUSED(ignored)) {
    ExecutionContext const *current_context = ContextVar_get(NULL);
    if (!current_context)
        return NULL;

    ExecutionContext *new_context = clone_execution_context(current_context);
    if (!new_context)
        return NULL;

    new_context->stream = self->payload;

    PyObject *capsule = PyCapsule_New(new_context, "ExecutionContext", free_exec_context);
    if (!capsule) {
        free(new_context);
        return NULL;
    }

    PyObject *token = ContextVar_set(capsule);
    Py_DECREF(capsule);
    if (!token) {
        free(new_context);
        return NULL;
    }

    self->token = token;
    Py_INCREF(self);
    return self;
}

PyObject *
StreamStrategyCtx_exit(StreamStrategyCtx *self, PyObject *Py_UNUSED(ignored)) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
        return NULL;
    }
    Py_RETURN_FALSE;
}

void
StreamStrategyCtx_dealloc(StreamStrategyCtx *self) {
    PyObject_Free(self);
}

TransferModeCtx *
TransferModeCtx_enter(TransferModeCtx *self, PyObject *Py_UNUSED(ignored)) {
    ExecutionContext const *current_context = ContextVar_get(NULL);
    if (!current_context)
        return NULL;

    ExecutionContext *new_context = clone_execution_context(current_context);
    if (!new_context)
        return NULL;

    new_context->transfer_mode = self->payload;

    PyObject *capsule = PyCapsule_New(new_context, "ExecutionContext", free_exec_context);
    if (!capsule) {
        free(new_context);
        return NULL;
    }

    PyObject *token = ContextVar_set(capsule);
    Py_DECREF(capsule);
    if (!token) {
        free(new_context);
        return NULL;
    }

    self->token = token;
    Py_INCREF(self);
    return self;
}

PyObject *
TransferModeCtx_exit(TransferModeCtx *self, PyObject *Py_UNUSED(ignored)) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
        return NULL;
    }
    Py_RETURN_FALSE;
}

void
TransferModeCtx_dealloc(TransferModeCtx *self) {
    PyObject_Free(self);
}

ExecutionContextCtx *
ExecutionContextCtx_enter(ExecutionContextCtx *self, PyObject *Py_UNUSED(ignored)) {
    PyObject *capsule = PyCapsule_New(self->payload, "ExecutionContext", free_exec_context);
    if (!capsule)
        return NULL;

    PyObject *token = ContextVar_set(capsule);
    Py_DECREF(capsule);
    if (!token) {
        PyErr_SetString(PyExc_ValueError, "Error while setting contextvar");
        return NULL;
    }

    struct io_uring_buf_ring *buf_ring;
    switch (self->buffer_payload->mode) {
    case FIXED:
        if (init_fixed_mode(
                self->loop->ring, self->buffer_payload->vector->iovecs, self->buffer_payload->vector->nr_vecs
            ) < 0) {
            ContextVar_reset(token);
            Py_DECREF(token);
            PyErr_SetString(PyExc_RuntimeWarning, "Can not initialize fixed buffers");
            return NULL;
        };
        FixedBufferIdxRegistry *buffer_idx_registry = buffer_idx_registry_create(
            (unsigned int)self->buffer_payload->amount
        );
        if (!buffer_idx_registry) {
            ContextVar_reset(token);
            Py_DECREF(token);
            PyErr_SetString(PyExc_RuntimeWarning, "Can not initialize provided buffers");
            return NULL;
        }

        self->buffer_payload->idx_registry = buffer_idx_registry;
        self->buffer_payload->vector = NULL;
        self->buffer_payload->payload_type = PAYLOAD_LINEAR;
        break;
    case PROVIDED:
        if (init_provided_mode(
                self->loop->ring,
                self->buffer_payload->linear->buffer,
                (int)self->buffer_payload->linear->len,
                self->buffer_payload->amount,
                self->buffer_payload->bgid
            ) < 0) {
            PyErr_SetString(PyExc_RuntimeWarning, "Can not initialize provided buffers");
            ContextVar_reset(token);
            Py_DECREF(token);
            return NULL;
        }
        break;
    case BUF_RING:
        buf_ring = init_buf_ring_mode(
            self->loop->ring,
            self->buffer_payload->linear->buffer,
            (int)self->buffer_payload->linear->len,
            self->buffer_payload->amount,
            self->buffer_payload->bgid
        );
        self->buffer_payload->buf_ring = buf_ring;
        break;
    case BUF_NO_VAL:
    case NORMAL_BUF:
    default: // do nothing
        ;
    }

    self->token = token;
    Py_INCREF(self);
    return self;
}

PyObject *
ExecutionContextCtx_exit(ExecutionContextCtx *self, PyObject *Py_UNUSED(ignored)) {
    int result = ContextVar_reset(self->token);
    Py_CLEAR(self->token);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "Error while resetting contextvar");
        return NULL;
    }
    Py_RETURN_FALSE;
}

void
ExecutionContextCtx_dealloc(ExecutionContextCtx *self) {
    PyObject_Free(self);
}

BufferMode
_validate_buffer_mode(BufferMode mode) {
    switch (mode) {
    case NORMAL_BUF:
    case FIXED:
    case PROVIDED:
    case BUF_RING:
        return mode;
    case BUF_NO_VAL:
        PyErr_SetString(PyExc_ValueError, "Wrong value for buffer mode");
        return BUF_NO_VAL;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for buffer mode");
        return BUF_NO_VAL;
    }
}

PayloadType
_validate_payload_type(PayloadType payload_type) {
    switch (payload_type) {
    case PAYLOAD_LINEAR:
        return payload_type;
    case PAYLOAD_IOVEC:
        return payload_type;
    case PAYLOAD_LINEAR_AND_IOVEC:
        return payload_type;
    case PAYLOAD_TYPE_NO_VAL:
        PyErr_SetString(PyExc_ValueError, "Wrong value for payload type");
        return PAYLOAD_TYPE_NO_VAL;
    default:
        PyErr_SetString(PyExc_ValueError, "Wrong value for payload type");
        return PAYLOAD_TYPE_NO_VAL;
    }
}
