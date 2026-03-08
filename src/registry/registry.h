#pragma once
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <string.h>


#define DEFAULT_REGISTRY_SIZE 128 

typedef struct UringSocket UringSocket;

typedef struct {
    uint64_t user_data;
    PyObject *future;
    PyObject *buffer;
    int opcode;
    UringSocket *socket;
} RequestSlot;


typedef struct {
    RequestSlot *slots;
    int *available_indices;
    int top;
    unsigned int size;
} RequestRegistry;


RequestRegistry* registry_new(unsigned int size);
void registry_destroy(RequestRegistry *reg);

int registry_add(
    RequestRegistry *reg,
    PyObject *future,
    PyObject *buffer,
    int opcode,
    UringSocket *socket
);

RequestSlot* registry_get(RequestRegistry *reg, int index);

void registry_remove(RequestRegistry *reg, int index);
