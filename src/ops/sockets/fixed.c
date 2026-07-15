#include "sockets.h"

int
puring_recv_fixed(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    void *buf,
    size_t len,
    int is_poll_first,
    int buf_index,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);
    if (!(state == CONNECTED || state == ACCEPTING)) {
        fprintf(stderr, "Wrong socket status - should be `CONNECTED`.\n");
        return -2;
    }

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_recv(sqe, sockfd, buf, len, flags);
    sqe->buf_index = (__u16)buf_index;
    sqe->ioprio |= IORING_RECVSEND_FIXED_BUF;
    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}

int
puring_send_fixed(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int is_poll_first,
    int buf_index,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);
    if (!(state == CONNECTED)) {
        fprintf(stderr, "Wrong socket status - should be `CONNECTED`.\n");
        return -2;
    }

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_send_zc_fixed(sqe, sockfd, buf, len, flags, 0, (unsigned)buf_index);
    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}

int
puring_sendmsg_fixed(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    const struct sockaddr *addr,
    size_t addrlen,
    int is_poll_first,
    int buf_index,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iovecs;
    msg.msg_iovlen = nr_vecs;
    msg.msg_name = (void *)addr;
    msg.msg_namelen = (socklen_t)addrlen;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_sendmsg_zc(sqe, sockfd, &msg, (unsigned int)flags);
    sqe->buf_index = (__u16)buf_index;
    sqe->ioprio |= IORING_RECVSEND_FIXED_BUF;

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}

int
puring_recvmsg_fixed(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int is_poll_first,
    int buf_index,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iovecs;
    msg.msg_iovlen = nr_vecs;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_recvmsg(sqe, sockfd, &msg, (unsigned int)flags);
    sqe->buf_index = (__u16)buf_index;
    sqe->ioprio |= IORING_RECVSEND_FIXED_BUF;

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);
    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}
