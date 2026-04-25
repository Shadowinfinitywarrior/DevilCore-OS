# DevilCore Kernel Makefile

CC = x86_64-elf-gcc
AS = nasm
LD = x86_64-elf-ld

CFLAGS = -m64 -ffreestanding -fno-pie -nostdinc -nostdlib -Wall -Wextra
ASFLAGS = -f elf64
LDFLAGS = -T kernel.ld --build-id=none

KERNEL_SRC = \
	init/main.c \
	arch/x86_64/gdt.c \
	arch/x86_64/idt.c \
	arch/x86_64/paging.c \
	arch/x86_64/context_switch.c \
	mm/pmm.c \
	mm/vmm.c \
	mm/heap.c \
	proc/process.c \
	proc/thread.c \
	proc/scheduler.c \
	fs/vfs.c \
	drivers/char/tty.c \
	net/ethernet.c \
	net/socket.c \
	crypto/sha256.c \
	lib/string.c \
	lib/printf.c \
	syscall/syscall_table.c \
	diag/diag.c \
	diag/diag_cpu.c \
	diag/diag_memory.c \
	diag/diag_process.c \
	diag/diag_fs.c \
	diag/diag_graphics.c \
	diag/diag_io.c \
	diag/diag_security.c \
	diag/diag_ipc.c \
	diag/diag_timing.c \
	diag/diag_network.c \
	diag/diag_debug.c \
	diag/diag_extra.c

KERNEL_OBJ = $(KERNEL_SRC:.c=.o)
ASM_OBJ = \
	arch/x86_64/gdt_flush.o \
	arch/x86_64/isr.o \
	arch/x86_64/context_switch.o

all: kernel.elf

kernel.elf: $(KERNEL_OBJ) $(ASM_OBJ) kernel.ld
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(KERNEL_OBJ) $(ASM_OBJ) kernel.elf

.PHONY: all clean