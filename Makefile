# Tested on Fedora 43 and WSL2 for Windows10

LIBURING_DIR := requirements/liburing
LIBURING_LIB := $(LIBURING_DIR)/src/liburing.a

PYTHON := python3
VENV := .venv
PIP := $(VENV)/bin/pip
PY := $(VENV)/bin/python
VENV_STAMP := $(VENV)/.stamp

EXAMPLES_DIR := docs/examples
TESTS_DIR := tests

ASAN_LIB := $(shell \
	if command -v ldconfig >/dev/null 2>&1; then \
		ldconfig -p | awk '/libasan\.so/ {print $$NF; exit}'; \
	else \
		echo /lib64/libasan.so.8; \
	fi)

ASAN_CFLAGS := -O0 -g3 -DPURING_DEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
ASAN_LDFLAGS := -fsanitize=address,undefined

ASAN_OPTIONS := detect_leaks=0:abort_on_error=1:halt_on_error=1:strict_string_checks=1
UBSAN_OPTIONS := print_stacktrace=1:halt_on_error=1

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


$(VENV_STAMP): Makefile
	@echo "Creating/updating virtualenv..."
	$(PYTHON) -m venv $(VENV)
	$(PIP) install --upgrade pip setuptools wheel build pytest
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

sanitize: dev-deps run-examples test-asan

run-examples: dev-deps
	@echo "START RUNNING EXAMPLES[ASan]"
	@echo "--------"

	CFLAGS="$(ASAN_CFLAGS)" \
	LDFLAGS="$(ASAN_LDFLAGS)" \
	$(PY) setup.py build_ext --inplace

	@fail=0; \
	for f in $$(find $(EXAMPLES_DIR) -type f -name '*.py' | sort); do \
		base=$$(basename $$f); \
		case $$base in \
			_*) echo ">>> Skipping $$f (private)"; continue ;; \
		esac; \
		echo ">>> Running $$f"; \
		LD_PRELOAD=$(ASAN_LIB):$$LD_PRELOAD \
		PYTHONMALLOC=malloc \
		ASAN_OPTIONS="$(ASAN_OPTIONS)" \
		UBSAN_OPTIONS="$(UBSAN_OPTIONS)" \
		$(PY) -X faulthandler $$f; \
		status=$$?; \
		if [ $$status -ne 0 ]; then \
			echo "!!! FAILED: $$f (exit $$status)"; \
			fail=1; \
		fi; \
	done; \
	echo "--------"; \
	if [ $$fail -ne 0 ]; then \
		echo "One or more examples FAILED"; \
		exit 1; \
	else \
		echo "All examples passed"; \
	fi
	@echo "========"


test: install
	@echo "START TESTING[plain]"
	@echo "--------"
	$(PY) -m pytest $(TESTS_DIR) -vv
	@echo "========"

test-asan: dev-deps
	@echo "START TESTING[ASan-UBSan]"
	@echo "--------"

	CFLAGS="$(ASAN_CFLAGS)" \
	LDFLAGS="$(ASAN_LDFLAGS)" \
	$(PY) setup.py build_ext --inplace

	ASAN_OPTIONS="$(ASAN_OPTIONS)" \
	UBSAN_OPTIONS="$(UBSAN_OPTIONS)" \
	LD_PRELOAD=$(ASAN_LIB):$$LD_PRELOAD \
	PYTHONMALLOC=malloc \
	$(PY) -X faulthandler -m pytest $(TESTS_DIR) -vv

	@echo "========"

test-asan-one: dev-deps
	@if [ -z "$(TEST)" ]; then \
		echo "Usage: make test-asan-one TEST=tests/...::test_name"; \
		exit 1; \
	fi

	@echo "START TESTING ONE [ASan-UBSan]"
	@echo ">>> $(TEST)"
	@echo "--------"

	CFLAGS="$(ASAN_CFLAGS)" \
	LDFLAGS="$(ASAN_LDFLAGS)" \
	$(PY) setup.py build_ext --inplace

	ASAN_OPTIONS="$(ASAN_OPTIONS)" \
	UBSAN_OPTIONS="$(UBSAN_OPTIONS)" \
	LD_PRELOAD=$(ASAN_LIB):$$LD_PRELOAD \
	PYTHONMALLOC=malloc \
	$(PY) -X faulthandler -m pytest "$(TEST)" -vv -s

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
	-D"Py_RETURN_FALSE=return Py_False;" \
	-D"Py_RETURN_TRUE=return Py_True;" \
	-D"Py_RETURN_NONE=return Py_None;" \
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
	@echo "── Examples ────────────────────────────────────────"
	@echo "  make run-examples         - run all docs/examples/*.py under ASan"
	@echo ""
	@echo "── Tests ───────────────────────────────────────────"
	@echo "  make test                 - run pytest"
	@echo "  make test-asan            - ASan + UBSan tests"
	@echo ""
	@echo "── Sanitizers ─────────────────────────────────────"
	@echo "  ASAN_LIB=$(ASAN_LIB)"


.PHONY: all deps dev-deps build install clean help check-submodule venv \
        install-python-venv install-python-dev install-dev-tools \
        run-examples sanitize test test-asan test-asan-one \
        lint lint-formatter lint-aggressive lint-cppcheck
