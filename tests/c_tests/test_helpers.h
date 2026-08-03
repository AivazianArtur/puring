#pragma once
#include <stdio.h>

static int g_tests_run = 0;
static int g_tests_failed = 0;
static int g_current_test_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "  FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg); \
            g_current_test_failed = 1; \
            return; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        g_tests_run++; \
        g_current_test_failed = 0; \
        fprintf(stderr, "RUN  %s\n", #fn); \
        fn(); \
        if (g_current_test_failed) { \
            g_tests_failed++; \
            fprintf(stderr, "FAIL %s\n", #fn); \
        } else { \
            fprintf(stderr, "OK   %s\n", #fn); \
        } \
    } while (0)
