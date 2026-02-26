LIBURING_DIR := requirements/liburing
LIBURING_LIB := $(LIBURING_DIR)/src/liburing.a

PYTHON := python3
VENV := .venv
PIP := $(VENV)/bin/pip
PY := $(VENV)/bin/python

all: build


check-submodule:
	@if [ ! -f "$(LIBURING_DIR)/Makefile" ]; then \
		echo "Liburing submodule not found. Initializing..."; \
		git submodule update --init --recursive; \
	fi
	@if [ ! -f "$(LIBURING_DIR)/Makefile" ]; then \
		echo "Error: Failed to initialize submodule."; \
		exit 1; \
	fi


$(VENV)/bin/activate:
	@echo "Checking for python3-venv..."
	@if command -v apt > /dev/null; then \
		dpkg -s python3-venv >/dev/null 2>&1 || { \
			echo "Debian-based system detected. Installing via apt..."; \
			sudo apt update && sudo apt install -y python3-venv; \
		}; \
	elif command -v dnf > /dev/null; then \
		dnf list installed python3 >/dev/null 2>&1 || { \
			echo "RedHat-based system detected. Installing via dnf..."; \
			sudo dnf install -y python3; \
		}; \
	else \
		echo "Unknown package manager. Please install python3-venv manually."; \
		exit 1; \
	fi
	@echo "Creating virtualenv..."
	$(PYTHON) -m venv $(VENV)
	$(PIP) install --upgrade pip setuptools wheel

venv: $(VENV)/bin/activate


install-python-dev:
	@echo "Checking for Python development headers..."
	@find /usr/include -name Python.h 2>/dev/null | grep -q . || { \
		if command -v apt > /dev/null; then \
			sudo apt update && sudo apt install -y python3-dev; \
		elif command -v dnf > /dev/null; then \
			sudo dnf install -y python3-devel --refresh --setopt=minrate=0 --setopt=timeout=300 || { echo "DNF failed, check your connection"; exit 1; }; \
		else \
			echo "Manual install of python3-dev/devel required."; exit 1; \
		fi; \
	}



$(LIBURING_LIB): check-submodule
	@echo "Building liburing..."
	$(MAKE) -C $(LIBURING_DIR)

deps: install-python-dev venv $(LIBURING_LIB)


build: deps
	$(PY) -m build --wheel

install: deps
	$(PIP) install -e .


clean:
	rm -rf build dist *.egg-info $(VENV)
	-$(MAKE) -C $(LIBURING_DIR) clean


help:
	@echo "make install  - build and install puring (venv)"
	@echo "make build    - build wheel"
	@echo "make clean    - clean everything"

.PHONY: all deps build install clean help check-submodule venv


BENCHMARK_SCRIPT := docs/benchmark/benchmark.py

run-benchmark: install
	@echo "Running benchmark inside virtualenv..."
	$(PY) $(BENCHMARK_SCRIPT)
