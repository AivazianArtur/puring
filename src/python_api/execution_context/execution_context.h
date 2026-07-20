#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "python_macroses.h"
#include "buffer_controllers/buffer_controllers.h"
#include "python_api/buffers/buffers.h"
#include "python_api/execution_context/execution_context_enums.h"

extern PyTypeObject PuringBufferModeCtxType;
extern PyTypeObject PuringStreamStrategyCtxType;
extern PyTypeObject PuringTransferModeCtxType;
extern PyTypeObject PuringExecutionContextCtxType;


typedef struct PuringLoop PuringLoop;

typedef struct ExecutionContext {
    PyObject_HEAD BufferMode buffer_mode;
    StreamStrategy stream;
    TransferMode transfer_mode;
    BufferPayload *buffer_payload;
} ExecutionContext;

typedef struct BufferModeCtx {
    PyObject_HEAD PuringLoop *loop;
    BufferMode payload;
    PyObject *token;
    BufferPayload *buffer_payload;
} BufferModeCtx;

typedef struct StreamStrategyCtx {
    PyObject_HEAD PuringLoop *loop;
    StreamStrategy payload;
    PyObject *token;
} StreamStrategyCtx;

typedef struct TransferModeCtx {
    PyObject_HEAD PuringLoop *loop;
    TransferMode payload;
    PyObject *token;
} TransferModeCtx;

typedef struct ExecutionContextCtx {
    PyObject_HEAD PuringLoop *loop;
    ExecutionContext *payload;
    PyObject *token;
    BufferPayload *buffer_payload;
} ExecutionContextCtx;

BufferModeCtx *
PuringLoop_buffer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs);

StreamStrategyCtx *
PuringLoop_stream_strategy(PuringLoop *self, PyObject *args, PyObject *kwargs);

TransferModeCtx *
PuringLoop_transfer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs);

ExecutionContextCtx *
PuringLoop_execution_context(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
create_buffer_mode_enum(void);

PyObject *
create_stream_strategy_enum(void);

PyObject *
create_transfer_mode_enum(void);

BufferPayload *
_get_buffer(void);

TransferMode
get_transfer_mode(void);

StreamStrategy
get_stream_strategy(void);

BufferModeCtx *
BufferModeCtx_enter(BufferModeCtx *self, PyObject *Py_UNUSED(ignored));

PyObject *
BufferModeCtx_exit(BufferModeCtx *self, PyObject *Py_UNUSED(ignored));

void
BufferModeCtx_dealloc(BufferModeCtx *self);

StreamStrategyCtx *
StreamStrategyCtx_enter(StreamStrategyCtx *self, PyObject *Py_UNUSED(ignored));

PyObject *
StreamStrategyCtx_exit(StreamStrategyCtx *self, PyObject *Py_UNUSED(ignored));

void
StreamStrategyCtx_dealloc(StreamStrategyCtx *self);

TransferModeCtx *
TransferModeCtx_enter(TransferModeCtx *self, PyObject *Py_UNUSED(ignored));

PyObject *
TransferModeCtx_exit(TransferModeCtx *self, PyObject *Py_UNUSED(ignored));

void
TransferModeCtx_dealloc(TransferModeCtx *self);

ExecutionContextCtx *
ExecutionContextCtx_enter(ExecutionContextCtx *self, PyObject *Py_UNUSED(ignored));

PyObject *
ExecutionContextCtx_exit(ExecutionContextCtx *self, PyObject *Py_UNUSED(ignored));

void
ExecutionContextCtx_dealloc(ExecutionContextCtx *self);

PyObject *
ContextVar_init(void);

PyObject *
ContextVar_set(PyObject *value);

int
ContextVar_reset(PyObject *token);

ExecutionContext *
ContextVar_get(PyObject *default_val);

BufferMode
_validate_buffer_mode(BufferMode mode);

PayloadType
_validate_payload_type(PayloadType payload_type);
