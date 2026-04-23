#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/openat2.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "liburing.h"

#include "timer/timer.h"
#include "macroses.h"


int prep_socket(
    struct io_uring *ring, 
    int request_idx,
    int domain,
    int type,
    // Below are optional
    struct TimeoutParams *timeout_params
);


int uring_bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen, 
    const void *buf,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr, 
    socklen_t addrlen,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    socklen_t *len,
    int flags,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_close_socket(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    // Below are optional
    struct TimeoutParams *timeout_params
);
