TOOLCHAIN_PREFIX ?= x86_64-linux-gnu-
CC := $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld
NASM := nasm
QEMU := qemu-system-x86_64

BUILD_DIR := build
DIST_DIR := dist
ISO_DIR := iso_root
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
HDD_IMAGE := $(DIST_DIR)/DevilCore.img
ISO_IMAGE := $(DIST_DIR)/DevilCore.iso
LIMINE_DIR := third_party/limine
LIMINE := $(LIMINE_DIR)/limine

# No changes needed if wildcards are used correctly. Let's verify.
C_SOURCES := $(wildcard src/kernel/*.c)
# vma.c is in src/kernel/ so it should be picked up.

ASM_SOURCES := $(wildcard src/kernel/*.asm)

C_OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst src/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

CFLAGS := -std=c17 -Os -Wall -Wextra -ffreestanding -fno-stack-protector \
	-fno-pic -fno-asynchronous-unwind-tables -fno-unwind-tables \
	-m64 -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
	-ffunction-sections -fdata-sections \
	-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -I./src/kernel
LDFLAGS := -m elf_x86_64 -T linker.ld -z max-page-size=0x1000 --gc-sections
NASMFLAGS := -f elf64
STRIP := $(TOOLCHAIN_PREFIX)strip

.PHONY: all kernel hdd iso run clean

all: iso

kernel: $(KERNEL_ELF)

hdd: $(HDD_IMAGE)

iso: $(ISO_IMAGE)

$(KERNEL_ELF): $(OBJECTS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	$(STRIP) --strip-all $@

$(ISO_IMAGE): $(KERNEL_ELF) limine.conf
	mkdir -p $(DIST_DIR) $(ISO_DIR)/boot $(ISO_DIR)/EFI/BOOT
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp limine.conf $(ISO_DIR)/limine.conf
	cp limine.conf $(ISO_DIR)/boot/limine.conf
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_DIR)/boot/limine-bios.sys
	cp $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_DIR)/boot/limine-bios-cd.bin
	cp $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_DIR)/boot/limine-uefi-cd.bin
	xorriso -as genisoimage \
		-b boot/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		-eltorito-alt-boot \
		-e boot/limine-uefi-cd.bin \
		-no-emul-boot \
		$(ISO_DIR) -o $(ISO_IMAGE)

$(HDD_IMAGE): $(KERNEL_ELF) limine.conf
	mkdir -p $(DIST_DIR)
	rm -f $(HDD_IMAGE)
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(HDD_IMAGE)
	sgdisk -a 1 $(HDD_IMAGE) \
		-n 1:34:2047 -t 1:ef02 \
		-n 2:2048:+48M -t 2:ef00
	mkdir -p /tmp/dosfs
	mformat -F -i $(HDD_IMAGE)@@1M -T 98304 ::
	mmd -i $(HDD_IMAGE)@@1M ::/EFI
	mmd -i $(HDD_IMAGE)@@1M ::/EFI/BOOT
	mmd -i $(HDD_IMAGE)@@1M ::/boot
	mcopy -i $(HDD_IMAGE)@@1M limine.conf ::/limine.conf
	mcopy -i $(HDD_IMAGE)@@1M limine.conf ::/boot/limine.conf
	mcopy -i $(HDD_IMAGE)@@1M $(LIMINE_DIR)/limine-bios.sys ::/limine-bios.sys
	mcopy -i $(HDD_IMAGE)@@1M $(LIMINE_DIR)/limine-bios.sys ::/boot/limine-bios.sys
	mcopy -i $(HDD_IMAGE)@@1M $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i $(HDD_IMAGE)@@1M $(KERNEL_ELF) ::/boot/kernel.elf
	mcopy -i $(HDD_IMAGE)@@1M limine.conf ::/boot/limine.conf
	mcopy -i $(HDD_IMAGE)@@1M $(LIMINE_DIR)/limine-bios.sys ::/limine-bios.sys
	mcopy -i $(HDD_IMAGE)@@1M $(LIMINE_DIR)/limine-bios.sys ::/boot/limine-bios.sys
	mcopy -i $(HDD_IMAGE)@@1M $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i $(HDD_IMAGE)@@1M $(KERNEL_ELF) ::/boot/kernel.elf

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

run: $(ISO_IMAGE)
	$(QEMU) -m 512M -cdrom $(ISO_IMAGE) -boot d -serial stdio -d int,cpu_reset 2>qemu_debug.log

run-fullscreen: $(ISO_IMAGE)
	$(QEMU) -m 512M -cdrom $(ISO_IMAGE) -boot d -serial stdio -full-screen -d int,cpu_reset 2>qemu_debug.log

run-scaled: $(ISO_IMAGE)
	$(QEMU) -m 512M -cdrom $(ISO_IMAGE) -boot d -serial stdio -vga std -display sdl,gl=off,show-cursor=on -d int,cpu_reset 2>qemu_debug.log

run-window: $(ISO_IMAGE)
	$(QEMU) -m 512M -cdrom $(ISO_IMAGE) -boot d -serial stdio -vga virtio -display gtk,show-cursor=on,zoom-to-fit=off -d int,cpu_reset 2>qemu_debug.log

test: $(ISO_IMAGE)
	rm -f serial.log
	$(QEMU) -m 512M -cdrom $(ISO_IMAGE) -boot d -serial file:serial.log -display none & \
	PID=$$!; \
	sleep 30; \
	kill $$PID; \
	grep "DevilCore OS boot complete!" serial.log && (echo "TEST PASSED"; exit 0) || (echo "TEST FAILED"; exit 1)

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)
	$(MAKE) -C $(LIMINE_DIR) clean