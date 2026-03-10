#pragma once

#include <stdio.h>
#include <string.h>
#include <linux/openat2.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "liburing.h"


/* Functions */
// Init functions - their Future become socket, so next works with socket
int tcp_socket(struct io_uring *ring, int request_idx);
int udp_socket(struct io_uring *ring, int request_idx);
int unix_stream(struct io_uring *ring, int request_idx);
int unix_dgram(struct io_uring *ring, int request_idx);


int uring_bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    socklen_t addrlen, 
    const void *buf
);

int uring_listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog
);

int uring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr, 
    socklen_t addrlen
);

int uring_send(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    int flags
);

int uring_recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags
);

int uring_accept(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    socklen_t *len,
    int flags
);

int uring_close_socket(
    struct io_uring *ring,
    int request_idx,
    int sockfd
);
