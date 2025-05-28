# Compiler and flags
CC := gcc
# CFLAGS := -D_GNU_SOURCE -std=c99 -Wall -Werror -O3 -g -O0
CFLAGS := -D_GNU_SOURCE -std=c99 -Wall -Werror -O3 -g
LDFLAGS := -lssl -lcrypto

# Directories
SRC_DIR := ../src
OBJ_DIR := ./objects
BIN := ../sample

# Source and object files
SRC_FILES := $(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Default target
all: $(BIN)

# Link final binary
$(BIN): $(OBJ_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
