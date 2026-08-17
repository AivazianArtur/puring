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

typedef struct RecvMsgMultishotResult {
    void *payload;
    size_t payload_len;
    bool is_null;
} RecvMsgMultishotResult;

int
prep_socket(struct io_uring *ring, int request_idx, int domain, int type, const struct TimeoutParams timeout_params);

int
aio_uring_bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen,
    const struct TimeoutParams timeout_params
);

int
aio_uring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr,
    socklen_t addrlen,
    const struct TimeoutParams timeout_params
);

int
aio_uring_listen(struct io_uring *ring, int request_idx, int fd, int backlog, const struct TimeoutParams timeout_params);

int
aio_uring_accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct sockaddr *addr,
    socklen_t *len,
    int flags,
    const struct TimeoutParams timeout_params
);

int
aio_uring_close_socket(struct io_uring *ring, int request_idx, int sockfd, const struct TimeoutParams timeout_params);

int
aio_uring_send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

int
aio_uring_recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    void *buf,
    size_t len,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

int
aio_uring_sendto(
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
aio_uring_recvfrom(
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
);

int
aio_uring_sendmsg(
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
aio_uring_recvmsg(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct iovec *iovecs,
    unsigned nr_vecs,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

int
aio_uring_recv_fixed(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    void *buf,
    size_t len,
    int is_poll_first,
    int buf_index,
    const struct TimeoutParams timeout_params
);

int
aio_uring_send_zc(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

int
aio_uring_send_zc_fixed(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int is_poll_first,
    int buf_index,
    const struct TimeoutParams timeout_params
);

int
aio_uring_sendmsg_zc(
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
aio_uring_recvmsg_fixed(
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
aio_uring_recv_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    size_t len,
    int bgid,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

int
aio_uring_recvmsg_buffer_select(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    int is_poll_first,
    int bgid,
    const struct TimeoutParams timeout_params
);

int
aio_uring_accept_multishot(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    struct sockaddr *addr,
    socklen_t *len,
    int flags,
    const struct TimeoutParams timeout_params
);

int
aio_uring_recv_multishot(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    size_t len,
    int bgid,
    int is_poll_first,
    const struct TimeoutParams timeout_params
);

int
aio_uring_recvmsg_multishot(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    int bgid,
    int is_poll_first,
    struct msghdr *msghdr,
    const struct TimeoutParams timeout_params
);

RecvMsgMultishotResult
aio_uring_recvmsg_validate_multishot(void *buf, int buf_len, struct msghdr *msghdr, int len);

bool
is_aio_uring_recvmsg_multishot_resubmit_required(const struct io_uring_cqe *cqe);
