#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "python_macroses.h"
#include "python_api/buffer/buffer.h"


extern PyTypeObject PuringBufferModeCtxType;
extern PyTypeObject PuringStreamStrategyCtxType;
extern PyTypeObject PuringTransferModeCtxType;
extern PyTypeObject PuringExecutionContextCtxType;


typedef struct PuringLoop PuringLoop;

typedef enum StreamStrategy { ONESHOT, MULTISHOT } StreamStrategy;

typedef enum TransferMode { NORMAL_TRANSFER, ZERO_COPY, BUFFER_POOL } TransferMode;

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

BufferMode
_get_buffer_mode(void);

PayloadType
_get_payload_type(void);

void
BufferModeCtx_aenter(BufferModeCtx *self);

void
BufferModeCtx_aexit(BufferModeCtx *self);

void
BufferModeCtx_dealloc(BufferModeCtx *self);


void
StreamStrategyCtx_aenter(StreamStrategyCtx *self);

void
StreamStrategyCtx_aexit(StreamStrategyCtx *self);

void
StreamStrategyCtx_dealloc(StreamStrategyCtx *self);

void
TransferModeCtx_aenter(TransferModeCtx *self);

void
TransferModeCtx_aexit(TransferModeCtx *self);

void
TransferModeCtx_dealloc(TransferModeCtx *self);

void
ExecutionContextCtx_aenter(ExecutionContextCtx *self);

void
ExecutionContextCtx_aexit(ExecutionContextCtx *self);

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

void
ContextVar_dealloc(void);
