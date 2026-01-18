#define PY_SSIZE_T_CLEAN
#include <Python.h>

typedef struct {
    PyObject **objects;
    unsigned int size;  // Should match ring entries (e.g., 256)
} RequestRegistry;


/* Functions */
int registry_init(RequestRegistry *reg, unsigned int size);
void registry_free(RequestRegistry *reg);
int registry_add(RequestRegistry *reg, PyObject *obj);
PyObject* registry_get(RequestRegistry *reg, int index);
void registry_remove(RequestRegistry *reg, int index);
