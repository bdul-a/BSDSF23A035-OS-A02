CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
SRC = src/ls-v1.2.0.c
BIN = bin/ls-v1.2.0

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p $(dir $(BIN))
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf bin
