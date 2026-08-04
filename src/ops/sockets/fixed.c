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
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

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
