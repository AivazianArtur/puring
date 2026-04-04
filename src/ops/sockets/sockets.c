#include "sockets.h"


int tcp_socket(
    struct io_uring *ring, 
    int request_idx,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    int domain = AF_INET; // AF_INET6;
    int type = SOCK_STREAM;
    int protocol = IPPROTO_TCP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }
    return 1;
}


int udp_socket(
    struct io_uring *ring,
    int request_idx,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    int domain = AF_INET;
    int type = SOCK_DGRAM;
    int protocol = IPPROTO_UDP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }

    return 1;
}


int unix_stream(
    struct io_uring *ring,
    int request_idx,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    int domain = AF_UNIX;
    int type = SOCK_STREAM;
    int protocol = IPPROTO_UDP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    int result = io_uring_submit(ring);
    if (result < 0) {
        fprintf(stderr, "io_uring_submit failed: %s\n", strerror(-result));
        return 0;
    }

    return 1;
}


int unix_dgram(
    struct io_uring *ring,
    int request_idx,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    int domain = AF_UNIX;
    int type = SOCK_DGRAM;
    int protocol = IPPROTO_UDP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

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
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

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


int uring_listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

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


int uring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr, 
    socklen_t addrlen,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

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


int uring_send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

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
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

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


int uring_accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    socklen_t *len,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
)
{
    SQE_WITH_OPTIONAL_TIMEOUT(ring, timeout_params);

    io_uring_prep_accept(sqe, sockfd, buf, len, flags);

    // socklen_t addrlen = sizeof(addr);
    // io_uring_prep_accept(sqe, sockfd, (struct sockaddr *)&addr, &addrlen, flags);

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

// TODO
// io_uring_prep_send_zc
// io_uring_prep_multishot_accept
// Vector ops
// io_uring_prep_sendmsg
// io_uring_prep_recvmsg

// init: raw_socket, packet_socket
