#include "sockets.h"


int prep_socket(
    struct io_uring *ring, 
    int request_idx,
    int domain,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    if (!(domain == AF_INET || domain == AF_INET6))  {
        fprintf(stderr, "Domain or type is not supported: %s\n", strerror(-1));
        return -2;
    }

    io_uring_prep_socket(sqe, domain, SOCK_STREAM, 0, 0);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen,
    const void *buf,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);
    if (!(state == NEW)) {
        fprintf(stderr, "Wrong socket status - should be `NEW`\n");
        return -2;
    }

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


int uring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr, 
    socklen_t addrlen,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);
    if (!(state == NEW || state == BOUND)) {
        fprintf(stderr, "Wrong socket status - should be `NEW` or `BOUND`.\n");
        return -2;
    }

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


int uring_listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);
    if (!(state == BOUND)) {
        fprintf(stderr, "Wrong socket status - should be `BOUND`.\n");
        return -2;
    }

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


int uring_accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	struct sockaddr *addr,
    socklen_t *len,
    int flags,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);
    if (!(state == LISTENING)) {
        fprintf(stderr, "Wrong socket status - should be `LISTENING`.\n");
        return -2;
    }

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


int uring_close_socket(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    // Below are optional
    struct TimeoutParams *timeout_params
) 
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

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


int uring_send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int flags,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);
    if (!(state == CONNECTED)) {
        fprintf(stderr, "Wrong socket status - should be `CONNECTED`.\n");
        return -2;
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


int uring_recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);
    if (!(state == CONNECTED)) {
        fprintf(stderr, "Wrong socket status - should be `CONNECTED`.\n");
        return -2;
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


int uring_sendto(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    const struct sockaddr *addr,
    size_t addrlen,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_sendto(sqe, sockfd, buf, len, flags, addr, addrlen);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_recvfrom(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    struct sockaddr *addr, 
    socklen_t addrlen,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    struct msghdr msg;
    struct iovec iov;

    memset(&msg, 0, sizeof(msg));

    msg.msg_name = addr;
    msg.msg_namelen = sizeof(addr);

    iov.iov_base = buf;
    iov.iov_len = len;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    io_uring_prep_recvmsg(sqe, sockfd, &msg, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}

int uring_sendmsg(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    const struct sockaddr *addr,
    size_t addrlen,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    msg.msg_iov = iovecs;
    msg.msg_iovlen = nr_vecs;

    msg.msg_name = (void *)addr;
    msg.msg_namelen = addrlen;

    // TODO: Anc.data in next versions
    msg.msg_control = NULL;
    msg.msg_controllen = 0;


    io_uring_prep_sendmsg(sqe, sockfd, &msg, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int uring_recvmsg(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);
    struct msghdr msg;

    memset(&msg, 0, sizeof(msg));

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    msg.msg_iov = iovecs;
    msg.msg_iovlen = nr_vecs;

    // TODO: Anc.data in next versions
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    io_uring_prep_recvmsg(sqe, sockfd, &msg, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}

// TODO
// io_uring_prep_send_zc
// io_uring_prep_multishot_accept
