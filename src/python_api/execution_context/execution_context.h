#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include "loop.h"
#include "python_macroses.h"

typedef enum BufferMode { NORMAL_BUFFER, FIXED, PROVIDED } BufferMode;

typedef enum StreamStrategy { ONESHOT, MULTISHOT } StreamStrategy;

typedef enum TransferMode { NORMAL_TRANSFER, ZERO_COPY, BUFFER_POOL } TransferMode;

typedef struct ExecutionContext {
    BufferMode buffers;
    StreamStrategy stream;
    TransferMode transfer_mode;
} ExecutionContext;

typedef struct BufferModeCtx {
    PyObject_HEAD
    PuringLoop *loop;
    BufferMode *payload; 
} BufferModeCtx;

typedef struct StreamStrategyCtx {
    PyObject_HEAD
    PuringLoop *loop;
    StreamStrategy *payload; 
} StreamStrategyCtx;

typedef struct TransferModeCtx {
    PyObject_HEAD
    PuringLoop *loop;
    TransferMode *payload; 
} TransferModeCtx;

typedef struct ExecutionContextCtx {
    PyObject_HEAD
    PuringLoop *loop;
    ExecutionContext *payload; 
} ExecutionContextCtx;

PyObject *
PuringLoop_buffer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_stream_strategy(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_transfer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_execution_context(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
create_buffer_mode_enum(void);

PyObject *
create_stream_strategy_enum(void);

PyObject *
create_transfer_mode_enum(void);

PyObject *
BufferMode_aenter(BufferModeCtx *self, PyObject *args, PyObject *kwargs);

PyObject *
BufferMode_aexit(BufferModeCtx *self, PyObject *args, PyObject *kwargs);

PyObject *
StreamStrategy_aenter(StreamStrategyCtx *self, PyObject *args, PyObject *kwargs);

PyObject *
StreamStrategy_aexit(StreamStrategyCtx *self, PyObject *args, PyObject *kwargs);

PyObject *
TransferMode_aenter(TransferModeCtx *self, PyObject *args, PyObject *kwargs);

PyObject *
TransferMode_aexit(TransferModeCtx *self, PyObject *args, PyObject *kwargs);
