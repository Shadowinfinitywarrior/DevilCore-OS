# DevilShell Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2

all: devilshell

devilshell: src/main.c
	$(CC) $(CFLAGS) -o $@ src/main.c

clean:
	rm -f devilshell

.PHONY: all clean