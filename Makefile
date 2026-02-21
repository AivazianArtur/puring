# ========================
# Paths
# ========================

LIBURING_DIR := requirements/liburing
LIBURING_LIB := $(LIBURING_DIR)/src/liburing.a

PYTHON := python3
VENV := .venv
PIP := $(VENV)/bin/pip
PY := $(VENV)/bin/python

# ========================
# Default
# ========================

all: build

# ========================
# Submodule check
# ========================

check-submodule:
	@if [ ! -f "$(LIBURING_DIR)/Makefile" ]; then \
		echo "Missing liburing submodule."; \
		echo "Run: git submodule update --init --recursive"; \
		exit 1; \
	fi

# ========================
# Virtual environment
# ========================

$(VENV)/bin/activate:
	@echo "Checking for python3-venv..."
	@dpkg -s python3-venv >/dev/null 2>&1 || { \
		echo "python3-venv not found. Installing..."; \
		sudo apt update && sudo apt install -y python3-venv; \
	}
	@echo "Creating virtualenv..."
	$(PYTHON) -m venv $(VENV)
	$(PIP) install --upgrade pip setuptools wheel

venv: $(VENV)/bin/activate


# ========================
# Build liburing
# ========================

$(LIBURING_LIB): check-submodule
	@echo "Building liburing..."
	$(MAKE) -C $(LIBURING_DIR)

deps: venv $(LIBURING_LIB)

# ========================
# Python build
# ========================

build: deps
	$(PY) -m build --wheel

install: deps
	$(PIP) install -e .

# ========================
# Clean
# ========================

clean:
	rm -rf build dist *.egg-info $(VENV)
	-$(MAKE) -C $(LIBURING_DIR) clean

# ========================
# Help
# ========================

help:
	@echo "make install  - build and install puring (venv)"
	@echo "make build    - build wheel"
	@echo "make clean    - clean everything"

.PHONY: all deps build install clean help check-submodule venv

# ========================
# Benchmark
# ========================

BENCHMARK_SCRIPT := docs/benchmark/benchmark.py

run-benchmark: install
	@echo "Running benchmark inside virtualenv..."
	$(PY) $(BENCHMARK_SCRIPT)
