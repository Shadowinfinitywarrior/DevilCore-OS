#!/bin/bash
# DevilCore 64-bit Build Script
set -e

cd /home/darkdevil404/DevilCore

echo "=== DevilCore 64-bit Build ==="

# Clean
rm -rf build
mkdir -p build/iso/boot/grub

# Assemble boot sector
echo "[1/5] Assembling boot sector..."
nasm -f bin bootloader/stage1/mbr.asm -o build/iso/boot/mbr.bin

# Assemble Stage 2 loader
echo "[2/5] Assembling Stage 2 loader..."
nasm -f bin bootloader/stage2/loader.asm -o build/iso/boot/loader.bin

# Assemble kernel
echo "[3/5] Assembling kernel..."
nasm -f elf64 kernel/kernel64.asm -o build/kernel.o

# Compile C kernel
echo "[4/5] Compiling C kernel..."
gcc -m64 -ffreestanding -nostdlib -fno-pie -O2 -c kernel/kernel.c -o build/kernel_c.o

# Link kernel
echo "[5/5] Linking kernel..."
ld -m elf_x86_64 -T kernel/kernel64.ld -o build/kernel.bin build/kernel.o build/kernel_c.o

echo "=== BUILD COMPLETE ==="
ls -lh build/kernel.bin