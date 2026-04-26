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


typedef enum SOCKET_STATES {
  NEW,
  BOUND,
  LISTENING,
  CONNECTED,
  ACCEPTED,
  CLOSED
} SOCKET_STATES;


int prep_socket(
    struct io_uring *ring, 
    int request_idx,
    int domain,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_bind(
    struct io_uring *ring,
    int request_idx,
    int fd,
    const struct sockaddr *addr,
    const void *buf,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_connect(
    struct io_uring *ring,
    int request_idx,
    int fd,
    struct sockaddr *addr, 
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_listen(
    struct io_uring *ring,
    int request_idx,
    int fd,
    int backlog,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
);

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
);

int uring_close_socket(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
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
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_recv(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    int flags,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_sendto(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
    const void *buf,
    size_t len,
    const struct sockaddr *addr,
    int flags,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
);

int uring_recvfrom(
    struct io_uring *ring,
    int request_idx,
    int sockfd,
	void *buf,
    size_t len,
    int flags,
    SOCKET_STATES state,
    // Below are optional
    struct TimeoutParams *timeout_params
);
