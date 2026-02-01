SRC_DIR = src
LIB_DIR = include
BIN_DIR = bin

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -I$(LIB_DIR)


SERIAL_SRC   = $(SRC_DIR)/stencil_template_serial.c
PARALLEL_SRC = $(SRC_DIR)/stencil_template_parallel.c

SERIAL_BIN   = $(BIN_DIR)/serial
PARALLEL_BIN = $(BIN_DIR)/parallel

.PHONY: all serial parallel run clean

all: serial parallel

$(SERIAL_BIN): $(SERIAL_SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(PARALLEL_BIN): $(PARALLEL_SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@
.PHONY: serial parallel run-serial run-parallel run clean

# --- build ---
serial: $(SERIAL_BIN)

parallel: $(PARALLEL_BIN)

# --- run ---
run-serial: serial
	@echo "Running SERIAL..."
	./$(SERIAL_BIN)

run-parallel: parallel
	@echo "Running PARALLEL..."
	./$(PARALLEL_BIN)

run: run-serial run-parallel
