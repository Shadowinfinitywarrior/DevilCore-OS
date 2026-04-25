#!/bin/bash
# DevilCore Build Script (Multiboot 32-bit GRUB)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

echo "=== DevilCore OS v2.0 Build ==="

rm -rf build
mkdir -p build/iso/boot/grub

echo "[1/4] Assembling boot.asm..."
nasm -f elf32 kernel/boot.asm -o build/boot.o

echo "[2/4] Compiling kernel source..."
gcc -m32 -ffreestanding -nostdlib -fno-pie -O2 -c kernel/kernel.c -o build/kernel.o

echo "[3/4] Linking kernel..."
ld -m elf_i386 -T kernel/kernel.ld -o build/kernel.bin build/boot.o build/kernel.o

echo "[4/4] Generating ISO with GRUB..."
cp build/kernel.bin build/iso/boot/
cat > build/iso/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0
menuentry "DevilCore OS v2.0" {
    multiboot /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o DevilCore_v2.0.iso build/iso 2>/dev/null || \
xorriso -as mkisofs -o DevilCore_v2.0.iso -b boot/grub/eltorito.img -no-emul-boot \
    -boot-load-size 4 -boot-info-table -iso-level 2 build/iso/

echo "=== BUILD COMPLETE ==="
ls -lh DevilCore_v2.0.iso build/kernel.bin