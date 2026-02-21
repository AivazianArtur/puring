LIBURING_DIR = requirements/liburing
LIBURING_INCLUDE = $(LIBURING_DIR)/src/include
LIBURING_LIB = $(LIBURING_DIR)/src/liburing.a

PURNG_SRC = src/puring.c
PURNG_HDR = include/puring.h
PURNG_OBJ = puring.o
PURNG_LIB = libpuring.so

CC = gcc
CFLAGS = -fPIC -Iinclude -I$(LIBURING_INCLUDE)
LDFLAGS = -shared

all: $(PURNG_LIB)

# Step 1: Build liburing.a if it doesn't exist
$(LIBURING_LIB):
	@echo "Building liburing..."
	$(MAKE) -C $(LIBURING_DIR)

# Step 2: Compile puring.o
$(PURNG_OBJ): $(PURNG_SRC) $(PURNG_HDR) $(LIBURING_LIB)
	$(CC) $(CFLAGS) -c $(PURNG_SRC) -o $(PURNG_OBJ)

# Step 3: Build libpuring.so
$(PURNG_LIB): $(PURNG_OBJ)
	$(CC) $(LDFLAGS) -o $(PURNG_LIB) $(PURNG_OBJ) $(LIBURING_LIB)

clean:
	rm -f $(PURNG_OBJ) $(PURNG_LIB)
	$(MAKE) -C $(LIBURING_DIR) clean

.PHONY: all clean
