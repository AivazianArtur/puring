#pragma once

#include <linux/openat2.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "liburing.h"

#include "macroses.h"
#include "timer/timer.h"

int
prep_socket(struct io_uring *ring, int request_idx, int domain, const struct TimeoutParams timeout_params);

int
puring_bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);

int
puring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr,
    socklen_t addrlen,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);

int
puring_listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);

int
puring_accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct sockaddr *addr,
    socklen_t *len,
    int flags,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);

int
puring_close_socket(struct io_uring *ring, int request_idx, int sockfd, const struct TimeoutParams timeout_params);

int
puring_send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int is_poll_first,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);

int
puring_recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    void *buf,
    size_t len,
    int is_poll_first,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);

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
);

int
puring_recvfrom(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    void *buf,
    size_t len,
    struct sockaddr *addr,
    // socklen_t addrlen,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

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
);

int
puring_recvmsg(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

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
);

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
);

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
);

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
);

int
puring_recv_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    size_t len,
    int is_poll_first,
    int bgid,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);


int
puring_recvmsg_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    int is_poll_first,
    int bgid,
    SOCKET_STATES state,
    const struct TimeoutParams timeout_params
);
