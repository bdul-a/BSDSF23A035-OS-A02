# Simple build file for your ls project (v1.4.0)
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
SRCDIR = src
BINDIR = bin
VERSION = v1.4.0
SRC = $(SRCDIR)/ls-$(VERSION).c
OUT_VER = $(BINDIR)/ls-$(VERSION)
OUT = $(BINDIR)/ls

.PHONY: all clean run

all: $(OUT)

$(BINDIR):
	mkdir -p $(BINDIR)

# build versioned binary
$(OUT_VER): | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

# create unversioned symlink/executable (bin/ls)
$(OUT): $(OUT_VER)
	# copy the versioned binary to a friendly name (replace if exists)
	cp -f $(OUT_VER) $(OUT)
	chmod +x $(OUT)

# convenience target to run from repo root
run: all
	./$(OUT)

clean:
	rm -rf $(BINDIR) ./ls

