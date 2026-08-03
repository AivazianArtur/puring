#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "test_helpers.h"
#include "test_utils.h"
#include "buffer_controllers/buffer_controllers.h"

/* ---------- buffer_idx_registry ---------- */

static void
test_buffer_idx_registry__create_with_zero_uses_default_size(void) {
    FixedBufferIdxRegistry *reg = buffer_idx_registry_create(0);
    TEST_ASSERT(reg != NULL, "registry_create(0) should succeed");
    TEST_ASSERT(reg->size == DEFAULT_BUFFER_IDX_REGISTRY_SIZE, "size should fall back to default");
    TEST_ASSERT(reg->top == (int)DEFAULT_BUFFER_IDX_REGISTRY_SIZE - 1, "top should be size-1");

    buffer_idx_registry_destroy(reg);
    free(reg);
}

static void
test_buffer_idx_registry__create_with_explicit_size(void) {
    FixedBufferIdxRegistry *reg = buffer_idx_registry_create(4);
    TEST_ASSERT(reg != NULL, "registry_create(4) should succeed");
    TEST_ASSERT(reg->size == 4, "size should match explicit value");
    TEST_ASSERT(reg->top == 3, "top should be size-1");

    buffer_idx_registry_destroy(reg);
    free(reg);
}

static void
test_get_buffer_idx__returns_indices_in_lifo_order(void) {
    FixedBufferIdxRegistry *reg = buffer_idx_registry_create(3);
    TEST_ASSERT(reg != NULL, "registry_create failed");

    int idx1 = get_buffer_idx(reg);
    TEST_ASSERT(idx1 == 2, "first get_buffer_idx should return last index (LIFO)");

    int idx2 = get_buffer_idx(reg);
    TEST_ASSERT(idx2 == 1, "second get_buffer_idx should return next index");

    int idx3 = get_buffer_idx(reg);
    TEST_ASSERT(idx3 == 0, "third get_buffer_idx should return remaining index");

    buffer_idx_registry_destroy(reg);
    free(reg);
}

static void
test_get_buffer_idx__exhausted_registry_returns_minus_one(void) {
    FixedBufferIdxRegistry *reg = buffer_idx_registry_create(1);
    TEST_ASSERT(reg != NULL, "registry_create failed");

    int idx = get_buffer_idx(reg);
    TEST_ASSERT(idx == 0, "single-slot registry should yield index 0");

    int exhausted = get_buffer_idx(reg);
    TEST_ASSERT(exhausted == -1, "get_buffer_idx on exhausted registry should return -1");

    buffer_idx_registry_destroy(reg);
    free(reg);
}

static void
test_release_buffer_idx__roundtrip(void) {
    FixedBufferIdxRegistry *reg = buffer_idx_registry_create(2);
    TEST_ASSERT(reg != NULL, "registry_create failed");

    int idx = get_buffer_idx(reg);
    TEST_ASSERT(idx == 1, "expected index 1 first");

    int rel = release_buffer_idx(reg, idx);
    TEST_ASSERT(rel == 0, "release_buffer_idx should succeed");

    int idx_again = get_buffer_idx(reg);
    TEST_ASSERT(idx_again == 1, "released index should be reusable");

    buffer_idx_registry_destroy(reg);
    free(reg);
}

static void
test_release_buffer_idx__overflow_returns_minus_one(void) {
    FixedBufferIdxRegistry *reg = buffer_idx_registry_create(2);
    TEST_ASSERT(reg != NULL, "registry_create failed");

    int rel = release_buffer_idx(reg, 99);
    TEST_ASSERT(rel == -1, "releasing into a full registry should return -1");

    buffer_idx_registry_destroy(reg);
    free(reg);
}

static void
test_buffer_idx_registry_destroy__resets_fields(void) {
    FixedBufferIdxRegistry *reg = buffer_idx_registry_create(4);
    TEST_ASSERT(reg != NULL, "registry_create failed");

    buffer_idx_registry_destroy(reg);
    TEST_ASSERT(reg->available_indices == NULL, "available_indices should be NULL after destroy");
    TEST_ASSERT(reg->size == 0, "size should be reset to 0 after destroy");
    TEST_ASSERT(reg->top == -1, "top should be reset to -1 after destroy");

    free(reg);
}

/* ---------- init_fixed_mode / close_fixed_mode ---------- */

static void
test_init_fixed_mode__success_and_close(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char buf1[64];
    char buf2[64];
    struct iovec iov[2] = {
        {.iov_base = buf1, .iov_len = sizeof(buf1)},
        {.iov_base = buf2, .iov_len = sizeof(buf2)},
    };

    int init_res = init_fixed_mode(&ring, iov, 2);
    TEST_ASSERT(init_res == 1, "init_fixed_mode should succeed");

    int close_res = close_fixed_mode(&ring);
    TEST_ASSERT(close_res == 1, "close_fixed_mode should succeed");

    ring_destroy(&ring);
}

static void
test_init_fixed_mode__zero_iovecs_fails(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int res = init_fixed_mode(&ring, NULL, 0);
    TEST_ASSERT(res == -1, "init_fixed_mode with 0 iovecs should fail");

    ring_destroy(&ring);
}

static void
test_close_fixed_mode__without_prior_register_fails(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int res = close_fixed_mode(&ring);
    TEST_ASSERT(res == -1, "unregistering buffers without prior registration should fail");

    ring_destroy(&ring);
}

/* ---------- init_provided_mode / close_provided_mode ---------- */

static void
test_init_provided_mode__success_and_close(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const int bgid = 11;
    const int nr = 4;
    const int buf_len = 32;
    char pool[4 * 32];

    int init_res = init_provided_mode(&ring, pool, buf_len, nr, bgid);
    TEST_ASSERT(init_res == 1, "init_provided_mode submit should succeed");

    int prov_cqe_res = wait_one_cqe(&ring);

    printf("prov_cqe_res res = %d\n", prov_cqe_res);
    TEST_ASSERT(prov_cqe_res == 0, "provide_buffers completion should report success");

    int close_res = close_provided_mode(&ring, nr, bgid);
    TEST_ASSERT(close_res == 1, "close_provided_mode submit should succeed");

    int close_cqe_res = wait_one_cqe(&ring);
    printf("close_cqe_res = %d\n", close_cqe_res);
    TEST_ASSERT(close_cqe_res == nr, "remove_buffers should report count of removed buffers");

    ring_destroy(&ring);
}

static void
test_init_provided_mode__then_actually_used_by_read_buffer_select(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "provided mode integration";
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");
    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    const int bgid = 12;
    char pool[64];
    int init_res = init_provided_mode(&ring, pool, sizeof(pool), 1, bgid);
    TEST_ASSERT(init_res == 1, "init_provided_mode submit should succeed");
    TEST_ASSERT(wait_one_cqe(&ring) == 0, "provide_buffers completion should succeed");

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    TEST_ASSERT(sqe != NULL, "get_sqe failed");
    io_uring_prep_read(sqe, fd, NULL, sizeof(pool), 0);
    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = (__u16)bgid;
    io_uring_sqe_set_data(sqe, NULL);
    TEST_ASSERT(io_uring_submit(&ring) >= 0, "read via provided buffer submit failed");

    struct io_uring_cqe *cqe;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe) == 0, "wait_cqe failed");
    TEST_ASSERT(cqe->res == (int)strlen(content), "read should report exact length");
    TEST_ASSERT(memcmp(pool, content, strlen(content)) == 0, "provided buffer content mismatch");
    io_uring_cqe_seen(&ring, cqe);

    close_provided_mode(&ring, 1, bgid);
    wait_one_cqe(&ring);

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

/* ---------- init_buf_ring_mode / close_buf_ring_mode ---------- */

static void
test_init_buf_ring_mode__success_and_close(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const int bgid = 13;
    const int nentries = 4;
    const int buf_len = 32;
    char pool[4 * 32];

    struct io_uring_buf_ring *buf_ring = init_buf_ring_mode(&ring, pool, buf_len, nentries, bgid);
    TEST_ASSERT(buf_ring != NULL, "init_buf_ring_mode should succeed");

    int close_res = close_buf_ring_mode(&ring, buf_ring, nentries, bgid);
    TEST_ASSERT(close_res == 1, "close_buf_ring_mode should succeed");

    ring_destroy(&ring);
}

static void
test_close_buf_ring_mode__null_ring_returns_minus_one(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int res = close_buf_ring_mode(&ring, NULL, 4, 14);
    TEST_ASSERT(res == -1, "close_buf_ring_mode with NULL buffer_ring should return -1");

    ring_destroy(&ring);
}

static void
test_init_buf_ring_mode__then_actually_used_by_read_multishot(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int sv[2];
    TEST_ASSERT(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair() failed");

    const int bgid = 15;
    const int nentries = 4;
    const int buf_len = 32;
    char pool[4 * 32];

    struct io_uring_buf_ring *buf_ring = init_buf_ring_mode(&ring, pool, buf_len, nentries, bgid);
    TEST_ASSERT(buf_ring != NULL, "init_buf_ring_mode should succeed");

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    TEST_ASSERT(sqe != NULL, "get_sqe failed");
    io_uring_prep_read_multishot(sqe, sv[0], 0, (__u64)-1, bgid);
    io_uring_sqe_set_data(sqe, NULL);
    TEST_ASSERT(io_uring_submit(&ring) >= 0, "read_multishot submit failed");

    const char *msg = "buf_ring_mode works";
    TEST_ASSERT(write(sv[1], msg, strlen(msg)) == (ssize_t)strlen(msg), "socket write failed");

    struct io_uring_cqe *cqe;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe) == 0, "wait_cqe failed");
    TEST_ASSERT(cqe->res == (int)strlen(msg), "multishot read should report exact length");
    unsigned bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
    TEST_ASSERT(memcmp(pool + (bid * buf_len), msg, strlen(msg)) == 0, "buf_ring content mismatch");
    io_uring_cqe_seen(&ring, cqe);

    close_buf_ring_mode(&ring, buf_ring, nentries, bgid);
    close(sv[0]);
    close(sv[1]);
    ring_destroy(&ring);
}

/* ---------- main ---------- */

int
main(void) {
    RUN_TEST(test_buffer_idx_registry__create_with_zero_uses_default_size);
    RUN_TEST(test_buffer_idx_registry__create_with_explicit_size);
    RUN_TEST(test_get_buffer_idx__returns_indices_in_lifo_order);
    RUN_TEST(test_get_buffer_idx__exhausted_registry_returns_minus_one);
    RUN_TEST(test_release_buffer_idx__roundtrip);
    RUN_TEST(test_release_buffer_idx__overflow_returns_minus_one);
    RUN_TEST(test_buffer_idx_registry_destroy__resets_fields);

    RUN_TEST(test_init_fixed_mode__success_and_close);
    RUN_TEST(test_init_fixed_mode__zero_iovecs_fails);
    RUN_TEST(test_close_fixed_mode__without_prior_register_fails);

    RUN_TEST(test_init_provided_mode__success_and_close);
    RUN_TEST(test_init_provided_mode__then_actually_used_by_read_buffer_select);

    RUN_TEST(test_init_buf_ring_mode__success_and_close);
    RUN_TEST(test_close_buf_ring_mode__null_ring_returns_minus_one);
    RUN_TEST(test_init_buf_ring_mode__then_actually_used_by_read_multishot);

    fprintf(stderr, "\n%d/%d tests passed\n", g_tests_run - g_tests_failed, g_tests_run);
    return g_tests_failed > 0 ? 1 : 0;
}
