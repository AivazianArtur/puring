#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include "loop.h"
#include "python_macroses.h"

typedef enum BufferMode { NORMAL_BUFFER, FIXED, PROVIDED } BufferMode;

typedef enum StreamStrategy { ONESHOT, MULTISHOT } StreamStrategy;

typedef enum TransferMode { NORMAL_TRANSFER, ZERO_COPY, BUFFER_POOL } TransferMode;

typedef struct ExecutionContext {
    PyObject_HEAD BufferMode buffer_mode;
    StreamStrategy stream;
    TransferMode transfer_mode;
} ExecutionContext;

typedef struct BufferModeCtx {
    PyObject_HEAD PuringLoop *loop;
    BufferMode payload;
} BufferModeCtx;

typedef struct StreamStrategyCtx {
    PyObject_HEAD PuringLoop *loop;
    StreamStrategy payload;
} StreamStrategyCtx;

typedef struct TransferModeCtx {
    PyObject_HEAD PuringLoop *loop;
    TransferMode payload;
} TransferModeCtx;

typedef struct ExecutionContextCtx {
    PyObject_HEAD PuringLoop *loop;
    ExecutionContext *payload;
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

void
BufferMode_aenter(BufferModeCtx *self);

void
BufferMode_aexit(BufferModeCtx *self);

void
StreamStrategy_aenter(StreamStrategyCtx *self);

void
StreamStrategy_aexit(StreamStrategyCtx *self);

void
TransferMode_aenter(TransferModeCtx *self);

void
TransferMode_aexit(TransferModeCtx *self);

void
ExecutionContext_aenter(ExecutionContextCtx *self);

void
ExecutionContext_aexit(ExecutionContextCtx *self);

PyObject *
ContextVar_init(void);

int
ContextVar_set(PyObject *value);

ExecutionContext *
ContextVar_get(PyObject *default_val);

void
ContextVar_dealloc(void);
