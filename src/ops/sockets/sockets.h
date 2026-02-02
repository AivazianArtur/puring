#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <linux/openat2.h>

#include "liburing.h"
#include "loop.h"


/* Functions */
// Init functions - their Future become socket, so next works with socket
int tcp_socket(struct io_uring *ring, int request_idx);
int udp_socket(struct io_uring *ring, int request_idx);
int unix_stream(struct io_uring *ring, int request_idx);
int unix_dgram(struct io_uring *ring, int request_idx);


int bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen, 
    const void *buf,
);

int listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog,
);

int connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr, 
    socklen_t addrlen,
);

int send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int flags,
);

int recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags,
);

int accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags,
);

int close(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
);
