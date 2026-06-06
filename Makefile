# Tested on Fedora 43 and WSL2 for Windows10
LIBURING_DIR := requirements/liburing
LIBURING_LIB := $(LIBURING_DIR)/src/liburing.a
PYTHON := python3
VENV := .venv
PIP := $(VENV)/bin/pip
PY := $(VENV)/bin/python
VENV_STAMP := $(VENV)/.stamp

PKG_MANAGER := $(shell command -v dnf 2>/dev/null | xargs basename || command -v apt 2>/dev/null | xargs basename)

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

$(VENV_STAMP):
	@echo "Creating virtualenv..."
	$(PYTHON) -m venv $(VENV)
	$(PIP) install --upgrade pip setuptools wheel
	touch $(VENV_STAMP)

venv: install-python-venv $(VENV_STAMP)

ifeq ($(PKG_MANAGER),apt)
install-python-venv:
	@dpkg -s python3-venv >/dev/null 2>&1 || { \
		echo "Installing python3-venv via apt..."; \
		sudo apt update && sudo apt install -y python3-venv; \
	}

install-python-dev:
	@find /usr/include -name Python.h 2>/dev/null | grep -q . || \
		sudo apt update && sudo apt install -y python3-dev

else ifeq ($(PKG_MANAGER),dnf)
install-python-venv:
	@echo "Python venv is included with python3 on dnf-based systems, skipping."

install-python-dev:
	@find /usr/include -name Python.h 2>/dev/null | grep -q . || \
		sudo dnf install -y python3-devel --refresh --setopt=minrate=0 --setopt=timeout=300

else
install-python-venv:
	@echo "Unknown package manager '$(PKG_MANAGER)'. Please install python3-venv manually."
	@exit 1

install-python-dev:
	@echo "Unknown package manager '$(PKG_MANAGER)'. Please install python3-dev/devel manually."
	@exit 1
endif

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
	-@$(MAKE) -C $(LIBURING_DIR) clean

help:
	@echo "Detected package manager: $(PKG_MANAGER)"
	@echo ""
	@echo "make install  - build and install puring (venv)"
	@echo "make build    - build wheel"
	@echo "make clean    - clean everything"

.PHONY: all deps build install clean help check-submodule venv install-python-venv install-python-dev
