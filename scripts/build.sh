#!/bin/bash
# DevilCore Build Script (Multiboot 32-bit GRUB)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

echo "=== DevilCore OS v2.1 Multiboot Build ==="

rm -rf build
mkdir -p build/iso/boot/grub

echo "[1/4] Assembling kernel_boot.asm..."
nasm -f elf32 kernel/kernel_boot.asm -o build/boot.o

echo "[2/4] Compiling kernel source..."
# Explicit C files to ensure we only build the GUI parts
C_FILES="kernel/kernel.c kernel/drivers/ps2.c kernel/gui/compositor.c kernel/gui/window.c"
OBJ_FILES="build/boot.o"

for file in $C_FILES; do
    if [ -f "$file" ]; then
        basename=$(basename "$file" .c)
        echo "  -> compiling $file"
        gcc -m32 -ffreestanding -nostdlib -fno-pie -O2 -Ikernel -Ikernel/gui -Ikernel/drivers -c "$file" -o "build/${basename}.o"
        OBJ_FILES="$OBJ_FILES build/${basename}.o"
    else
        echo "  !! ERROR: File $file not found!"
        exit 1
    fi
done

echo "[3/4] Linking kernel..."
ld -m elf_i386 -T kernel/kernel.ld -o build/kernel.bin $OBJ_FILES

echo "[4/4] Generating ISO with GRUB..."
cp build/kernel.bin build/iso/boot/
cat > build/iso/boot/grub/grub.cfg << EOF
set timeout=0
set default=0
menuentry "DevilCore OS" {
    multiboot /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o DevilCore.iso build/iso

echo "=== BUILD COMPLETE ==="
ls -lh DevilCore.iso build/kernel.bin