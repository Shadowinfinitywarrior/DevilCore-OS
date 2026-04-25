#!/bin/bash
# DevilCore 64-bit Build Script with ISO
set -e

cd /home/darkdevil404/DevilCore

echo "=== DevilCore 64-bit Build ==="

rm -rf build/iso
mkdir -p build/iso/boot/grub

# Build kernel
echo "[1/4] Assembling kernel..."
nasm -f elf64 kernel/kernel64.asm -o build/kernel.o 2>/dev/null

echo "[2/4] Compiling C kernel..."
gcc -m64 -ffreestanding -nostdlib -fno-pie -O2 -c kernel/kernel.c -o build/kernel_c.o

echo "[3/4] Linking kernel..."
ld -m elf_x86_64 -T kernel/kernel64.ld -o build/kernel.bin build/kernel.o build/kernel_c.o

# Copy kernel
cp build/kernel.bin build/iso/boot/

# Create GRUB config
cat > build/iso/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "DevilCore OS v3.0 64-bit" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# Make ISO
echo "[4/4] Creating ISO..."
grub-mkrescue -o DevilCore_v3.0.iso build/iso 2>/dev/null || echo "Note: No GRUB, ISO not created"

echo "=== BUILD COMPLETE ==="
ls -lh build/kernel.bin