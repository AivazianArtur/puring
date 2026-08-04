#include "sockets.h"

int
prep_socket(struct io_uring *ring, int request_idx, int domain, int type, const struct TimeoutParams timeout_params) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    if (!(domain == AF_INET || domain == AF_INET6)) {
        fprintf(stderr, "Domain or type is not supported: %s\n", strerror(-1));
        return -2;
    }

    io_uring_prep_socket(sqe, domain, type, 0, 0);

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
puring_bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_bind(sqe, fd, addr, addrlen);

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
puring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr,
    socklen_t addrlen,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_connect(sqe, fd, addr, addrlen);

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
puring_listen(struct io_uring *ring, int request_idx, int fd, int backlog, const struct TimeoutParams timeout_params) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_listen(sqe, fd, backlog);

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
puring_accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct sockaddr *addr,
    socklen_t *len,
    int flags,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_accept(sqe, sockfd, addr, len, flags);

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
puring_close_socket(struct io_uring *ring, int request_idx, int sockfd, const struct TimeoutParams timeout_params) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    io_uring_prep_close(sqe, sockfd);

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
puring_send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int is_poll_first,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_send(sqe, sockfd, buf, len, flags);

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
puring_recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    void *buf,
    size_t len,
    int is_poll_first,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_recv(sqe, sockfd, buf, len, flags);

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
puring_sendto(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    const struct sockaddr *addr,
    size_t addrlen,
    int is_poll_first,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_sendto(sqe, sockfd, buf, len, flags, addr, (socklen_t)addrlen);

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
puring_recvfrom(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    void *buf,
    size_t len,
    struct sockaddr *addr,
    socklen_t addrlen,
    int is_poll_first,
    struct msghdr *msg,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    struct iovec *iov = (struct iovec *)(msg + 1);

    memset(msg, 0, sizeof(*msg));
    msg->msg_name = addr;
    msg->msg_namelen = addrlen;

    iov->iov_base = buf;
    iov->iov_len = len;
    msg->msg_iov = iov;
    msg->msg_iovlen = 1;

    msg->msg_control = NULL;
    msg->msg_controllen = 0;

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_recvmsg(sqe, sockfd, msg, (unsigned int)flags);

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
puring_sendmsg(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    const struct sockaddr *addr,
    size_t addrlen,
    int is_poll_first,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    msg.msg_iov = iovecs;
    msg.msg_iovlen = nr_vecs;

    msg.msg_name = (void *)addr;
    msg.msg_namelen = (socklen_t)addrlen;

    // TODO: Anc.data in next versions
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_sendmsg(sqe, sockfd, &msg, (unsigned int)flags);

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
puring_recvmsg(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int is_poll_first,
    const struct TimeoutParams timeout_params
) {
    SQE_WITH_OPTIONAL_TIMEOUT(ring, &timeout_params);
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    msg.msg_iov = iovecs;
    msg.msg_iovlen = nr_vecs;

    // TODO: Anc.data in next versions
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    int flags = 0;
    if (is_poll_first) {
        flags |= IORING_RECVSEND_POLL_FIRST;
    }

    io_uring_prep_recvmsg(sqe, sockfd, &msg, (unsigned int)flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}
