#include "files.h"

int
read_dispatcher(
    PuringFile *self, BufferPayload *buffer_payload, int request_idx, int size, int offset, TimeoutParams timeout_params
) {
    int result;
    switch (buffer_payload->mode) {
    case FIXED:
        int buf_idx = _get_buffer_index(); // Наверно внутрь убрать
        result = puring_read_fixed(
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
        result = puring_read_buffer_select(
            self->loop->ring, request_idx, self->fd, (unsigned)size, offset, buffer_payload->bgid, timeout_params
        );
        break;
    case NORMAL_BUF:
    default:
        result = uring_read(
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
    PuringFile *self,
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
        result = puring_readv_buffer_select(
            self->loop->ring,
            request_idx,
            self->fd,
            buffer_payload->linear->len,
            offset,
            buffer_payload->bgid,
            nowait,
            timeout_params
        );
        break;
    case FIXED:
    case NORMAL_BUF:
    default:
        result = uring_readv(
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
    PuringFile *self, BufferPayload *buffer_payload, int request_idx, int offset, TimeoutParams timeout_params
) {
    int result;
    switch (buffer_payload->mode) {
    case FIXED:
        int buf_idx = _get_buffer_index(); // Наверно внутрь убрать
        result = puring_write_fixed(
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
    default:
        result = uring_write(
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
