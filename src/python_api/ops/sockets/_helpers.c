#include "sockets.h"


PyObject* _check_sockets_result(
    int result, UringSocket *socket, int request_idx, PyObject *future 
) {
    if (result < 1) {
        if (result == -1) {
            PyErr_SetString(PyExc_RuntimeError, "SQE is not awailable\n");
        } else if (result == -2) {
            PyErr_SetString(PyExc_RuntimeError, "Wrong socket status.\n");
        } else if (result == 0) {
            PyErr_SetString(PyExc_RuntimeError, "SQE submission failed\n");
        }
        Py_DECREF(future);
        registry_remove(socket->loop->registry, request_idx);
        return NULL;
    }
    return future;
}


struct sockaddr* _serialize_address(const char *host, int port, int domain) {
    struct sockaddr *addr;
    if (domain == AF_INET) {
        struct sockaddr_in *temp_addr = malloc(sizeof(*temp_addr));
        if (!temp_addr) {
            PyErr_NoMemory();
            return NULL;
        }
    
        temp_addr->sin_family=AF_INET;
        temp_addr->sin_port = htons(port);
        if (inet_pton(AF_INET, host, &temp_addr->sin_addr) != 1) {
            free(temp_addr);
            PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IP address");
            return NULL;
        }
        addr = (struct sockaddr*)temp_addr;
        free(temp_addr);
    } else if (domain == AF_INET6) {
        struct sockaddr_in6 *temp_addr = malloc(sizeof(*temp_addr));
        if (!addr) {
            PyErr_NoMemory();
            return NULL;
        }
    
        temp_addr->sin6_family=AF_INET;
        temp_addr->sin6_port = htons(port);
        if (inet_pton(AF_INET, host, &temp_addr->sin6_addr) != 1) {
            free(temp_addr);
            PyErr_SetString(PyExc_ConnectionRefusedError, "Invalid IP address");
            return NULL;
        }
        addr = (struct sockaddr*)temp_addr;
        free(temp_addr);
    } else {
        return NULL;
    }
    memset(addr, 0, sizeof(*addr));
    return addr;
}


socklen_t _get_socket_size(int domain) {
    switch (domain) {
        case AF_INET6:
            return sizeof(struct sockaddr_in6);
        default:
            return sizeof(struct sockaddr_in);
    }
}
