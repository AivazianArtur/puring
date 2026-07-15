#include "sockets.h"

int
recv_dispatcher(
    PuringSocket *self, BufferPayload *buffer_payload, int request_idx, int is_poll_first, TimeoutParams timeout_params
) {
    int result;
    int buf_idx;
    switch (buffer_payload->mode) {
    case FIXED:
        buf_idx = get_buffer_idx(buffer_payload->idx_registry);
        buffer_payload->buf_idx = buf_idx;
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
    case BUF_NO_VAL:
    default:
        result = puring_recv(
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
    int buf_idx;

    fprintf(stderr, "INSIDE DISPATCHER");
    switch (buffer_payload->mode) {
    case FIXED:
        buf_idx = get_buffer_idx(buffer_payload->idx_registry);
        buffer_payload->buf_idx = buf_idx;
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
    case BUF_NO_VAL:
    default:
        fprintf(stderr, "INSIDE DISPATCHER DEFAULT\n");
        result = puring_send(
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

    fprintf(stderr, "RESULT %d \n", result);
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
    int buf_idx;
    switch (buffer_payload->mode) {
    case FIXED:
        buf_idx = get_buffer_idx(buffer_payload->idx_registry);
        buffer_payload->buf_idx = buf_idx;
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
    case BUF_NO_VAL:
    default:
        result = puring_sendmsg(
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
    int buf_idx;
    switch (buffer_payload->mode) {
    case FIXED:
        buf_idx = get_buffer_idx(buffer_payload->idx_registry);
        buffer_payload->buf_idx = buf_idx;
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
            self->loop->ring,
            request_idx,
            self->sock_fd,
            is_poll_first,
            buffer_payload->bgid,
            self->state,
            timeout_params
        );
        break;
    case NORMAL_BUF:
    case BUF_NO_VAL:
    default:
        result = puring_recvmsg(
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
