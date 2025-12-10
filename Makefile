SHELL := /bin/bash

CC := gcc
CFLAGS += -D_POSIX_C_SOURCE=200112L -pthread -Wall -Wextra -O2

CWD := $(realpath .)
OUT := $(CWD)/build

.PHONY: all $(OUT) clean

all: $(OUT)
	@if [[ -z "$(P1)" ]]; then echo "Part 1 directory 'P1' is not specified"; exit 1; fi
	@if [[ -z "$(P2)" ]]; then echo "Part 2 directory 'P2' is not specified"; exit 1; fi
	@for tb in $(P2)/*.c; do                                                           \
		b=$$(basename $$tb .c);                                                        \
		echo "Compiling $$b ...";                                                      \
		$(CC) $(CFLAGS) -Iinclude -I$(P1) -I$(P2) $$tb $(P1)/channel.c -o $(OUT)/$$b;  \
	done

$(OUT):
	mkdir -p $@

clean:
	-rm -rf $(OUT)
