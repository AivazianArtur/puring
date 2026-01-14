#pragma once  // Alternative of `include guards (IFNDEF)`
#include "liburing.h"


/* ShadowRegistry to shadow PyObjects from GC*/
typedef struct {
    PyObject **objects;
    unsigned int size;  // Should match ring entries (e.g., 256)
} RequestRegistry;


/* Ring Wrapper*/
typedef struct {
    PyObject_HEAD
    struct io_uring ring;
    RequestRegistry registry; 
    bool initialized;
    int fd;                 // File descriptor
    void *buffer;           // The buffer where Kernel puts data
} UringObject;
