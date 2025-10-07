# Makefile for ls clone
CC = gcc
CFLAGS = -Wall -g

SRC = src/ls-v1.1.0.c
OBJ = obj/ls-v1.1.0.o
BIN = bin/ls

all: $(BIN)

$(OBJ): $(SRC) | obj
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

$(BIN): $(OBJ) | bin
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)

obj:
	mkdir -p obj

bin:
	mkdir -p bin

clean:
	rm -f $(OBJ) $(BIN)

run: $(BIN)
	./$(BIN)
