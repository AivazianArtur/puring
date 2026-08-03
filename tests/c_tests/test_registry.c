#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "test_helpers.h"
#include "registry/registry.h"

/*
 * PuringFile/PuringSocket both start with PyObject_HEAD, and registry.c only
 * ever touches them through Py_INCREF/Py_DECREF (it never dereferences
 * puring-specific fields). Any real PyObject can stand in for them here.
 */
static PyObject *
make_future(void) {
    return PyLong_FromLong(0);
}

/* ---------- registry_new ---------- */

static void
test_registry_new__default_size(void) {
    RequestRegistry *reg = registry_new(0);
    TEST_ASSERT(reg != NULL, "registry_new(0) should succeed");
    TEST_ASSERT(reg->size == DEFAULT_REGISTRY_SIZE, "size should fall back to DEFAULT_REGISTRY_SIZE");
    TEST_ASSERT(reg->top == (int)reg->size - 1, "top should start at size-1 (all slots free)");

    for (unsigned int i = 0; i < reg->size; i++) {
        TEST_ASSERT(reg->available_indices[i] == (int)i, "available_indices should be identity-initialized");
    }

    registry_destroy(reg);
}

static void
test_registry_new__custom_size(void) {
    RequestRegistry *reg = registry_new(8);
    TEST_ASSERT(reg != NULL, "registry_new(8) should succeed");
    TEST_ASSERT(reg->size == 8, "size should match requested value");
    TEST_ASSERT(reg->top == 7, "top should be size-1");

    registry_destroy(reg);
}

/* ---------- registry_add ---------- */

static void
test_registry_add__returns_lifo_indices(void) {
    RequestRegistry *reg = registry_new(4);
    TEST_ASSERT(reg != NULL, "registry_new failed");

    PyObject *f1 = make_future();
    PyObject *f2 = make_future();

    int idx1 = registry_add(reg, f1, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);
    int idx2 = registry_add(reg, f2, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);

    TEST_ASSERT(idx1 == 3, "first add should hand out the top index (size-1)");
    TEST_ASSERT(idx2 == 2, "second add should hand out the next index down");
    TEST_ASSERT(reg->top == 1, "top should have decremented twice");

    registry_remove(reg, idx1);
    registry_remove(reg, idx2);
    Py_DECREF(f1);
    Py_DECREF(f2);
    registry_destroy(reg);
}

static void
test_registry_add__increfs_future(void) {
    RequestRegistry *reg = registry_new(2);
    PyObject *future = make_future();
    Py_ssize_t before = Py_REFCNT(future);

    int idx = registry_add(reg, future, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);
    TEST_ASSERT(idx >= 0, "registry_add should succeed");
    TEST_ASSERT(Py_REFCNT(future) == before + 1, "registry_add should take its own reference on future");

    registry_remove(reg, idx);
    TEST_ASSERT(Py_REFCNT(future) == before, "registry_remove should release the reference");

    Py_DECREF(future);
    registry_destroy(reg);
}

static void
test_registry_add__increfs_file_and_socket_when_present(void) {
    RequestRegistry *reg = registry_new(2);
    PyObject *future = make_future();
    PyObject *fake_file = PyLong_FromLong(1);
    PyObject *fake_socket = PyLong_FromLong(2);

    Py_ssize_t file_before = Py_REFCNT(fake_file);
    Py_ssize_t socket_before = Py_REFCNT(fake_socket);

    int idx = registry_add(
        reg, future, NULL, ONESHOT, 1, (PuringFile *)fake_file, (PuringSocket *)fake_socket, NULL, NULL
    );
    TEST_ASSERT(idx >= 0, "registry_add should succeed");
    TEST_ASSERT(Py_REFCNT(fake_file) == file_before + 1, "file should be increfed");
    TEST_ASSERT(Py_REFCNT(fake_socket) == socket_before + 1, "socket should be increfed");

    registry_remove(reg, idx);
    TEST_ASSERT(Py_REFCNT(fake_file) == file_before, "file reference should be released on remove");
    TEST_ASSERT(Py_REFCNT(fake_socket) == socket_before, "socket reference should be released on remove");

    Py_DECREF(future);
    Py_DECREF(fake_file);
    Py_DECREF(fake_socket);
    registry_destroy(reg);
}

static void
test_registry_add__slot_fields_set_correctly(void) {
    RequestRegistry *reg = registry_new(2);
    PyObject *future = make_future();

    struct msghdr *msghdr = calloc(1, sizeof(struct msghdr));
    TEST_ASSERT(msghdr != NULL, "calloc for msghdr failed");

    int idx = registry_add(reg, future, NULL, MULTISHOT, 42, NULL, NULL, NULL, msghdr);
    TEST_ASSERT(idx >= 0, "registry_add should succeed");

    RequestSlot *slot = registry_get(reg, idx);
    TEST_ASSERT(slot != NULL, "registry_get should return the slot we just added");
    TEST_ASSERT(slot->user_data == (uint64_t)idx, "user_data should equal the slot index");
    TEST_ASSERT(slot->opcode == 42, "opcode should be stored as-is");
    TEST_ASSERT(slot->stream_strategy == MULTISHOT, "stream_strategy should be stored as-is");
    TEST_ASSERT(slot->future == future, "slot->future should point at the passed future");
    TEST_ASSERT(slot->msghdr == msghdr, "slot->msghdr should point at the passed msghdr");

    registry_remove(reg, idx); /* frees msghdr internally */
    Py_DECREF(future);
    registry_destroy(reg);
}

static void
test_registry_add__full_registry_returns_minus_one(void) {
    RequestRegistry *reg = registry_new(1);
    PyObject *f1 = make_future();
    PyObject *f2 = make_future();

    int idx1 = registry_add(reg, f1, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);
    TEST_ASSERT(idx1 == 0, "single-slot registry should hand out index 0");

    int idx2 = registry_add(reg, f2, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);
    TEST_ASSERT(idx2 == -1, "registry_add on a full registry should return -1");
    TEST_ASSERT(Py_REFCNT(f2) == 1, "a rejected add must not have touched f2's refcount");

    registry_remove(reg, idx1);
    Py_DECREF(f1);
    Py_DECREF(f2);
    registry_destroy(reg);
}

/* ---------- registry_get ---------- */

static void
test_registry_get__out_of_bounds_returns_null(void) {
    RequestRegistry *reg = registry_new(4);

    TEST_ASSERT(registry_get(reg, -1) == NULL, "negative index should return NULL");
    TEST_ASSERT(registry_get(reg, 4) == NULL, "index == size should return NULL");
    TEST_ASSERT(registry_get(reg, 1000) == NULL, "far out-of-bounds index should return NULL");
    TEST_ASSERT(registry_get(reg, 0) != NULL, "index 0 should be a valid slot pointer even when unused");

    registry_destroy(reg);
}

/* ---------- registry_remove ---------- */

static void
test_registry_remove__out_of_bounds_index_is_noop(void) {
    RequestRegistry *reg = registry_new(4);
    int top_before = reg->top;

    registry_remove(reg, -1);
    registry_remove(reg, 4);
    registry_remove(reg, 1000);

    TEST_ASSERT(reg->top == top_before, "out-of-bounds remove must not change top");

    registry_destroy(reg);
}

static void
test_registry_remove__frees_msghdr(void) {
    RequestRegistry *reg = registry_new(2);
    PyObject *future = make_future();
    struct msghdr *msghdr = calloc(1, sizeof(struct msghdr));

    int idx = registry_add(reg, future, NULL, ONESHOT, 1, NULL, NULL, NULL, msghdr);
    registry_remove(reg, idx);

    RequestSlot *slot = registry_get(reg, idx);
    TEST_ASSERT(slot->msghdr == NULL, "msghdr pointer should be cleared after remove");
    /* freed pointer itself can't be safely dereferenced again; absence of a
       crash/ASan report on this test run is the actual assertion here. */

    Py_DECREF(future);
    registry_destroy(reg);
}

static void
test_registry_remove__index_becomes_reusable(void) {
    RequestRegistry *reg = registry_new(2);
    PyObject *f1 = make_future();

    int idx1 = registry_add(reg, f1, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);
    registry_remove(reg, idx1);
    Py_DECREF(f1);

    PyObject *f2 = make_future();
    int idx2 = registry_add(reg, f2, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);
    TEST_ASSERT(idx2 == idx1, "a freed index should be handed out again");

    registry_remove(reg, idx2);
    Py_DECREF(f2);
    registry_destroy(reg);
}

static void
test_registry_remove__double_remove_aborts(void) {
    pid_t pid = fork();
    TEST_ASSERT(pid >= 0, "fork failed");

    if (pid == 0) {
        RequestRegistry *reg = registry_new(1);
        PyObject *f = make_future();
        int idx = registry_add(reg, f, NULL, ONESHOT, 1, NULL, NULL, NULL, NULL);
        registry_remove(reg, idx);
        registry_remove(reg, idx); /* top already at size-1 -> should abort() */
        _exit(0);                 /* should never reach here */
    }

    int status = 0;
    TEST_ASSERT(waitpid(pid, &status, 0) == pid, "waitpid failed");
    TEST_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
                "double registry_remove should abort() the process");
}

/* ---------- registry_destroy ---------- */

static void
test_registry_destroy__releases_pending_slots(void) {
    RequestRegistry *reg = registry_new(4);
    PyObject *future = make_future();
    PyObject *fake_file = PyLong_FromLong(1);
    PyObject *fake_socket = PyLong_FromLong(2);

    Py_ssize_t future_before = Py_REFCNT(future);
    Py_ssize_t file_before = Py_REFCNT(fake_file);
    Py_ssize_t socket_before = Py_REFCNT(fake_socket);

    int idx = registry_add(
        reg, future, NULL, ONESHOT, 1, (PuringFile *)fake_file, (PuringSocket *)fake_socket, NULL, NULL
    );
    TEST_ASSERT(idx >= 0, "registry_add should succeed");

    registry_destroy(reg);

    TEST_ASSERT(Py_REFCNT(future) == future_before, "registry_destroy should release future refs it still holds");
    TEST_ASSERT(Py_REFCNT(fake_file) == file_before, "registry_destroy should release file refs it still holds");
    TEST_ASSERT(Py_REFCNT(fake_socket) == socket_before, "registry_destroy should release socket refs it still holds");

    Py_DECREF(future);
    Py_DECREF(fake_file);
    Py_DECREF(fake_socket);
}

static void
test_registry_destroy__null_is_safe(void) {
    registry_destroy(NULL); /* must not crash */
    TEST_ASSERT(1, "registry_destroy(NULL) should be a no-op");
}

/* ---------- main ---------- */

int
main(void) {
    Py_Initialize();

    RUN_TEST(test_registry_new__default_size);
    RUN_TEST(test_registry_new__custom_size);

    RUN_TEST(test_registry_add__returns_lifo_indices);
    RUN_TEST(test_registry_add__increfs_future);
    RUN_TEST(test_registry_add__increfs_file_and_socket_when_present);
    RUN_TEST(test_registry_add__slot_fields_set_correctly);
    RUN_TEST(test_registry_add__full_registry_returns_minus_one);

    RUN_TEST(test_registry_get__out_of_bounds_returns_null);

    RUN_TEST(test_registry_remove__out_of_bounds_index_is_noop);
    RUN_TEST(test_registry_remove__frees_msghdr);
    RUN_TEST(test_registry_remove__index_becomes_reusable);
    RUN_TEST(test_registry_remove__double_remove_aborts);

    RUN_TEST(test_registry_destroy__releases_pending_slots);
    RUN_TEST(test_registry_destroy__null_is_safe);

    fprintf(stderr, "\n%d/%d tests passed\n", g_tests_run - g_tests_failed, g_tests_run);

    int failed = g_tests_failed;
    Py_Finalize();
    return failed > 0 ? 1 : 0;
}
