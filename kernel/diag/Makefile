# DevilCore OS — Diagnostic Subsystem Makefile

include ../../config.mk

CFLAGS += -I. -I../.. -O2 -Wall -Wextra -fno-pic -fno-omit-frame-pointer
CFLAGS += -mno-red-zone -mno-sse -mno-mmx -mno-sse2 -mno-sse3 -mno-3dnow

OBJS = diag.o diag_cpu.o diag_memory.o diag_process.o diag_fs.o \
       diag_graphics.o diag_io.o diag_security.o diag_ipc.o \
       diag_timing.o diag_network.o diag_debug.o diag_extra.o

all: $(OBJS)

diag.o: diag.c diag.h
	$(CC) $(CFLAGS) -c diag.c -o diag.o

diag_cpu.o: diag_cpu.c diag.h
	$(CC) $(CFLAGS) -c diag_cpu.c -o diag_cpu.o

diag_memory.o: diag_memory.c diag.h
	$(CC) $(CFLAGS) -c diag_memory.c -o diag_memory.o

diag_process.o: diag_process.c diag.h
	$(CC) $(CFLAGS) -c diag_process.c -o diag_process.o

diag_fs.o: diag_fs.c diag.h
	$(CC) $(CFLAGS) -c diag_fs.c -o diag_fs.o

diag_graphics.o: diag_graphics.c diag.h
	$(CC) $(CFLAGS) -c diag_graphics.c -o diag_graphics.o

diag_io.o: diag_io.c diag.h
	$(CC) $(CFLAGS) -c diag_io.c -o diag_io.o

diag_security.o: diag_security.c diag.h
	$(CC) $(CFLAGS) -c diag_security.c -o diag_security.o

diag_ipc.o: diag_ipc.c diag.h
	$(CC) $(CFLAGS) -c diag_ipc.c -o diag_ipc.o

diag_timing.o: diag_timing.c diag.h
	$(CC) $(CFLAGS) -c diag_timing.c -o diag_timing.o

diag_network.o: diag_network.c diag.h
	$(CC) $(CFLAGS) -c diag_network.c -o diag_network.o

diag_debug.o: diag_debug.c diag.h
	$(CC) $(CFLAGS) -c diag_debug.c -o diag_debug.o

diag_extra.o: diag_extra.c diag.h
	$(CC) $(CFLAGS) -c diag_extra.c -o diag_extra.o

clean:
	rm -f $(OBJS) *.o

.PHONY: all clean
