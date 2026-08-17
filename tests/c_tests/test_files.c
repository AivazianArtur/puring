#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "test_helpers.h"
#include "test_utils.h"

/* ---------- open_file ---------- */

static void
test_open_file__creates_new_file_with_default_flags(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char path[] = "/tmp/aio_uring_ctest_open_XXXXXX";
    int tmp_fd = mkstemp(path);
    TEST_ASSERT(tmp_fd >= 0, "mkstemp failed");
    close(tmp_fd);
    unlink(path);

    int ret = open_file(&ring, 1, AT_FDCWD, path, -1, 0, 0644, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "open_file submit should succeed");

    int fd = wait_one_cqe(&ring);
    TEST_ASSERT(fd >= 0, "opened fd should be non-negative");

    struct stat st;
    TEST_ASSERT(fstat(fd, &st) == 0, "fstat on opened fd failed");

    close(fd);
    unlink(path);
    ring_destroy(&ring);
}

static void
test_open_file__explicit_rdonly_on_existing_file(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "hello aio_uring";
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int ret = open_file(&ring, 1, AT_FDCWD, path, O_RDONLY, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "open_file submit should succeed");

    int fd = wait_one_cqe(&ring);
    TEST_ASSERT(fd >= 0, "opened fd should be non-negative");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_open_file__nonexistent_without_creat_fails(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *path = "/tmp/aio_uring_ctest_does_not_exist_xyz";
    unlink(path);

    int ret = open_file(&ring, 1, AT_FDCWD, path, O_WRONLY, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -ENOENT, "expected -ENOENT for missing file without O_CREAT");

    unlink(path);
    ring_destroy(&ring);
}

static void
test_open_file__sqe_unavailable_returns_minus_one(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    exhaust_sq(&ring);

    int ret = open_file(&ring, 1, AT_FDCWD, "/tmp/whatever", 0, 0, 0644, NO_TIMEOUT);
    TEST_ASSERT(ret == -1, "open_file should fail fast when SQ is exhausted");

    ring_destroy(&ring);
}

static void
test_open_file__O_RDONLY_dont_collides_with_sentinel(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int ret = open_file(&ring, 1, AT_FDCWD, "/tmp/aio_uring_ctest_rdonly_no_collision_xyz", O_RDONLY, 0, 0644, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -ENOENT, "O_RDONLY (0) should no longer be treated as the defaults sentinel");

    ring_destroy(&ring);
}

/* ---------- aio_uring_read ---------- */

static void
test_aio_uring_read__success_reads_full_content(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "0123456789";
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char buf[16] = {0};
    int ret = aio_uring_read(&ring, 1, fd, buf, sizeof(buf), 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_read submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == (int)strlen(content), "should read exactly len(content) bytes");
    TEST_ASSERT(memcmp(buf, content, strlen(content)) == 0, "buffer content mismatch");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_read__respects_offset(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "0123456789";
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char buf[16] = {0};
    int ret = aio_uring_read(&ring, 1, fd, buf, sizeof(buf), 5, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_read submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 5, "should read remaining 5 bytes from offset 5");
    TEST_ASSERT(memcmp(buf, "56789", 5) == 0, "buffer content mismatch for offset read");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_read__zero_size_reads_nothing(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("data", 4);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char buf[4] = {0};
    int ret = aio_uring_read(&ring, 1, fd, buf, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_read submit should succeed even with size 0");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 0, "reading with size 0 should return 0 bytes");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_read__invalid_fd_returns_ebadf(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char buf[16] = {0};
    int ret = aio_uring_read(&ring, 1, 9999 /* not a valid fd */, buf, sizeof(buf), 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EBADF, "expected -EBADF for invalid fd");

    ring_destroy(&ring);
}

static void
test_aio_uring_read__sqe_unavailable_returns_minus_one(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    exhaust_sq(&ring);

    char buf[16];
    int ret = aio_uring_read(&ring, 1, 0, buf, sizeof(buf), 0, NO_TIMEOUT);
    TEST_ASSERT(ret == -1, "aio_uring_read should fail fast when SQ is exhausted");

    ring_destroy(&ring);
}

/* ---------- aio_uring_readv ---------- */

static void
test_aio_uring_readv__success_across_multiple_buffers(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "AAAABBBBCCCC"; /* 12 bytes */
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char buf1[4] = {0};
    char buf2[4] = {0};
    char buf3[4] = {0};
    struct iovec iov[3] = {
        {.iov_base = buf1, .iov_len = 4},
        {.iov_base = buf2, .iov_len = 4},
        {.iov_base = buf3, .iov_len = 4},
    };

    int ret = aio_uring_readv(&ring, 1, fd, iov, 3, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_readv submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 12, "should read exactly 12 bytes total");
    TEST_ASSERT(memcmp(buf1, "AAAA", 4) == 0, "buf1 mismatch");
    TEST_ASSERT(memcmp(buf2, "BBBB", 4) == 0, "buf2 mismatch");
    TEST_ASSERT(memcmp(buf3, "CCCC", 4) == 0, "buf3 mismatch");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_readv__short_read_on_small_file(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "AB"; 
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char buf1[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    char buf2[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    struct iovec iov[2] = {
        {.iov_base = buf1, .iov_len = 4},
        {.iov_base = buf2, .iov_len = 4},
    };

    int ret = aio_uring_readv(&ring, 1, fd, iov, 2, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_readv submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 2, "short read should report only actual bytes read");
    TEST_ASSERT(memcmp(buf1, "AB", 2) == 0, "first 2 bytes should be filled");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_readv__zero_vecs(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("data", 4);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    int ret = aio_uring_readv(&ring, 1, fd, NULL, 0, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_readv submit should succeed even with 0 vecs");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 0, "readv with 0 iovecs should report 0 bytes");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_readv__invalid_fd_returns_ebadf(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char buf[4];
    struct iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};

    int ret = aio_uring_readv(&ring, 1, 9999, &iov, 1, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EBADF, "expected -EBADF for invalid fd");

    ring_destroy(&ring);
}

/* ---------- aio_uring_write ---------- */

static void
test_aio_uring_write__success_writes_content(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file(NULL, 0);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_WRONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char data[] = "written via io_uring";
    int ret = aio_uring_write(&ring, 1, fd, data, (unsigned)strlen(data), 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_write submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == (int)strlen(data), "should write exactly len(data) bytes");
    close(fd);

    int verify_fd = open(path, O_RDONLY);
    char readback[64] = {0};
    ssize_t n = read(verify_fd, readback, sizeof(readback));
    close(verify_fd);
    TEST_ASSERT((size_t)n == strlen(data), "readback length mismatch");
    TEST_ASSERT(memcmp(readback, data, strlen(data)) == 0, "readback content mismatch");

    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_write__respects_offset(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("XXXXXXXXXX", 10);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_WRONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char data[] = "YYY";
    int ret = aio_uring_write(&ring, 1, fd, data, 3, 5, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_write submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 3, "should write exactly 3 bytes");
    close(fd);

    int verify_fd = open(path, O_RDONLY);
    char readback[16] = {0};
    read(verify_fd, readback, 10);
    close(verify_fd);
    TEST_ASSERT(memcmp(readback, "XXXXXYYYXX", 10) == 0, "offset write mismatch");

    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_write__invalid_fd_returns_ebadf(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char data[] = "x";
    int ret = aio_uring_write(&ring, 1, 9999, data, 1, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EBADF, "expected -EBADF for invalid fd");

    ring_destroy(&ring);
}

/* ---------- aio_uring_writev ---------- */

static void
test_aio_uring_writev__success_from_multiple_buffers(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file(NULL, 0);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_WRONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char part1[] = "hello ";
    char part2[] = "world";
    struct iovec iov[2] = {
        {.iov_base = part1, .iov_len = strlen(part1)},
        {.iov_base = part2, .iov_len = strlen(part2)},
    };

    int ret = aio_uring_writev(&ring, 1, fd, iov, 2, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_writev submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 11, "should write exactly 11 bytes total");
    close(fd);

    int verify_fd = open(path, O_RDONLY);
    char readback[16] = {0};
    read(verify_fd, readback, 11);
    close(verify_fd);
    TEST_ASSERT(memcmp(readback, "hello world", 11) == 0, "writev content mismatch");

    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_writev__invalid_fd_returns_ebadf(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char data[] = "x";
    struct iovec iov = {.iov_base = data, .iov_len = 1};

    int ret = aio_uring_writev(&ring, 1, 9999, &iov, 1, 0, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EBADF, "expected -EBADF for invalid fd");

    ring_destroy(&ring);
}

/* ---------- aio_uring_close_file ---------- */

static void
test_aio_uring_close_file__success_closes_fd(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("data", 4);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    int ret = aio_uring_close_file(&ring, 1, fd, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_close_file submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 0, "close should succeed with res == 0");

    TEST_ASSERT(fcntl(fd, F_GETFD) == -1 && errno == EBADF, "fd should be invalid after close");

    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_close_file__invalid_fd_returns_ebadf(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int ret = aio_uring_close_file(&ring, 1, 9999, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EBADF, "expected -EBADF for invalid fd");

    ring_destroy(&ring);
}

/* ---------- aio_uring_fsync / aio_uring_fdatasync ---------- */

static void
test_aio_uring_fsync__success_on_regular_file(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("data", 4);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDWR);
    TEST_ASSERT(fd >= 0, "posix open failed");

    int ret = aio_uring_fsync(&ring, 1, fd, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_fsync submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 0, "fsync on regular file should succeed");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_fdatasync__success_on_regular_file(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("data", 4);
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDWR);
    TEST_ASSERT(fd >= 0, "posix open failed");

    int ret = aio_uring_fdatasync(&ring, 1, fd, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_fdatasync submit should succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == 0, "fdatasync on regular file should succeed");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_fsync__invalid_fd_returns_ebadf(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int ret = aio_uring_fsync(&ring, 1, 9999, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EBADF, "expected -EBADF for invalid fd");

    ring_destroy(&ring);
}

/* ---------- aio_uring_splice ---------- */

static void
test_aio_uring_splice__file_to_pipe_to_file(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "splice me";
    size_t len = strlen(content);
    char *src_path = make_temp_file(content, len);
    char *dst_path = make_temp_file(NULL, 0);
    TEST_ASSERT(src_path && dst_path, "make_temp_file failed");

    int src_fd = open(src_path, O_RDONLY);
    int dst_fd = open(dst_path, O_WRONLY);
    TEST_ASSERT(src_fd >= 0 && dst_fd >= 0, "posix open failed");

    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe() failed");

    /* file -> pipe */
    int ret1 = aio_uring_splice(&ring, 1, src_fd, 0, pipefd[1], -1, (int)len, 0, NO_TIMEOUT);
    TEST_ASSERT(ret1 == 1, "splice file->pipe submit should succeed");
    int res1 = wait_one_cqe(&ring);
    TEST_ASSERT(res1 == (int)len, "splice file->pipe should move all bytes");

    /* pipe -> file */
    int ret2 = aio_uring_splice(&ring, 2, pipefd[0], -1, dst_fd, 0, (int)len, 0, NO_TIMEOUT);
    TEST_ASSERT(ret2 == 1, "splice pipe->file submit should succeed");
    int res2 = wait_one_cqe(&ring);
    TEST_ASSERT(res2 == (int)len, "splice pipe->file should move all bytes");

    close(src_fd);
    close(dst_fd);
    close(pipefd[0]);
    close(pipefd[1]);

    int verify_fd = open(dst_path, O_RDONLY);
    char readback[32] = {0};
    read(verify_fd, readback, len);
    close(verify_fd);
    TEST_ASSERT(memcmp(readback, content, len) == 0, "spliced content mismatch");

    unlink(src_path);
    unlink(dst_path);
    free(src_path);
    free(dst_path);
    ring_destroy(&ring);
}

static void
test_aio_uring_splice__invalid_fd_returns_error(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int pipefd[2];
    TEST_ASSERT(pipe(pipefd) == 0, "pipe() failed");

    int ret = aio_uring_splice(&ring, 1, 9999, 0, pipefd[1], -1, 4, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res < 0, "expected an error for invalid fd_in");

    close(pipefd[0]);
    close(pipefd[1]);
    ring_destroy(&ring);
}

/* ---------- aio_uring_read_buffer_select ---------- */

static void
test_aio_uring_read_buffer_select__success(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "buffer select works";
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    const int bgid = 7;
    char pool[64];
    int prov_res = provide_buffers_group(&ring, pool, sizeof(pool), 1, bgid);
    TEST_ASSERT(prov_res == 0, "provide_buffers should succeed");

    int ret = aio_uring_read_buffer_select(&ring, 1, fd, (unsigned)strlen(content), 0, bgid, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_read_buffer_select submit should succeed");

    struct io_uring_cqe *cqe;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe) == 0, "wait_cqe failed");
    TEST_ASSERT(cqe->res == (int)strlen(content), "should read exactly len(content) bytes");
    TEST_ASSERT((cqe->flags & IORING_CQE_F_BUFFER) != 0, "CQE should carry selected buffer id");
    TEST_ASSERT(memcmp(pool, content, strlen(content)) == 0, "provided buffer content mismatch");
    io_uring_cqe_seen(&ring, cqe);

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_read_buffer_select__invalid_fd_returns_ebadf(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const int bgid = 7;
    char pool[64];
    TEST_ASSERT(provide_buffers_group(&ring, pool, sizeof(pool), 1, bgid) == 0, "provide_buffers should succeed");

    int ret = aio_uring_read_buffer_select(&ring, 1, 9999, sizeof(pool), 0, bgid, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EBADF, "expected -EBADF for invalid fd");

    ring_destroy(&ring);
}

/* ---------- aio_uring_readv_buffer_select ---------- */

static void
test_aio_uring_readv_buffer_select__success(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    const char *content = "readv via buffer select";
    char *path = make_temp_file(content, strlen(content));
    TEST_ASSERT(path != NULL, "make_temp_file failed");

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    const int bgid = 9;
    char pool[64];
    TEST_ASSERT(provide_buffers_group(&ring, pool, sizeof(pool), 1, bgid) == 0, "provide_buffers should succeed");

    int ret = aio_uring_readv_buffer_select(&ring, 1, fd, (unsigned)strlen(content), 0, bgid, 0, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_readv_buffer_select submit should succeed");

    struct io_uring_cqe *cqe;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe) == 0, "wait_cqe failed");
    TEST_ASSERT(cqe->res == (int)strlen(content), "should read exactly len(content) bytes");
    TEST_ASSERT(memcmp(pool, content, strlen(content)) == 0, "provided buffer content mismatch");
    io_uring_cqe_seen(&ring, cqe);

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

/* ---------- aio_uring_read_fixed / aio_uring_write_fixed ---------- */

static void
test_aio_uring_write_fixed__then_read_fixed_roundtrip(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file(NULL, 0);
    TEST_ASSERT(path != NULL, "make_temp_file failed");
    int fd = open(path, O_RDWR);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char write_buf[32] = "fixed buffer roundtrip";
    char read_buf[32] = {0};

    struct iovec regs[2] = {
        {.iov_base = write_buf, .iov_len = sizeof(write_buf)},
        {.iov_base = read_buf, .iov_len = sizeof(read_buf)},
    };
    TEST_ASSERT(io_uring_register_buffers(&ring, regs, 2) == 0, "register_buffers failed");

    unsigned len = (unsigned)strlen(write_buf);

    int wret = aio_uring_write_fixed(&ring, 1, fd, write_buf, len, 0, /*buf_index=*/0, NO_TIMEOUT);
    TEST_ASSERT(wret == 1, "aio_uring_write_fixed submit should succeed");
    int wres = wait_one_cqe(&ring);
    TEST_ASSERT(wres == (int)len, "write_fixed should write exactly len bytes");

    int rret = aio_uring_read_fixed(&ring, 2, fd, read_buf, len, 0, /*buf_index=*/1, NO_TIMEOUT);
    TEST_ASSERT(rret == 1, "aio_uring_read_fixed submit should succeed");
    int rres = wait_one_cqe(&ring);
    TEST_ASSERT(rres == (int)len, "read_fixed should read exactly len bytes");
    TEST_ASSERT(memcmp(read_buf, write_buf, len) == 0, "roundtrip content mismatch");

    io_uring_unregister_buffers(&ring);
    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

static void
test_aio_uring_read_fixed__invalid_buf_index_returns_efault(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("data", 4);
    TEST_ASSERT(path != NULL, "make_temp_file failed");
    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    char buf[16];
    struct iovec regs[1] = {{.iov_base = buf, .iov_len = sizeof(buf)}};
    TEST_ASSERT(io_uring_register_buffers(&ring, regs, 1) == 0, "register_buffers failed");

    int ret = aio_uring_read_fixed(&ring, 1, fd, buf, 4, 0, 5, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res == -EFAULT, "expected -EFAULT for unregistered buf_index");

    io_uring_unregister_buffers(&ring);
    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

/* ---------- aio_uring_read_multishot ---------- */

static void
test_aio_uring_read_multishot__success_on_socket(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    int sv[2];
    TEST_ASSERT(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0, "socketpair() failed");

    const int bgid = 3;
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

    int ret = aio_uring_read_multishot(&ring, 1, sv[0], -1, bgid, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "aio_uring_read_multishot submit should succeed");

    const char *msg = "multishot hello";
    TEST_ASSERT(write(sv[1], msg, strlen(msg)) == (ssize_t)strlen(msg), "socket write failed");

    struct io_uring_cqe *cqe;
    TEST_ASSERT(io_uring_wait_cqe(&ring, &cqe) == 0, "wait_cqe failed");

    char errbuf[128];
    snprintf(errbuf, sizeof(errbuf), "multishot should report bytes written to socket, got %d", cqe->res);
    TEST_ASSERT(cqe->res == (int)strlen(msg), errbuf);

    unsigned bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
    TEST_ASSERT(bid < nr_bufs, "buffer id from CQE out of range");
    fprintf(stderr, "cqe->res=%d cqe->flags=%u\n", cqe->res, cqe->flags);
    TEST_ASSERT(memcmp(pool[bid], msg, strlen(msg)) == 0, "buffer ring content mismatch");

    io_uring_cqe_seen(&ring, cqe);

    io_uring_free_buf_ring(&ring, buf_ring, nr_bufs, bgid);
    close(sv[0]);
    close(sv[1]);
    ring_destroy(&ring);
}

static void
test_aio_uring_read_multishot__regular_file_unsupported(void) {
    struct io_uring ring;
    TEST_ASSERT(ring_init(&ring) == 0, "ring_init failed");

    char *path = make_temp_file("data", 4);
    TEST_ASSERT(path != NULL, "make_temp_file failed");
    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0, "posix open failed");

    const int bgid = 4;
    char pool[64];
    TEST_ASSERT(provide_buffers_group(&ring, pool, sizeof(pool), 2, bgid) == 0, "provide_buffers should succeed");

    int ret = aio_uring_read_multishot(&ring, 1, fd, 0, bgid, NO_TIMEOUT);
    TEST_ASSERT(ret == 1, "submit itself should still succeed");

    int res = wait_one_cqe(&ring);
    TEST_ASSERT(res < 0, "expected an error for multishot read on a non-pollable regular file");

    close(fd);
    unlink(path);
    free(path);
    ring_destroy(&ring);
}

/* ---------- main ---------- */

int
main(void) {
    RUN_TEST(test_open_file__creates_new_file_with_default_flags);
    RUN_TEST(test_open_file__explicit_rdonly_on_existing_file);
    RUN_TEST(test_open_file__nonexistent_without_creat_fails);
    RUN_TEST(test_open_file__sqe_unavailable_returns_minus_one);

    RUN_TEST(test_aio_uring_read__success_reads_full_content);
    RUN_TEST(test_aio_uring_read__respects_offset);
    RUN_TEST(test_aio_uring_read__zero_size_reads_nothing);
    RUN_TEST(test_aio_uring_read__invalid_fd_returns_ebadf);
    RUN_TEST(test_aio_uring_read__sqe_unavailable_returns_minus_one);

    RUN_TEST(test_aio_uring_readv__success_across_multiple_buffers);
    RUN_TEST(test_aio_uring_readv__short_read_on_small_file);
    RUN_TEST(test_aio_uring_readv__zero_vecs);
    RUN_TEST(test_aio_uring_readv__invalid_fd_returns_ebadf);

    RUN_TEST(test_aio_uring_write__success_writes_content);
    RUN_TEST(test_aio_uring_write__respects_offset);
    RUN_TEST(test_aio_uring_write__invalid_fd_returns_ebadf);

    RUN_TEST(test_aio_uring_writev__success_from_multiple_buffers);
    RUN_TEST(test_aio_uring_writev__invalid_fd_returns_ebadf);

    RUN_TEST(test_aio_uring_close_file__success_closes_fd);
    RUN_TEST(test_aio_uring_close_file__invalid_fd_returns_ebadf);

    RUN_TEST(test_aio_uring_fsync__success_on_regular_file);
    RUN_TEST(test_aio_uring_fdatasync__success_on_regular_file);
    RUN_TEST(test_aio_uring_fsync__invalid_fd_returns_ebadf);

    RUN_TEST(test_aio_uring_splice__file_to_pipe_to_file);
    RUN_TEST(test_aio_uring_splice__invalid_fd_returns_error);

    RUN_TEST(test_aio_uring_read_buffer_select__success);
    RUN_TEST(test_aio_uring_read_buffer_select__invalid_fd_returns_ebadf);
    RUN_TEST(test_aio_uring_readv_buffer_select__success);

    RUN_TEST(test_aio_uring_write_fixed__then_read_fixed_roundtrip);
    RUN_TEST(test_aio_uring_read_fixed__invalid_buf_index_returns_efault);

    RUN_TEST(test_aio_uring_read_multishot__success_on_socket);
    RUN_TEST(test_aio_uring_read_multishot__regular_file_unsupported);
    fprintf(stderr, "\n%d/%d tests passed\n", g_tests_run - g_tests_failed, g_tests_run);
    return g_tests_failed > 0 ? 1 : 0;
}