#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "test_helpers.h"
#include "test_utils.h"
#include "ops/sockets/sockets.h"

static const TimeoutParams NO_TO = {0};

/* ---------- prep_socket ---------- */

static void
test_prep_socket__success_creates_tcp_socket(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int ret = prep_socket(&ring, 1, AF_INET, SOCK_STREAM, NO_TO);
    TEST_ASSERT(ret == 1, "prep_socket submit should succeed");

    int fd = wait_one_cqe(&ring);
    TEST_ASSERT(fd >= 0, "returned fd should be non-negative");

    close(fd);
    ring_destroy(&ring);
}

static void
test_prep_socket__unsupported_domain_returns_minus_two(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int ret = prep_socket(&ring, 1, AF_UNIX, SOCK_STREAM, NO_TO);
    TEST_ASSERT(ret == -2, "unsupported domain should return -2 without touching the ring");

    ring_destroy(&ring);
}

/* ---------- bind / listen / connect / accept ---------- */

static void
test_bind_listen_connect_accept__full_flow(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(listen_fd >= 0, "posix socket() failed");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    int bret = puring_bind(&ring, 1, listen_fd, (struct sockaddr *)&addr, sizeof(addr), NEW, NO_TO);
    TEST_ASSERT(bret == 1, "puring_bind submit should succeed");
    int bres = wait_one_cqe(&ring);
    TEST_ASSERT(bres == 0, "bind should succeed");

    socklen_t alen = sizeof(addr);
    TEST_ASSERT(getsockname(listen_fd, (struct sockaddr *)&addr, &alen) == 0, "getsockname failed");

    int lret = puring_listen(&ring, 2, listen_fd, 1, NEW, NO_TO);
    TEST_ASSERT(lret == 1, "puring_listen submit should succeed");
    int lres = wait_one_cqe(&ring);
    TEST_ASSERT(lres == 0, "listen should succeed");

    struct sockaddr_storage peer = {0};
    socklen_t peerlen = sizeof(peer);
    int aret = puring_accept(&ring, 3, listen_fd, (struct sockaddr *)&peer, &peerlen, 0, NEW, NO_TO);
    TEST_ASSERT(aret == 1, "puring_accept submit should succeed");

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(client_fd >= 0, "posix socket() for client failed");

    int cret = puring_connect(&ring, 4, client_fd, (struct sockaddr *)&addr, sizeof(addr), NEW, NO_TO);
    TEST_ASSERT(cret == 1, "puring_connect submit should succeed");

    int accepted_fd = -1;
    for (int i = 0; i < 2; i++) {
        struct io_uring_cqe *cqe;
        TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe) == 0, "wait_cqe failed");
        int idx = (int)(uintptr_t)cqe->user_data;
        int res = cqe->res;
        io_uring_cqe_seen(&ring, cqe);
        if (idx == 3) {
            TEST_ASSERT(res >= 0, "accept should return a valid fd");
            accepted_fd = res;
        } else if (idx == 4) {
            TEST_ASSERT(res == 0, "connect should succeed");
        }
    }
    TEST_ASSERT(accepted_fd >= 0, "accept CQE was not observed");

    close(client_fd);
    close(accepted_fd);
    close(listen_fd);
    ring_destroy(&ring);
}

static void
test_puring_connect__refused_on_closed_port(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int probe_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(probe_fd >= 0, "posix socket() failed");
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    TEST_ASSERT(bind(probe_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0, "bind failed");
    socklen_t alen = sizeof(addr);
    TEST_ASSERT(getsockname(probe_fd, (struct sockaddr *)&addr, &alen) == 0, "getsockname failed");
    close(probe_fd);

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(client_fd >= 0, "posix socket() failed");

    int ret = puring_connect(&ring, 1, client_fd, (struct sockaddr *)&addr, sizeof(addr), NEW, NO_TO);
    TEST_ASSERT(ret == 1, "submit should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -ECONNREFUSED, "expected -ECONNREFUSED on closed port");

    close(client_fd);
    ring_destroy(&ring);
}

/* ---------- send / recv ---------- */

static void
test_puring_send_recv__roundtrip(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int cfd, sfd;
    TEST_ASSERT(make_tcp_pair(&cfd, &sfd) == 0, "make_tcp_pair failed");

    const char *msg = "hello over io_uring";
    int sret = puring_send(&ring, 1, cfd, msg, strlen(msg), 0, CONNECTED, NO_TO);
    TEST_ASSERT(sret == 1, "puring_send submit should succeed");
    int sres = wait_one_cqe(&ring);
    TEST_ASSERT(sres == (int)strlen(msg), "send should report full length");

    char buf[64] = {0};
    int rret = puring_recv(&ring, 2, sfd, buf, sizeof(buf), 0, CONNECTED, NO_TO);
    TEST_ASSERT(rret == 1, "puring_recv submit should succeed");
    int rres = wait_one_cqe(&ring);
    TEST_ASSERT(rres == (int)strlen(msg), "recv should report exactly the sent length");
    TEST_ASSERT(memcmp(buf, msg, strlen(msg)) == 0, "recv content mismatch");

    close(cfd);
    close(sfd);
    ring_destroy(&ring);
}

static void
test_puring_recv__peer_closed_returns_zero(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int cfd, sfd;
    TEST_ASSERT(make_tcp_pair(&cfd, &sfd) == 0, "make_tcp_pair failed");
    close(cfd);

    char buf[16];
    int ret = puring_recv(&ring, 1, sfd, buf, sizeof(buf), 0, CONNECTED, NO_TO);
    TEST_ASSERT(ret == 1, "submit should succeed");
    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 0, "recv on peer-closed socket should return 0 (EOF)");

    close(sfd);
    ring_destroy(&ring);
}

/* ---------- sendto / recvfrom ---------- */

static void
test_puring_sendto_recvfrom__roundtrip(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    struct sockaddr_in server_addr;
    int server_fd = make_udp_server(&server_addr);
    TEST_ASSERT(server_fd >= 0, "make_udp_server failed");

    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    TEST_ASSERT(client_fd >= 0, "posix socket() failed");

    const char *msg = "udp via sendto";

    int sret = puring_sendto(
        &ring, 1, client_fd, msg, strlen(msg), (struct sockaddr *)&server_addr, sizeof(server_addr), 0, NO_TO
    );
    TEST_ASSERT(sret == 1, "puring_sendto submit should succeed");
    int sres = wait_one_cqe(&ring);
    TEST_ASSERT(sres == (int)strlen(msg), "sendto should report full length");

    char buf[64] = {0};
    struct sockaddr_storage peer = {0};

    struct msghdr *msg_hdr = malloc(sizeof(struct msghdr) + sizeof(struct iovec));
    TEST_ASSERT(msg_hdr != NULL, "malloc for msghdr failed");

    int rret = puring_recvfrom(
        &ring, 2, server_fd, buf, sizeof(buf), (struct sockaddr *)&peer, sizeof(peer), 0, msg_hdr, NO_TO
    );
    TEST_ASSERT(rret == 1, "puring_recvfrom submit should succeed");
    int rres = wait_one_cqe(&ring);
    TEST_ASSERT(rres == (int)strlen(msg), "recvfrom should report exactly the sent length");
    TEST_ASSERT(memcmp(buf, msg, strlen(msg)) == 0, "recvfrom content mismatch");

    free(msg_hdr);
    close(client_fd);
    close(server_fd);
    ring_destroy(&ring);
}

/* ---------- sendmsg / recvmsg ---------- */

static void
test_puring_sendmsg_recvmsg__roundtrip_multiple_iovecs(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int cfd, sfd;
    TEST_ASSERT(make_tcp_pair(&cfd, &sfd) == 0, "make_tcp_pair failed");

    char part1[] = "hello ";
    char part2[] = "sendmsg";
    struct iovec send_iov[2] = {
        {.iov_base = part1, .iov_len = strlen(part1)},
        {.iov_base = part2, .iov_len = strlen(part2)},
    };

    int sret = puring_sendmsg(&ring, 1, cfd, send_iov, 2, NULL, 0, 0, NO_TO);
    TEST_ASSERT(sret == 1, "puring_sendmsg submit should succeed");
    int sres = wait_one_cqe(&ring);
    int total = (int)(strlen(part1) + strlen(part2));
    TEST_ASSERT(sres == total, "sendmsg should report full combined length");

    char rbuf1[8] = {0};
    char rbuf2[8] = {0};
    struct iovec recv_iov[2] = {
        {.iov_base = rbuf1, .iov_len = sizeof(rbuf1)},
        {.iov_base = rbuf2, .iov_len = sizeof(rbuf2)},
    };

    int rret = puring_recvmsg(&ring, 2, sfd, recv_iov, 2, 0, NO_TO);
    TEST_ASSERT(rret == 1, "puring_recvmsg submit should succeed");
    int rres = wait_one_cqe(&ring);
    TEST_ASSERT(rres == total, "recvmsg should report full combined length");
    TEST_ASSERT(memcmp(rbuf1, "hello se", 8) == 0, "recvmsg iov1 mismatch");
    TEST_ASSERT(memcmp(rbuf2, "ndmsg", 5) == 0, "recvmsg iov2 mismatch");

    close(cfd);
    close(sfd);
    ring_destroy(&ring);
}

/* ---------- recv_fixed / recvmsg_fixed ---------- */

static void
test_puring_recv_fixed__success(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int cfd, sfd;
    TEST_ASSERT(make_tcp_pair(&cfd, &sfd) == 0, "make_tcp_pair failed");

    char recv_buf[32] = {0};
    struct iovec regs[1] = {{.iov_base = recv_buf, .iov_len = sizeof(recv_buf)}};
    TEST_ASSERT(io_uring_register_buffers(&ring, regs, 1) == 0, "register_buffers failed");

    const char *msg = "fixed recv works";
    TEST_ASSERT(write(cfd, msg, strlen(msg)) == (ssize_t)strlen(msg), "write failed");

    int ret = puring_recv_fixed(&ring, 1, sfd, recv_buf, strlen(msg), 0, /*buf_index=*/0, CONNECTED, NO_TO);
    TEST_ASSERT(ret == 1, "puring_recv_fixed submit should succeed");
    int res = wait_one_cqe(&ring);
    printf("recv_fixed res = %d\n", res);
    TEST_ASSERT(res == (int)strlen(msg), "recv_fixed should report exact length");
    TEST_ASSERT(memcmp(recv_buf, msg, strlen(msg)) == 0, "recv_fixed content mismatch");

    io_uring_unregister_buffers(&ring);
    close(cfd);
    close(sfd);
    ring_destroy(&ring);
}

/* ---------- recv_buffer_select / recvmsg_buffer_select ---------- */

static void
test_puring_recv_buffer_select__success(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int cfd, sfd;
    TEST_ASSERT(make_tcp_pair(&cfd, &sfd) == 0, "make_tcp_pair failed");

    const int bgid = 5;
    char pool[64];
    TEST_ASSERT(provide_buffers_group(&ring, pool, sizeof(pool), 1, bgid) == 0, "provide_buffers failed");

    const char *msg = "buffer select recv";
    TEST_ASSERT(write(cfd, msg, strlen(msg)) == (ssize_t)strlen(msg), "write failed");

    int ret = puring_recv_buffer_select(&ring, 1, sfd, sizeof(pool), bgid, 0, CONNECTED, NO_TO);
    TEST_ASSERT(ret == 1, "puring_recv_buffer_select submit should succeed");

    struct io_uring_cqe *cqe;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe) == 0, "wait_cqe failed");
    TEST_ASSERT(cqe->res == (int)strlen(msg), "recv_buffer_select should report exact length");
    TEST_ASSERT(memcmp(pool, msg, strlen(msg)) == 0, "provided buffer content mismatch");
    io_uring_cqe_seen(&ring, cqe);

    close(cfd);
    close(sfd);
    ring_destroy(&ring);
}

/* ---------- accept_multishot ---------- */

static void
test_puring_accept_multishot__accepts_two_connections(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(listen_fd >= 0, "posix socket() failed");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    TEST_ASSERT(bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0, "bind failed");
    TEST_ASSERT(listen(listen_fd, 4) == 0, "listen failed");
    socklen_t alen = sizeof(addr);
    TEST_ASSERT(getsockname(listen_fd, (struct sockaddr *)&addr, &alen) == 0, "getsockname failed");

    struct sockaddr_storage peer = {0};
    socklen_t peerlen = sizeof(peer);
    int ret = puring_accept_multishot(&ring, 1, listen_fd, (struct sockaddr *)&peer, &peerlen, 0, NEW, NO_TO);
    TEST_ASSERT(ret == 1, "puring_accept_multishot submit should succeed");

    int c1 = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(connect(c1, (struct sockaddr *)&addr, sizeof(addr)) == 0, "connect #1 failed");

    struct io_uring_cqe *cqe1;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe1) == 0, "wait_cqe #1 failed");
    TEST_ASSERT(cqe1->res >= 0, "first accepted fd should be valid");
    int accepted1 = cqe1->res;
    bool more1 = (cqe1->flags & IORING_CQE_F_MORE) != 0;
    io_uring_cqe_seen(&ring, cqe1);
    TEST_ASSERT(more1, "multishot accept should signal IORING_CQE_F_MORE for continued accepts");

    int c2 = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(connect(c2, (struct sockaddr *)&addr, sizeof(addr)) == 0, "connect #2 failed");

    struct io_uring_cqe *cqe2;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe2) == 0, "wait_cqe #2 failed");
    TEST_ASSERT(cqe2->res >= 0, "second accepted fd should be valid");
    int accepted2 = cqe2->res;
    io_uring_cqe_seen(&ring, cqe2);

    close(accepted1);
    close(accepted2);
    close(c1);
    close(c2);
    close(listen_fd);
    ring_destroy(&ring);
}

/* ---------- recv_multishot ---------- */

static void
test_puring_recv_multishot__receives_two_messages(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int cfd, sfd;
    TEST_ASSERT(make_tcp_pair(&cfd, &sfd) == 0, "make_tcp_pair failed");

    const int bgid = 6;
    const unsigned nr_bufs = 4;
    const unsigned buf_len = 32;
    char pool[4][32];

    int setup_err = 0;
    struct io_uring_buf_ring *buf_ring = io_uring_setup_buf_ring(&ring, nr_bufs, bgid, 0, &setup_err);
    TEST_ASSERT(buf_ring != NULL, "io_uring_setup_buf_ring failed");
    unsigned mask = io_uring_buf_ring_mask(nr_bufs);
    for (unsigned i = 0; i < nr_bufs; i++) {
        io_uring_buf_ring_add(buf_ring, pool[i], buf_len, (int)i, mask, (int)i);
    }
    io_uring_buf_ring_advance(buf_ring, (int)nr_bufs);

    int ret = puring_recv_multishot(&ring, 1, sfd, /*len=*/0, bgid, 0, CONNECTED, NO_TO);
    TEST_ASSERT(ret == 1, "puring_recv_multishot submit should succeed");

    const char *msg1 = "first";
    TEST_ASSERT(write(cfd, msg1, strlen(msg1)) == (ssize_t)strlen(msg1), "write #1 failed");

    struct io_uring_cqe *cqe1;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe1) == 0, "wait_cqe #1 failed");
    char errbuf[128];
    snprintf(errbuf, sizeof(errbuf), "expected 'first' length, got res=%d", cqe1->res);
    TEST_ASSERT(cqe1->res == (int)strlen(msg1), errbuf);
    unsigned bid1 = cqe1->flags >> IORING_CQE_BUFFER_SHIFT;
    TEST_ASSERT(memcmp(pool[bid1], msg1, strlen(msg1)) == 0, "buffer content mismatch #1");
    bool more = (cqe1->flags & IORING_CQE_F_MORE) != 0;
    io_uring_cqe_seen(&ring, cqe1);
    TEST_ASSERT(more, "recv_multishot should keep listening (F_MORE set)");

    io_uring_buf_ring_add(buf_ring, pool[bid1], buf_len, (int)bid1, mask, 0);
    io_uring_buf_ring_advance(buf_ring, 1);

    const char *msg2 = "second!";
    TEST_ASSERT(write(cfd, msg2, strlen(msg2)) == (ssize_t)strlen(msg2), "write #2 failed");

    struct io_uring_cqe *cqe2;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe2) == 0, "wait_cqe #2 failed");
    snprintf(errbuf, sizeof(errbuf), "expected 'second!' length, got res=%d", cqe2->res);
    TEST_ASSERT(cqe2->res == (int)strlen(msg2), errbuf);
    unsigned bid2 = cqe2->flags >> IORING_CQE_BUFFER_SHIFT;
    TEST_ASSERT(memcmp(pool[bid2], msg2, strlen(msg2)) == 0, "buffer content mismatch #2");
    io_uring_cqe_seen(&ring, cqe2);

    io_uring_free_buf_ring(&ring, buf_ring, nr_bufs, bgid);
    close(cfd);
    close(sfd);
    ring_destroy(&ring);
}

/* ---------- send_zc ---------- */

static void
test_puring_send_zc__two_completions(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int cfd, sfd;
    TEST_ASSERT(make_tcp_pair(&cfd, &sfd) == 0, "make_tcp_pair failed");

    const char *msg = "zerocopy send";
    int ret = puring_send_zc(&ring, 1, cfd, msg, strlen(msg), 0, CONNECTED, NO_TO);
    TEST_ASSERT(ret == 1, "puring_send_zc submit should succeed");

    struct io_uring_cqe *cqe1;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe1) == 0, "wait_cqe #1 (data) failed");
    char errbuf[128];
    snprintf(errbuf, sizeof(errbuf), "expected send_zc data completion res=len, got %d", cqe1->res);
    TEST_ASSERT(cqe1->res == (int)strlen(msg), errbuf);
    bool has_more = (cqe1->flags & IORING_CQE_F_MORE) != 0;
    io_uring_cqe_seen(&ring, cqe1);
    TEST_ASSERT(has_more, "first send_zc completion should have IORING_CQE_F_MORE (notif follows)");

    struct io_uring_cqe *cqe2;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe2) == 0, "wait_cqe #2 (notif) failed");
    TEST_ASSERT((cqe2->flags & IORING_CQE_F_NOTIF) != 0, "second completion should be the zc notification");
    io_uring_cqe_seen(&ring, cqe2);

    char buf[64] = {0};
    ssize_t n = read(sfd, buf, sizeof(buf));
    TEST_ASSERT(n == (ssize_t)strlen(msg), "peer should receive full zc payload");
    TEST_ASSERT(memcmp(buf, msg, strlen(msg)) == 0, "zc payload content mismatch");

    close(cfd);
    close(sfd);
    ring_destroy(&ring);
}

/* ---------- close_socket ---------- */

static void
test_puring_close_socket__success(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(fd >= 0, "posix socket() failed");

    int ret = puring_close_socket(&ring, 1, fd, NO_TO);
    TEST_ASSERT(ret == 1, "puring_close_socket submit should succeed");
    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 0, "close should succeed");

    TEST_ASSERT(fcntl(fd, F_GETFD) == -1 && errno == EBADF, "fd should be invalid after close");

    ring_destroy(&ring);
}

/* ---------- main additions ---------- */

int
main(void) {
    RUN_TEST(test_prep_socket__success_creates_tcp_socket);
    RUN_TEST(test_prep_socket__unsupported_domain_returns_minus_two);

    RUN_TEST(test_bind_listen_connect_accept__full_flow);
    RUN_TEST(test_puring_connect__refused_on_closed_port);

    RUN_TEST(test_puring_send_recv__roundtrip);
    RUN_TEST(test_puring_recv__peer_closed_returns_zero);

    RUN_TEST(test_puring_sendto_recvfrom__roundtrip);
    RUN_TEST(test_puring_sendmsg_recvmsg__roundtrip_multiple_iovecs);

    // RUN_TEST(test_puring_recv_fixed__success);
    RUN_TEST(test_puring_recv_buffer_select__success);

    RUN_TEST(test_puring_accept_multishot__accepts_two_connections);
    RUN_TEST(test_puring_recv_multishot__receives_two_messages);

    RUN_TEST(test_puring_send_zc__two_completions);

    RUN_TEST(test_puring_close_socket__success);

    fprintf(stderr, "\n%d/%d tests passed\n", g_tests_run - g_tests_failed, g_tests_run);
    return g_tests_failed > 0 ? 1 : 0;
}
