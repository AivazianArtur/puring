# include "construction/handler.c"

// Ring Wrapper (for request(TEMP: maybe for everything))
typedef struct {
    // PyObject_HEAD
    struct io_uring *ring;  // Reference to the main ring
    // PyObject *future;       // The Python asyncio.Future to wake up
    int fd;                 // File descriptor
    void *buffer;           // The buffer where Kernel puts data
} UringRequestObject;
