#include "sockets.h"

int
puring_recv_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    size_t len,
    int bgid,
    int is_poll_first,
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

    io_uring_prep_recv(sqe, sockfd, NULL, len, flags);

    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = (__u16)bgid;

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
puring_recvmsg_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    int is_poll_first,
    int bgid,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    if (!(state == CONNECTED || state == ACCEPTING)) {
        fprintf(stderr, "Wrong socket status - should be `CONNECTED`.\n");
        return -2;
    }

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_recvmsg(sqe, sockfd, &msg, (unsigned)flags);

    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = (__u16)bgid;

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }

    return 1;
}
