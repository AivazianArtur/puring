#include "sockets.h"

int
recv_dispatcher(
    PuringSocket *self, BufferPayload *buffer_payload, int request_idx, int is_poll_first, TimeoutParams timeout_params
) {
    int result;
    switch (buffer_payload->mode) {
    case FIXED:
        int buf_idx = _get_buffer_index(); // Наверно внутрь убрать
        result = puring_recv_fixed(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->linear->buffer,
            buffer_payload->linear->len,
            is_poll_first,
            buf_idx,
            self->state,
            timeout_params
        );
        break;
    case PROVIDED:
    case BUF_RING:
        result = puring_recv_buffer_select(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->linear->len,
            is_poll_first,
            buffer_payload->bgid,
            self->state,
            timeout_params
        );
        break;
    case NORMAL_BUF:
    default:
        result = uring_recv(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->linear->buffer,
            buffer_payload->linear->len,
            is_poll_first,
            self->state,
            timeout_params
        );
    }
    return result;
}

int
send_dispatcher(
    PuringSocket *self, BufferPayload *buffer_payload, int request_idx, int is_poll_first, TimeoutParams timeout_params
) {
    int result;
    switch (buffer_payload->mode) {
    case FIXED:
        int buf_idx = _get_buffer_index(); // Наверно внутрь убрать
        result = puring_send_fixed(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->linear->buffer,
            buffer_payload->linear->len,
            is_poll_first,
            buf_idx,
            self->state,
            timeout_params
        );
        break;
    case PROVIDED:
    case BUF_RING:
    case NORMAL_BUF:
    default:
        result = uring_send(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->linear->buffer,
            (unsigned)buffer_payload->linear->len,
            is_poll_first,
            self->state,
            timeout_params
        );
    }
    return result;
}

int
sendmsg_dispatcher(
    PuringSocket *self,
    BufferPayload *buffer_payload,
    int request_idx,
    const struct sockaddr *addr,
    size_t addrlen,
    int is_poll_first,
    TimeoutParams timeout_params
) {
    int result;
    switch (buffer_payload->mode) {
    case FIXED:
        int buf_idx = _get_buffer_index(); // Наверно внутрь убрать
        result = puring_sendmsg_fixed(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->vector->iovecs,
            (unsigned int)buffer_payload->vector->nr_vecs,
            addr,
            addrlen,
            is_poll_first,
            buf_idx,
            timeout_params
        );
        break;
    case PROVIDED:
    case BUF_RING:
    case NORMAL_BUF:
    default:
        result = uring_sendmsg(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->vector->iovecs,
            (unsigned int)buffer_payload->vector->nr_vecs,
            addr,
            addrlen,
            is_poll_first,
            timeout_params
        );
    }
    return result;
}

int
recvmsg_dispatcher(
    PuringSocket *self, BufferPayload *buffer_payload, int request_idx, int is_poll_first, TimeoutParams timeout_params
) {
    int result;
    switch (buffer_payload->mode) {
    case FIXED:
        int buf_idx = _get_buffer_index(); // Наверно внутрь убрать
        result = puring_recvmsg_fixed(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->vector->iovecs,
            (unsigned int)buffer_payload->vector->nr_vecs,
            is_poll_first,
            buf_idx,
            timeout_params
        );
        break;
    case PROVIDED:
    case BUF_RING:
        result = puring_recvmsg_buffer_select(
            self->loop->ring, request_idx, self->sock_fd, is_poll_first, self->state, timeout_params
        );
        break;
    case NORMAL_BUF:
    default:
        result = uring_recvmsg(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->vector->iovecs,
            (unsigned int)buffer_payload->vector->nr_vecs,
            is_poll_first,
            timeout_params
        );
    }
    return result;
}
