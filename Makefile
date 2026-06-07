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
	$(PIP) install --upgrade pip setuptools wheel build
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

install-dev-tools:
	@echo "Installing dev tools via apt..."
	sudo apt update && sudo apt install -y \
		clang \
		llvm \
		lldb \
		gcc \
		g++ \
		cppcheck \
		clang-tools \
		libclang-dev \
		clang-format \
		python3-dev

else ifeq ($(PKG_MANAGER),dnf)
install-python-venv:
	@echo "Python venv is included with python3 on dnf-based systems, skipping."

install-python-dev:
	@find /usr/include -name Python.h 2>/dev/null | grep -q . || \
		sudo dnf install -y python3-devel --refresh --setopt=minrate=0 --setopt=timeout=300

install-dev-tools:
	@echo "Installing dev tools via dnf..."
	sudo dnf install -y \
		clang \
		llvm \
		lldb \
		gcc \
		gcc-c++ \
		cppcheck \
		clang-tools-extra \
		clang-analyzer \
		clang-devel \
		clang-format \
		python3-devel

else
install-python-venv:
	@echo "Unknown package manager '$(PKG_MANAGER)'. Please install python3-venv manually."
	@exit 1

install-python-dev:
	@echo "Unknown package manager '$(PKG_MANAGER)'. Please install python3-dev/devel manually."
	@exit 1

install-dev-tools:
	@echo "Unknown package manager '$(PKG_MANAGER)'. Please install dev tools manually."
	@exit 1
endif

$(LIBURING_LIB): check-submodule
	@echo "Building liburing..."
	$(MAKE) -C $(LIBURING_DIR)

deps: install-python-dev venv $(LIBURING_LIB)

dev-deps: deps install-dev-tools

build: deps
	$(PY) -m build --wheel

install: deps
	$(PIP) install -e .

sanitize: dev-deps sanitize-asan_ubsan sanitize-tsan sanitize-msan

sanitize-asan_ubsan: dev-deps
	@echo "START SANITIZING[ASan-UBSan]"
	@echo "--------"
	CFLAGS="-O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer" \
	LDFLAGS="-fsanitize=address,undefined" \
	$(PY) setup.py build_ext --inplace
	@echo "========"

sanitize-tsan: dev-deps
	@echo "START SANITIZING[TSan]"
	@echo "--------"
	CC=clang \
	CFLAGS="-O0 -g3 -fsanitize=thread -fno-omit-frame-pointer" \
	LDFLAGS="-fsanitize=thread" \
	$(PY) setup.py build_ext --inplace
	@echo "========"

sanitize-msan: dev-deps
	@echo "START SANITIZING[MSan]"
	@echo "--------"
	CC=clang \
	CFLAGS="-O0 -g3 -fsanitize=memory -fno-omit-frame-pointer" \
	LDFLAGS="-fsanitize=memory" \
	$(PY) setup.py build_ext --inplace
	@echo "========"

lint: dev-deps lint-formatter lint-aggressive lint-cppcheck

lint-formatter: dev-deps
	@echo "START LINTING[formatter]"
	@echo "--------"
	cd src && find . \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} \;
	@echo "========"

lint-aggressive: dev-deps
	@echo "START LINTING[Aggressive compilation flags]"
	@echo "--------"
	CC=clang CFLAGS="-O0 -g3 \
	-Wall \
	-Wextra \
	-Werror \
	-Wshadow \
	-Wconversion \
	-Wcast-align \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wpointer-arith \
	-Wformat=2 \
	-Wundef \
	-Wwrite-strings \
	-Wswitch-enum \
	-Wunreachable-code \
	-Wdouble-promotion \
	-Wfloat-equal \
	-fstack-protector-strong \
	-fno-omit-frame-pointer \
	-isystem requirements/liburing/include \
	-isystem requirements/liburing/src/include \
	-isystem requirements/liburing/src \
	-isystem requirements/liburing" \
	$(PY) setup.py build_ext --inplace
	@echo "========"

lint-cppcheck: dev-deps
	@echo "START LINTING[cppcheck]"
	@echo "--------"
	cppcheck \
	--cppcheck-build-dir=cpp-check-results \
	--enable=all \
	--inconclusive \
	--force \
	--inline-suppr \
	--check-level=exhaustive \
	--suppress=missingIncludeSystem \
	--suppress=missingInclude \
	-I ./src ./src
	@echo "========"

clean:
	rm -rf build dist *.egg-info $(VENV)
	-@$(MAKE) -C $(LIBURING_DIR) clean

help:
	@echo "Detected package manager: $(PKG_MANAGER)"
	@echo ""
	@echo "── Build ──────────────────────────────────────────"
	@echo "  make install              - build and install puring (venv)"
	@echo "  make build                - build wheel"
	@echo "  make clean                - clean everything"
	@echo ""
	@echo "── Sanitizers (require dev tools) ─────────────────"
	@echo "  make sanitize             - run all sanitizers"
	@echo "  make sanitize-asan_ubsan  - AddressSanitizer + UBSan"
	@echo "  make sanitize-tsan        - ThreadSanitizer (clang)"
	@echo "  make sanitize-msan        - MemorySanitizer (clang)"
	@echo ""
	@echo "── Linters (require dev tools) ────────────────────"
	@echo "  make lint                 - run all linters"
	@echo "  make lint-formatter       - clang-format"
	@echo "  make lint-aggressive      - aggressive compiler flags"
	@echo "  make lint-cppcheck        - cppcheck"
	@echo ""
	@echo "── Setup ──────────────────────────────────────────"
	@echo "  make install-dev-tools    - install clang/gcc/valgrind/etc"

.PHONY: all deps dev-deps build install clean help check-submodule venv \
        install-python-venv install-python-dev install-dev-tools \
        sanitize sanitize-asan_ubsan sanitize-tsan sanitize-msan \
        lint lint-formatter lint-aggressive lint-cppcheck
