#pragma once

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <liburing.h>

#include "ring/ring.h"
#include "ops/files/files.h"


static inline int
wait_one_cqe(struct io_uring *ring) {
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(ring, &cqe);
    if (ret < 0) {
        return ret;
    }
    int res = cqe->res;
    io_uring_cqe_seen(ring, cqe);
    return res;
}

static const TimeoutParams NO_TIMEOUT = {0, 0, false};

static inline char *
make_temp_file(const char *content, size_t len) {
    char *path = strdup("/tmp/puring_ctest_XXXXXX");
    int fd = mkstemp(path);
    if (fd < 0) {
        free(path);
        return NULL;
    }
    if (content && len > 0) {
        ssize_t written = write(fd, content, len);
        (void)written;
    }
    close(fd);
    return path;
}

static inline void
exhaust_sq(struct io_uring *ring) {
    while (io_uring_get_sqe(ring) != NULL) {
    }
}

static inline int
provide_buffers_group(struct io_uring *ring, void *base, unsigned buf_len, unsigned nr, int bgid) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) return -1;
    io_uring_prep_provide_buffers(sqe, base, (int)buf_len, (int)nr, bgid, 0);
    io_uring_sqe_set_data(sqe, NULL);
    if (io_uring_submit(ring) < 0) return -1;

    struct io_uring_cqe *cqe;
    if (io_uring_wait_cqe(ring, &cqe) < 0) return -1;
    int res = cqe->res;
    io_uring_cqe_seen(ring, cqe);
    return res;
}

static inline int
make_tcp_pair(int *client_fd, int *server_fd) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { close(listen_fd); return -1; }
    if (listen(listen_fd, 1) < 0) { close(listen_fd); return -1; }

    socklen_t alen = sizeof(addr);
    if (getsockname(listen_fd, (struct sockaddr *)&addr, &alen) < 0) { close(listen_fd); return -1; }

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd < 0) { close(listen_fd); return -1; }
    if (connect(cfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { close(cfd); close(listen_fd); return -1; }

    int sfd = accept(listen_fd, NULL, NULL);
    close(listen_fd);
    if (sfd < 0) { close(cfd); return -1; }

    *client_fd = cfd;
    *server_fd = sfd;
    return 0;
}

static inline int
make_udp_server(struct sockaddr_in *out_addr) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { close(fd); return -1; }

    socklen_t alen = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &alen) < 0) { close(fd); return -1; }

    *out_addr = addr;
    return fd;
}
