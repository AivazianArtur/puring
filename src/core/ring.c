#include "core.h"

io_uring* ring_new(void) {
    struct io_uring ring;
    struct io_uring_params p;

    if (io_uring_queue_init(8, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        return 1;
    }
    return ring;
}


int ring_init(void) {

    memory_params memory_params = NULL;
    ring_initialization_params *params = NULL,

    static char *kwlist[] = {"memory_params", "ring_initialization_parans", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Ii", kwlist, &memory_params, &ring_initialization_params))
        return -1;

    // TODO: 1 - Serialize args
    // TODO: 2 - Initialize the Shadow Registry, if err - PyErr_NoMemory();
    // TODO: 3 - Route initialization

    if (memory_params) {
        // TODO
        if (!params) {
            params = {0}
        }
        result = io_uring_queue_init_mem(),  // TODO: Good name, not `resutlt`
    }
    else if (params) {
        // TODO
        result = io_uring_queue_init_params(8, ring, params);
    }
    else {
        result = io_uring_queue_init(0, ring, 0);
    }
    
    if (result < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    self->initialized = true;
    return result; // TEMP: OR return 0?
}

void ring_destroy(io_uring* ring) {
    io_uring_queue_exit(ring);
}
