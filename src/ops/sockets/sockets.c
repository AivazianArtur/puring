#include "sockets.h"


int tcp_socket(struct io_uring *ring, int request_idx) 
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    int domain = AF_INET; // AF_INET6;
    int type = SOCK_STREAM;
    int protocol = IPPROTO_TCP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int udp_socket(struct io_uring *ring, int request_idx)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    int domain = AF_INET;
    int type = SOCK_DGRAM;
    int protocol = IPPROTO_UDP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int unix_stream(struct io_uring *ring, int request_idx)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    int domain = AF_UNIX;
    int type = SOCK_STREAM;
    int protocol = IPPROTO_UDP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int unix_dgram(struct io_uring *ring, int request_idx)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    int domain = AF_UNIX;
    int type = SOCK_DGRAM;
    int protocol = IPPROTO_UDP;
    unsigned int flags = 0;

    io_uring_prep_socket(sqe, domain, type, protocol, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen, 
    const void *buf,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    io_uring_prep_bind(sqe, fd, addr, addrlen);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    io_uring_prep_listen(sqe, fd, backlog);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr, 
    socklen_t addrlen,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    io_uring_prep_connect(sqe, fd, addr, addrlen);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int flags,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    io_uring_prep_send(sqe, sockfd, buf, len, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    io_uring_prep_recv(sqe, sockfd, buf, len, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags,
)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    io_uring_prep_accept(sqe, sockfd, buf, len, flags);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}


int close(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
) 
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "SQE is not available\n");
        return -1;
    }

    io_uring_prep_close(sqe, sockfd);

    void *rings_data_pointer = (void *)(uintptr_t)request_idx;
    io_uring_sqe_set_data(sqe, rings_data_pointer);

    io_uring_submit(ring);
    return 0;
}

// TOOD
// io_uring_prep_send_zc
// io_uring_prep_multishot_accept
// Vector ops
// io_uring_prep_sendmsg
// io_uring_prep_recvmsg

// init: raw_socket, packet_socket
