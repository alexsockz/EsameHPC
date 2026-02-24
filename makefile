############################################################
# Variables
############################################################
SRC_DIR         = src
LIB_DIR         = include
BIN_DIR         = bin
PARALLEL_FUNC_DIR = parallel_func

CC              = gcc
MPICC           = mpicc
CFLAGS_COM      = -Wall -Wextra -O2 -I$(LIB_DIR)
CFLAGS_OMP      = -fopenmp

SERIAL_SRC      = $(SRC_DIR)/stencil_template_serial.c
PARALLEL_SRC    = $(SRC_DIR)/stencil_template_parallel.c

PARALLEL_FUNC_SRC = $(wildcard $(PARALLEL_FUNC_DIR)/*.c)
PARALLEL_FUNC_OBJ = $(patsubst $(PARALLEL_FUNC_DIR)/%.c,$(BIN_DIR)/%.o,$(PARALLEL_FUNC_SRC))

SERIAL_BIN      = $(BIN_DIR)/serial
PARALLEL_BIN    = $(BIN_DIR)/parallel


TEST_SRC := $(wildcard tests/*_test.c)
TEST_BIN := $(patsubst tests/%.c,bin/%,$(TEST_SRC))


############################################################
# Build Rules
############################################################
.PHONY: all serial parallel clean run-serial run-parallel run test

# Compile all
all: serial parallel



# Build serial executable
$(SERIAL_BIN): $(SERIAL_SRC)
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile parallel_func object files into bin/
$(BIN_DIR)/%.o: $(PARALLEL_FUNC_DIR)/%.c
	mkdir -p $(BIN_DIR)
	$(MPICC) $(CFLAGS_COM) $(CFLAGS_OMP) -c $< -o $@

# Build parallel executable (update dependency)
$(PARALLEL_BIN): $(PARALLEL_SRC) $(PARALLEL_FUNC_OBJ)
	mkdir -p $(BIN_DIR)
	$(MPICC) $(CFLAGS_COM) $(CFLAGS_OMP) $^ -o $@

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

############################################################
# Test Rules
############################################################

# Rule to build each test
bin/%_test: tests/%_test.c $(PARALLEL_FUNC_OBJ)
	mkdir -p $(BIN_DIR)
	$(MPICC) $(CFLAGS) $^ -o $@

# Run all tests
test: $(TEST_BIN)
	for t in $(TEST_BIN); do $$t; done