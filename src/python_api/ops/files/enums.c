#include "files.h"
// TODO: Optimize - im afrtaid if those runs for every python: import ResolveFlags/StatxFlags/StatxMask


// open()
PyObject*
create_resolve_enum(void)
{
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module) return NULL;

    PyObject *IntFlag = PyObject_GetAttrString(enum_module, "IntFlag");
    Py_DECREF(enum_module);
    if (!IntFlag) return NULL;

    PyObject *members = Py_BuildValue(
        "{s:i, s:i, s:i, s:i, s:i, s:i}",
        "NO_XDEV",       RESOLVE_NO_XDEV,
        "NO_MAGICLINKS", RESOLVE_NO_MAGICLINKS,
        "NO_SYMLINKS",   RESOLVE_NO_SYMLINKS,
        "BENEATH",       RESOLVE_BENEATH,
        "IN_ROOT",       RESOLVE_IN_ROOT,
        "CACHED",        RESOLVE_CACHED
    );
    if (!members) { 
        Py_DECREF(IntFlag);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(IntFlag, "sO", "ResolveFlags", members);
    Py_DECREF(IntFlag);
    Py_DECREF(members);
    return result;
}


// statx() flags (AT_*)
PyObject*
create_statx_flags_enum(void)
{
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module) return NULL;

    PyObject *IntFlag = PyObject_GetAttrString(enum_module, "IntFlag");
    Py_DECREF(enum_module);
    if (!IntFlag) return NULL;

    PyObject *members = Py_BuildValue(
        "{s:i, s:i, s:i, s:i, s:i}",
        "EMPTY_PATH",        AT_EMPTY_PATH,
        "SYMLINK_NOFOLLOW",  AT_SYMLINK_NOFOLLOW,
        "NO_AUTOMOUNT",      AT_NO_AUTOMOUNT,
        "STATX_SYNC_AS_STAT", AT_STATX_SYNC_AS_STAT,
        "STATX_FORCE_SYNC",  AT_STATX_FORCE_SYNC,
        "STATX_DONT_SYNC",   AT_STATX_DONT_SYNC
    );
    if (!members) {
        Py_DECREF(IntFlag);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(IntFlag, "sO", "StatxFlags", members);
    Py_DECREF(IntFlag);
    Py_DECREF(members);
    return result;
}


// statx() mask (STATX_*)
PyObject*
create_statx_mask_enum(void)
{
    PyObject *enum_module = PyImport_ImportModule("enum");
    if (!enum_module) return NULL;

    PyObject *IntFlag = PyObject_GetAttrString(enum_module, "IntFlag");
    Py_DECREF(enum_module);
    if (!IntFlag) return NULL;

    PyObject *members = Py_BuildValue(
        "{s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i}",
        "TYPE",        STATX_TYPE,
        "MODE",        STATX_MODE,
        "NLINK",       STATX_NLINK,
        "UID",         STATX_UID,
        "GID",         STATX_GID,
        "ATIME",       STATX_ATIME,
        "MTIME",       STATX_MTIME,
        "CTIME",       STATX_CTIME,
        "INO",         STATX_INO,
        "SIZE",        STATX_SIZE,
        "BLOCKS",      STATX_BLOCKS,
        "BASIC_STATS", STATX_BASIC_STATS,
        "BTIME",       STATX_BTIME,
        "ALL",         STATX_ALL
    );
    if (!members) {
        Py_DECREF(IntFlag);
        return NULL;
    }

    PyObject *result = PyObject_CallFunction(IntFlag, "sO", "StatxMask", members);
    Py_DECREF(IntFlag);
    Py_DECREF(members);
    return result;
}
