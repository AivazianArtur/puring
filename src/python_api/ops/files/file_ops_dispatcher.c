#include "files.h"

int
read_dispatcher(
) {
    switch (buffer_payload->mode) {
    case FIXED:
        int buf_idx = _get_buffer_index(); // Наверно внутрь убрать
        result = puring_read_fixed(
            self->loop->ring, request_idx, self->fd, buffer_payload->linear->buffer, (unsigned)size, offset, buf_idx, &timeout_params
        );
        break;
    case PROVIDED:
    case BUF_RING:
        result = puring_read_buffer_select(
            self->loop->ring, request_idx, self->fd, buffer_payload->linear->buffer, (unsigned)size, offset, &timeout_params
        );
    case NORMAL_BUF:
    default:
        result = uring_read(
            self->loop->ring, request_idx, self->fd, buffer_payload->linear->buffer, (unsigned)size, offset, &timeout_params
        );
    }
}
