#!/bin/bash
# ============================================================
#  DevilCore OS v5.0 - Modular Build System
#  Produces: DevilCoreOS.iso  (BIOS + UEFI bootable, 64-bit)
#  Requirements: nasm gcc ld grub-mkrescue xorriso
# ============================================================
set -e
cd "$(dirname "$0")/.."
ROOT="$(pwd)"

echo "=============================================="
echo "  DevilCore OS v5.0 - Build System (64-bit)"
echo "=============================================="

# ---------- Toolchain checks ------------------------------------------
for tool in nasm gcc ld grub-mkrescue xorriso; do
    if ! command -v $tool &>/dev/null; then
        echo "[ERROR] Missing tool: $tool"
        exit 1
    fi
done

# ---------- Directories -----------------------------------------------
BUILD="$ROOT/build_dc"
ISO_STAGE="$BUILD/iso_stage"
rm -rf "$BUILD"
mkdir -p "$BUILD" "$ISO_STAGE/boot/grub"

# ---------- Common CFLAGS ---------------------------------------------
CFLAGS="-m64 -ffreestanding -mcmodel=large -mno-red-zone -nostdlib -nostdinc \
        -fno-pie -fno-stack-protector -fno-builtin -O2 -std=c99 \
        -mno-sse -mno-mmx -mno-sse2 \
        -Wall -Wextra -Wno-unused-parameter -Wno-type-limits \
        -I$ROOT/kernel -I$ROOT/kernel/include"

# ---------- Assembly ---------------------------------------------------
echo ""
echo "[1/8] Assembling multiboot2 boot entry..."
nasm -f elf64 "$ROOT/kernel/boot/multiboot2.asm" -o "$BUILD/multiboot2.o"

echo "[2/8] Assembling ISR and Context Stubs..."
nasm -f elf64 "$ROOT/kernel/arch/isr_stubs.asm" -o "$BUILD/isr_stubs.o"
nasm -f elf64 "$ROOT/kernel/arch/context_switch.asm" -o "$BUILD/context_switch.o"
nasm -f elf64 "$ROOT/kernel/syscall/syscall.asm" -o "$BUILD/syscall_asm.o"

# ---------- C Source Files ---------------------------------------------
echo "[3/8] Compiling 30 C modules..."
for file in \
    libc/string.c \
    arch/idt.c \
    drivers/pic.c drivers/timer.c drivers/keyboard.c drivers/mouse.c \
    drivers/pci.c drivers/ata.c fs/vfs.c \
    drivers/usb/usb.c drivers/usb/uhci.c drivers/usb/usb_hid.c \
    drivers/net/rtl8139.c net/net.c net/eth.c \
    gui/framebuffer.c gui/font.c gui/wm.c gui/terminal.c \
    apps/dsh.c \
    security/crypto.c security/privacy_hub.c security/sandbox.c \
    proc/elf_loader.c \
    mm/pmm.c mm/vmm.c mm/heap.c \
    lib/printf.c \
    proc/process.c proc/scheduler.c \
    syscall/syscall.c \
    diag/diag.c \
    diag/diag_cpu.c diag/diag_memory.c diag/diag_process.c diag/diag_fs.c \
    diag/diag_graphics.c diag/diag_io.c diag/diag_security.c diag/diag_ipc.c \
    diag/diag_timing.c diag/diag_network.c diag/diag_debug.c diag/diag_extra.c \
    kernel_main.c
do
    echo "      CC  $(basename $file)"
    gcc -c kernel/$file -o build_dc/$(basename ${file%.c}.o) $CFLAGS
done

# ---------- Link -------------------------------------------------------
echo "[4/8] Linking kernel ELF..."
ld -n -T kernel/kernel.ld -o build_dc/kernel.elf \
    build_dc/multiboot2.o \
    build_dc/isr_stubs.o build_dc/context_switch.o build_dc/syscall_asm.o \
    build_dc/string.o \
    build_dc/idt.o \
    build_dc/pic.o build_dc/timer.o build_dc/keyboard.o build_dc/mouse.o \
    build_dc/pci.o build_dc/ata.o build_dc/vfs.o \
    build_dc/usb.o build_dc/uhci.o build_dc/usb_hid.o \
    build_dc/rtl8139.o build_dc/net.o build_dc/eth.o \
    build_dc/framebuffer.o build_dc/font.o build_dc/wm.o build_dc/terminal.o \
    build_dc/dsh.o \
    build_dc/crypto.o build_dc/privacy_hub.o build_dc/sandbox.o \
    build_dc/elf_loader.o \
    build_dc/pmm.o build_dc/vmm.o build_dc/heap.o \
    build_dc/printf.o \
    build_dc/process.o build_dc/scheduler.o build_dc/syscall.o \
    build_dc/diag.o \
    build_dc/diag_cpu.o build_dc/diag_memory.o build_dc/diag_process.o \
    build_dc/diag_fs.o build_dc/diag_graphics.o build_dc/diag_io.o \
    build_dc/diag_security.o build_dc/diag_ipc.o build_dc/diag_timing.o \
    build_dc/diag_network.o build_dc/diag_debug.o build_dc/diag_extra.o \
    build_dc/kernel_main.o

# Verify multiboot2 header
if command -v grub-file &>/dev/null; then
    if grub-file --is-x86-multiboot2 "$BUILD/kernel.elf"; then
        echo "      [OK] Multiboot2 header verified"
    else
        echo "[WARN] grub-file multiboot2 check failed"
    fi
fi

echo "[5/8] Staging ISO filesystem..."
cp "$BUILD/kernel.elf" "$ISO_STAGE/boot/kernel.elf"
cp "$ROOT/iso_root/boot/grub/grub.cfg" "$ISO_STAGE/boot/grub/grub.cfg"

if [ -f "$ROOT/logo.png" ]; then
    cp "$ROOT/logo.png" "$ISO_STAGE/boot/grub/background.png" 2>/dev/null || true
fi

echo "[6/8] Creating bootable ISO with grub-mkrescue..."
grub-mkrescue \
    --output="$ROOT/DevilCoreOS.iso" \
    "$ISO_STAGE" \
    2>/dev/null || {
    echo "      grub-mkrescue failed, trying fallback..."
    xorriso -as mkisofs \
        -o "$ROOT/DevilCoreOS.iso" \
        -b boot/grub/eltorito.img \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        -iso-level 2 \
        -A "DevilCore OS v5.0" \
        -publisher "DevilCore Team" \
        -V "DEVILCORE50" \
        "$ISO_STAGE/"
}

echo "[7/8] Build statistics..."
echo "=============================================="
ls -lh "$ROOT/DevilCoreOS.iso"
echo ""
echo "  Kernel ELF:   $(ls -lh $BUILD/kernel.elf | awk '{print $5}')"
echo "  ISO:          $(ls -lh $ROOT/DevilCoreOS.iso | awk '{print $5}')"
echo "  Modules:      ${#C_SOURCES[@]} C files + 2 ASM files"
echo ""
echo "[8/8] Ready!"
echo ""
echo "Test with QEMU (64-bit):"
echo "  qemu-system-x86_64 -cdrom DevilCoreOS.iso -m 512M -vga std -boot d"
echo ""
echo "VirtualBox: New VM → Other/Unknown (64-bit) → Mount DevilCoreOS.iso"
echo "=============================================="
