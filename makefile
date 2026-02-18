############################################################
# Variables
############################################################
SRC_DIR         = src
LIB_DIR         = include
BIN_DIR         = bin
PARALLEL_FUNC_DIR = parallel_func

CC              = gcc
MPICC           = mpicc
CFLAGS          = -Wall -Wextra -O2 -I$(LIB_DIR)

SERIAL_SRC      = $(SRC_DIR)/stencil_template_serial.c
PARALLEL_SRC    = $(SRC_DIR)/stencil_template_parallel.c

PARALLEL_FUNC_SRC = $(wildcard $(PARALLEL_FUNC_DIR)/*.c)
PARALLEL_FUNC_OBJ = $(PARALLEL_FUNC_SRC:.c=.o)

SERIAL_BIN      = $(BIN_DIR)/serial
PARALLEL_BIN    = $(BIN_DIR)/parallel

############################################################
# Build Rules
############################################################
.PHONY: all serial parallel clean run-serial run-parallel run

# Compile all
all: serial parallel

# Compile parallel_func object files
$(PARALLEL_FUNC_OBJ): %.o: %.c
    $(MPICC) $(CFLAGS) -c $< -o $@

# Build serial executable
$(SERIAL_BIN): $(SERIAL_SRC)
    mkdir -p $(BIN_DIR)
    $(CC) $(CFLAGS) $< -o $@

# Build parallel executable
$(PARALLEL_BIN): $(PARALLEL_SRC) $(PARALLEL_FUNC_OBJ)
    mkdir -p $(BIN_DIR)
    $(MPICC) $(CFLAGS) $^ -o $@

# Convenience targets
serial: $(SERIAL_BIN)
parallel: $(PARALLEL_BIN)

# Clean build
clean:
    rm -rf $(BIN_DIR)

############################################################
# Run Rules
############################################################
run-serial: serial
    @echo "Running SERIAL..."
    ./$(SERIAL_BIN)

run-parallel: parallel
    @echo "Running PARALLEL..."
    ./$(PARALLEL_BIN)

run: run-serial run-parallel