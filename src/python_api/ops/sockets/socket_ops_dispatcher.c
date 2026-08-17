#include "python_api/ops/sockets/sockets.h"

int
accept_dispatcher(
    AioUringSocket *self,
    StreamStrategy stream_strategy,
    int request_idx,
    struct sockaddr *addr,
    socklen_t *len,
    TimeoutParams timeout_params
) {
    int result;
    // TODO: correct flags handling
    int flags = 0;

    switch (stream_strategy) {
    case MULTISHOT:
        result = aio_uring_accept_multishot(
            self->loop->ring, request_idx, self->sock_fd, (struct sockaddr *)addr, len, flags, timeout_params
        );
        break;
    case ONESHOT:
    default:
        result = aio_uring_accept(
            self->loop->ring, request_idx, self->sock_fd, (struct sockaddr *)addr, len, flags, timeout_params
        );
    }
    return result;
}

int
recv_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int request_idx,
    int is_poll_first,
    TimeoutParams timeout_params
) {
    int result;
    // int buf_idx;
    switch (buffer_payload->mode) {
    // Works on modern distros with Jule2026 path with https://lwn.net/Articles/1077014/
    // case FIXED:
    //     buf_idx = get_buffer_idx(buffer_payload->idx_registry);
    //     buffer_payload->buf_idx = buf_idx;
    //     result = aio_uring_recv_fixed(
    //         self->loop->ring,
    //         request_idx,
    //         self->sock_fd,
    //         buffer_payload->linear->buffer,
    //         buffer_payload->linear->len,
    //         is_poll_first,
    //         buf_idx,
    //         timeout_params
    //     );
    //     break;
    case PROVIDED:
    case BUF_RING:
        if (stream_strategy == MULTISHOT) {
            result = aio_uring_recv_multishot(
                self->loop->ring,
                request_idx,
                self->sock_fd,
                buffer_payload->linear->len,
                buffer_payload->bgid,
                is_poll_first,
                timeout_params
            );
        } else {
            result = aio_uring_recv_buffer_select(
                self->loop->ring,
                request_idx,
                self->sock_fd,
                buffer_payload->linear->len,
                buffer_payload->bgid,
                is_poll_first,
                timeout_params
            );
        }
        break;
    case NORMAL_BUF:
    case BUF_NO_VAL:
    case FIXED: // TEMP
    default:
        result = aio_uring_recv(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->linear->buffer,
            buffer_payload->linear->len,
            is_poll_first,
            timeout_params
        );
    }
    return result;
}

int
recvmsg_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    StreamStrategy stream_strategy,
    int request_idx,
    int is_poll_first,
    struct msghdr *msghdr,
    TimeoutParams timeout_params
) {
    int result;
    // int buf_idx;
    switch (buffer_payload->mode) {
    // Same as for recv
    // case FIXED:
    //     buf_idx = get_buffer_idx(buffer_payload->idx_registry);
    //     buffer_payload->buf_idx = buf_idx;
    //     result = aio_uring_recvmsg_fixed(
    //         self->loop->ring,
    //         request_idx,
    //         self->sock_fd,
    //         buffer_payload->vector->iovecs,
    //         (unsigned int)buffer_payload->vector->nr_vecs,
    //         is_poll_first,
    //         buf_idx,
    //         timeout_params
    //     );
    //     break;
    case PROVIDED:
    case BUF_RING:
        if (stream_strategy == MULTISHOT) {
            result = aio_uring_recvmsg_multishot(
                self->loop->ring,
                request_idx,
                self->sock_fd,
                buffer_payload->bgid,
                is_poll_first,
                msghdr,
                timeout_params
            );
        } else {
            result = aio_uring_recvmsg_buffer_select(
                self->loop->ring, request_idx, self->sock_fd, buffer_payload->bgid, is_poll_first, timeout_params
            );
        }
        break;
    case NORMAL_BUF:
    case BUF_NO_VAL:
    case FIXED: // TEMP
    default:
        result = aio_uring_recvmsg(
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

int
send_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    TransferMode transfer_mode,
    int request_idx,
    int is_poll_first,
    TimeoutParams timeout_params
) {
    int result;

    if (transfer_mode == ZERO_COPY) {
        if (buffer_payload->mode == FIXED) {
            int buf_idx;
            buf_idx = get_buffer_idx(buffer_payload->idx_registry);
            buffer_payload->buf_idx = buf_idx;
            result = aio_uring_send_zc_fixed(
                self->loop->ring,
                request_idx,
                self->sock_fd,
                buffer_payload->linear->buffer,
                buffer_payload->linear->len,
                is_poll_first,
                buf_idx,
                timeout_params
            );
        } else {
            result = aio_uring_send_zc(
                self->loop->ring,
                request_idx,
                self->sock_fd,
                buffer_payload->linear->buffer,
                buffer_payload->linear->len,
                is_poll_first,
                timeout_params
            );
        }
    } else {
        result = aio_uring_send(
            self->loop->ring,
            request_idx,
            self->sock_fd,
            buffer_payload->linear->buffer,
            (unsigned)buffer_payload->linear->len,
            is_poll_first,
            timeout_params
        );
    }
    return result;
}

int
sendmsg_dispatcher(
    AioUringSocket *self,
    BufferPayload *buffer_payload,
    TransferMode transfer_mode,
    int request_idx,
    const struct sockaddr *addr,
    size_t addrlen,
    int is_poll_first,
    TimeoutParams timeout_params
) {
    int result;
    int buf_idx;
    switch (transfer_mode) {
    case ZERO_COPY:
        buf_idx = get_buffer_idx(buffer_payload->idx_registry);
        buffer_payload->buf_idx = buf_idx;
        result = aio_uring_sendmsg_zc(
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
    case NORMAL_TRANSFER:
    default:
        result = aio_uring_sendmsg(
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
