#include "sockets.h"

int
puring_accept_multishot(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct sockaddr *addr,
    socklen_t *len,
    int flags,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_multishot_accept(sqe, sockfd, addr, len, flags);

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
puring_recv_multishot(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    size_t len,
    int bgid,
    int is_poll_first,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    int flags = 0;
    if (is_poll_first)
        flags |= IORING_RECVSEND_POLL_FIRST;

    io_uring_prep_recv_multishot(sqe, sockfd, NULL, len, flags);

    sqe->buf_group = (__u16)bgid;
    sqe->flags |= IOSQE_BUFFER_SELECT;

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
puring_recvmsg_multishot(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    int bgid,
    int is_poll_first,
    struct msghdr *msghdr,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    int flags = 0;
    if (is_poll_first)
        flags |= IORING_RECVSEND_POLL_FIRST;

    io_uring_prep_recvmsg_multishot(sqe, sockfd, msghdr, (unsigned)flags);

    sqe->buf_group = (__u16)bgid;
    sqe->flags |= IOSQE_BUFFER_SELECT;

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }

    return 1;
}

RecvMsgMultishotResult
puring_recvmsg_validate_multishot(void *buf, int buf_len, struct msghdr *msghdr, int len) {
    RecvMsgMultishotResult result = {NULL, 0, true};

    struct io_uring_recvmsg_out *out = io_uring_recvmsg_validate(buf, buf_len, msghdr);
    if (!out)
        return result;

    result.payload = io_uring_recvmsg_payload(out, msghdr);
    result.payload_len = io_uring_recvmsg_payload_length(out, len, msghdr);
    result.is_null = false;
    return result;
}

bool
is_puring_recvmsg_multishot_resubmit_required(const struct io_uring_cqe *cqe) {
    return !(cqe->flags & IORING_CQE_F_MORE);
}
