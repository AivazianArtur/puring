#include "python_api/ops/files/files.h"

int
read_dispatcher(
    UringioFile *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream,
    int request_idx,
    int size,
    int offset,
    TimeoutParams timeout_params
) {
    int result;
    int buf_idx;
    switch (buffer_payload->mode) {
    case FIXED:
        buf_idx = get_buffer_idx(buffer_payload->idx_registry);
        buffer_payload->buf_idx = buf_idx;
        result = uringio_read_fixed(
            self->loop->ring,
            request_idx,
            self->fd,
            buffer_payload->linear->buffer,
            (unsigned)size,
            offset,
            buf_idx,
            timeout_params
        );
        break;
    case PROVIDED:
    case BUF_RING:
        if (stream == MULTISHOT) {
            result = uringio_read_multishot(
                self->loop->ring, request_idx, self->fd, offset, buffer_payload->bgid, timeout_params
            );
        } else {
            result = uringio_read_buffer_select(
                self->loop->ring, request_idx, self->fd, (unsigned)size, offset, buffer_payload->bgid, timeout_params
            );
        }
        break;
    case NORMAL_BUF:
    case BUF_NO_VAL:
    default:
        result = uringio_read(
            self->loop->ring,
            request_idx,
            self->fd,
            buffer_payload->linear->buffer,
            (unsigned)size,
            offset,
            timeout_params
        );
    }
    return result;
}

int
readv_dispatcher(
    UringioFile *self,
    BufferPayload *buffer_payload,
    int request_idx,
    int offset,
    int nowait,
    TimeoutParams timeout_params
) {
    int result;
    switch (buffer_payload->mode) {
    case PROVIDED:
    case BUF_RING:
        result = uringio_readv_buffer_select(
            self->loop->ring,
            request_idx,
            self->fd,
            (unsigned int)buffer_payload->linear->len,
            offset,
            buffer_payload->bgid,
            nowait,
            timeout_params
        );
        break;
    case FIXED:
    case NORMAL_BUF:
    case BUF_NO_VAL:
    default:
        result = uringio_readv(
            self->loop->ring,
            request_idx,
            self->fd,
            buffer_payload->vector->iovecs,
            (unsigned int)(buffer_payload->vector->nr_vecs),
            offset,
            nowait,
            timeout_params
        );
    }
    return result;
}

int
write_dispatcher(
    UringioFile *self, BufferPayload *buffer_payload, int request_idx, int offset, TimeoutParams timeout_params
) {
    int result;
    int buf_idx;
    switch (buffer_payload->mode) {
    case FIXED:
        buf_idx = get_buffer_idx(buffer_payload->idx_registry);
        buffer_payload->buf_idx = buf_idx;
        result = uringio_write_fixed(
            self->loop->ring,
            request_idx,
            self->fd,
            buffer_payload->linear->buffer,
            (unsigned)buffer_payload->linear->len,
            offset,
            buf_idx,
            timeout_params
        );
        break;
    case PROVIDED:
    case BUF_RING:
    case NORMAL_BUF:
    case BUF_NO_VAL:
    default:
        result = uringio_write(
            self->loop->ring,
            request_idx,
            self->fd,
            buffer_payload->linear->buffer,
            (unsigned)buffer_payload->linear->len,
            offset,
            timeout_params
        );
    }
    return result;
}
