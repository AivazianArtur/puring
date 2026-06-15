#include "execution_context.h"

PyObject *
PuringLoop_buffer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char mode;
    static const char *kwlist[] = {"mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", (char **)kwlist, &mode)) {
        return NULL;
    }
    switch(mode) {
        case NORMAL_BUFFER:
            printf('stub');
            break;        case FIXED:
            printf('stub');
            break;
        case PROVIDED:
            printf('stub');
            break;
        default:
            printf('NORMAL BUFFER stub');
    }

}

PyObject *
PuringLoop_stream_strategy(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char strategy = ONESHOT;
    static const char *kwlist[] = {"strategy", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", (char **)kwlist, &strategy)) {
        return NULL;
    }
    switch(strategy) {
        case ONESHOT:
            printf('stub');
            break;
        case MULTISHOT:
            printf('stub');
            break;
        default:
            printf('stub ONESHOT');
    }
}

PyObject *
PuringLoop_transfer_mode(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char mode = NORMAL_TRANSFER;
    static const char *kwlist[] = {"mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s", (char **)kwlist, &mode)) {
        return NULL;
    }
    switch(mode) {
        case NORMAL_TRANSFER:
            printf('stub');
            break;
        case ZERO_COPY:
            printf('stub');
            break;
        case BUFFER_POOL:
            printf('stub');
            break;
        default:
            printf('stub');
    }
}

PyObject *
PuringLoop_execution_context(PuringLoop *self, PyObject *args, PyObject *kwargs) {
    ASSERT_LOOP_THREAD(self);
    ASSERT_RING_LOOP_IS_CLOSING(self);

    char buffer_mode = NORMAL_BUFFER;
    char stream_strategy = ONESHOT;
    char transfer_mode = NORMAL_TRANSFER;
    static const char *kwlist[] = {"buffer_mode", "stream_strategy", "transfer_mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "|sss",
        (char **)kwlist,
        &buffer_mode
    )) {
        return NULL;
    }
    switch(buffer_mode) {
        case NORMAL_BUFFER:
            printf('stub');
            break;        case FIXED:
            printf('stub');
            break;
        case PROVIDED:
            printf('stub');
            break;
        default:
            printf('NORMAL BUFFER stub');
    }
    switch(stream_strategy) {
        case ONESHOT:
            printf('stub');
            break;
        case MULTISHOT:
            printf('stub');
            break;
        default:
            printf('stub ONESHOT');
    }
    switch(transfer_mode) {
        case NORMAL_TRANSFER:
            printf('stub');
            break;
        case ZERO_COPY:
            printf('stub');
            break;
        case BUFFER_POOL:
            printf('stub');
            break;
        default:
            printf('stub');
    }
}


PyObject *
BufferMode_aenter(BufferModeCtx *self, PyObject *args, PyObject *kwargs) {}

PyObject *
BufferMode_aexit(BufferModeCtx *self, PyObject *args, PyObject *kwargs) {}

PyObject *
StreamStrategy_aenter(StreamStrategyCtx *self, PyObject *args, PyObject *kwargs) {}

PyObject *
StreamStrategy_aexit(StreamStrategyCtx *self, PyObject *args, PyObject *kwargs) {}

PyObject *
TransferMode_aenter(TransferModeCtx *self, PyObject *args, PyObject *kwargs) {}

PyObject *
TransferMode_aexit(TransferModeCtx *self, PyObject *args, PyObject *kwargs) {}
