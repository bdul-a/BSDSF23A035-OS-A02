# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -D_GNU_SOURCE -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Files
SRC = $(SRC_DIR)/ls-v1.1.0.c
OBJ = $(OBJ_DIR)/ls-v1.1.0.o
BIN = $(BIN_DIR)/ls

# Default target
all: $(BIN)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)

clean:
	rm -f $(OBJ) $(BIN)
