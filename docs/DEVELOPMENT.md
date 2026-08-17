# Development Guide

This guide outlines how to set up your environment, run tests, and submit Pull Requests for **aio_uring**.

## Quick Start

### 1. Clone the repository (with submodules)

```bash
git clone --recurse-submodules <repo-url>
cd aio_uring
```

If you already cloned without `--recurse-submodules`, the build will initialize it
automatically (see `check-submodule`), or you can do it manually:

```bash
git submodule update --init --recursive
```

### 2. Install system dependencies

The Makefile auto-detects your package manager (`apt` or `dnf`) and can install
everything needed:

```bash
make dev-deps
```

This installs:

- Python dev headers (`python3-dev` / `python3-devel`)
- A Python virtualenv (`.venv`)
- `liburing` (built from the submodule)
- Native toolchain: `clang`, `llvm`, `lldb`, `gcc`/`g++`, `cppcheck`, `clang-tools`, `clang-format`

> Tested on Fedora 43; Debian/Ubuntu (`apt` is also supported).

### 3. Build and install

```bash
make install
```

This builds `liburing`, sets up the venv, and installs the package in editable mode
(`pip install -e .`).

### 4. Run the checks

```bash
make test-all  # Python + C tests
make lint  # formatter + aggressive warnings + cppcheck
make sanitize  # Run examples and tests under ASAN
```

## Pull Request Checks

Before opening a PR, please run locally:

```bash
make test-all
make lint
make sanitize
```

## Prerequisites

| Tool                          | Purpose                                          |
|-------------------------------|--------------------------------------------------|
| Linux kernel 6.11             | io_uring and its modern functionality |
| Python 3.12 (+ `venv`)        | build/test the Python bindings                   |
| C compiler (`clang`/`gcc`)    | build the native extension and C-level tests     |
| `clang-format`, `clang-tools` | formatting / static analysis                     |
| `cppcheck`                    | static analysis                                  |
| ASan/UBSan-capable libc       | `make *-asan` targets                            |
| `liburing` (git submodule)    | core io_uring bindings                           |

All of the above are installed automatically via `make dev-deps` on `apt`/`dnf`
systems. On other package managers, install the equivalents manually.

## Available Commands

### Build & Install

| Command | Description |
|---------|-------------|
| `make build` | Build the wheel |
| `make install` | Install the package (editable, venv) |
| `make deps` | Install Python dev headers, venv, and liburing |
| `make dev-deps` | `deps` + native dev tools (clang, cppcheck, etc.) |
| `make clean` | Remove build artifacts, the venv, and clean liburing |
| `make help` | Show all available commands |

### Tests

| Command | Description |
|---------|-------------|
| `make test-python` | Run pytest (Python only) |
| `make test-python-asan` | Python tests under ASan + UBSan |
| `make test-python-asan-one TEST=path::name` | Run a single pytest test under ASan |
| `make test-c-files` | C-level tests for `ops/files/files.c` |
| `make test-c-files-asan` | Same, under ASan + UBSan |
| `make test-c-sockets` | C-level tests for `ops/sockets/sockets.c` |
| `make test-c-sockets-asan` | Same, under ASan + UBSan |
| `make test-c-buffers` | C-level tests for `buffer_controllers/*.c` |
| `make test-c-buffers-asan` | Same, under ASan + UBSan |
| `make test-c-registry` | C-level tests for `registry/registry.c` |
| `make test-c-registry-asan` | Same, under ASan + UBSan |
| `make test-all` | Python + all C tests (plain) |
| `make test-all-asan` | Python + all C tests (ASan + UBSan) |
| `make run-examples` | Run `docs/examples/*.py` under ASan |
| `make sanitize` | `dev-deps` + `run-examples` + every `*-asan` test target |

### Linting

| Command | Description |
|---------|-------------|
| `make lint` | Run all linters (formatter + aggressive warnings + cppcheck) |
| `make lint-formatter` | Auto-format all `.c`/`.h` files with `clang-format` |
| `make lint-aggressive` | Rebuild the extension with strict warning flags (`-Wall -Wextra -Werror -Wshadow -Wconversion ...`) |
| `make lint-cppcheck` | Static analysis with `cppcheck` (exhaustive check level) |

## Configuration Files

| File              | Purpose |
|-------------------|---------|
| `Makefile`        | All build/test/lint automation |
| `vendor/liburing` | `liburing` git submodule |
| `setup.py`        | Python extension build config |

## Quality Check Details

### 1. Formatting

**Tool:** clang-format
**Command:** `make lint-formatter`

Formats every `.c`/`.h` file under `src/` in place.

### 2. Aggressive Warnings

**Tool:** clang, with `-Wall -Wextra -Werror -Wshadow -Wconversion -Wcast-align
-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Wformat=2 -Wundef
-Wwrite-strings -Wswitch-enum -Wunreachable-code -Wdouble-promotion -Wfloat-equal`
**Command:** `make lint-aggressive`

Rebuilds the extension with a strict warning set to catch bugs the normal build
wouldn't flag.

### 3. Static Analysis

**Tool:** cppcheck (exhaustive)
**Command:** `make lint-cppcheck`

Runs `cppcheck --enable=all --inconclusive --check-level=exhaustive` over `src/`.

### 4. Sanitizers

**Tools:** AddressSanitizer, UndefinedBehaviorSanitizer
**Commands:** `make test-all-asan`, `make sanitize`, `make run-examples`

Builds the extension and C test binaries with `-fsanitize=address,undefined` and
runs them with `ASAN_OPTIONS`/`UBSAN_OPTIONS` set to halt on the first error.

## Handling CI Failures

If CI fails but local checks pass:

1. Make sure the submodule and dependencies are current:

   ```bash
   git submodule update --init --recursive
   make dev-deps
   ```

2. Sync with `main`:

   ```bash
   git fetch origin
   git rebase origin/main
   ```

3. Re-run the full suite:

   ```bash
   make test-all
   make lint
   ```

## Troubleshooting

### Submodule missing

```bash
make check-submodule
# or manually
git submodule update --init --recursive
```

### Missing system packages

```bash
make install-dev-tools    # clang, cppcheck, etc.
make install-python-dev   # Python.h headers
```

### Wrong ASan library path

`ASAN_LIB` is auto-detected via `ldconfig`. If detection fails, override it:

```bash
make test-all-asan ASAN_LIB=/path/to/libasan.so
```

### Stale build artifacts

```bash
make clean
make install
```
## Notes about memory management
While in not little C-project memory handling could easily make project fragile, when you are bridging it with CPython memory management it just causes headaches.

So you must know that

- Event loop and ring are linked
- That's why (almost) all objects are linked through them
- And there are two types of objects:
    - Pure C-objects. This objects we should handle through standard allocation methods 
      - CPython objects - they should be allocated/deacllocated only with CPython instrumentation
- There are cases when it's better to handle objects manually in C and their nature is C, but they need to be pushed in Python memory. There is CPython `Capsule` for this case, better not to multiply Python objects

## Notes about style
Style could be inconsistent, because: some things clang formatter cant fix; and style of project has been distillated at time and some artifacts are still left in project.

1.One-line returners are better:
```
if (!example)
    return NULL;
```
then this:
```
if (!example) {
    return NULL;
}
```
2. Function definitions.
This is better:
```
int
example(void)
{
```
then this:
```
int
example(void) {
```
and then this:
```
int example(void) {
```

## Additional Resources

- [liburing](https://github.com/axboe/liburing) - underlying io_uring library
- [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [cppcheck](https://cppcheck.sourceforge.io/)

If you face some problems, please leave an issue on [GitHub](https://github.com/AivazianArtur/aio_uring/issues)
