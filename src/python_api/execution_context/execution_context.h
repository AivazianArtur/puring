#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdio.h>
#include "loop.h"

typedef enum BufferMode { NORMAL_BUFFER, FIXED, PROVIDED } BufferMode;

typedef enum StreamStrategy { ONESHOT, MULTISHOT } StreamStrategy;

typedef enum TransferMode { NORMAL_TRANSFER, ZERO_COPY, BUFFER_POOL } TransferMode;

typedef struct ExecutionContext {
    BufferMode buffers;
    StreamStrategy stream;
    TransferMode transfer_mode;
} ExecutionContext;

PyObject *
PuringLoop_buffer_mode_aenter(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_buffer_mode_aexit(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_stream_strategy_aenter(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_stream_strategy_aexit(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_transfer_mode_aenter(PuringLoop *self, PyObject *args, PyObject *kwargs);

PyObject *
PuringLoop_transfer_mode_aexit(PuringLoop *self, PyObject *args, PyObject *kwargs);


PyObject *
create_buffer_mode_enum(void);

PyObject *
create_stream_strategy_enum(void);

PyObject *
create_transfer_mode_enum(void);
