#!/bin/bash
# DevilCore Clean Build Script
set -e

cd /home/darkdevil404/DevilCore

echo "=== DevilCore Clean Build ==="

# Create directories
rm -rf build/iso
mkdir -p build/iso/boot/grub

# Step 1: Assemble boot code (multiboot header)
echo "[1/6] Assembling boot code..."
nasm -f elf32 kernel/boot.asm -o build/boot.o

# Step 2: Compile kernel (32-bit, freestanding, no libraries)
echo "[2/6] Compiling kernel..."
gcc -m32 -ffreestanding -nostdlib -fno-pie -O2 -c kernel/kernel.c -o build/kernel_c.o

# Step 3: Link into flat binary
echo "[3/6] Linking kernel..."
ld -m elf_i386 -T kernel/kernel.ld -o build/kernel.bin build/boot.o build/kernel_c.o

# Step 4: Copy kernel to ISO
echo "[4/6] Preparing ISO..."
cp build/kernel.bin build/iso/boot/

# Step 5: Create GRUB config with multiboot
cat > build/iso/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "DevilCore OS v3.0" {
    multiboot /boot/kernel.bin
    boot
}
EOF

# Step 6: Create ISO
echo "[5/6] Creating ISO image..."
xorriso -as mkisofs \
    -o DevilCore_v3.0.iso \
    -b boot/grub/eltorito.img \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -eltorito-catalog boot/grub/boot.cat \
    -iso-level 2 \
    -A "DevilCore OS" \
    -publisher "DevilCore Team" \
    -V "DEVILCORE" \
    build/iso/ 2>/dev/null || \
grub-mkrescue -o DevilCore_v3.0.iso build/iso/ 2>/dev/null || {
    cp /usr/lib/grub/i386-pc/boot_hybrid.img build/iso/boot/
    grub-mkrescue -o DevilCore_v3.0.iso build/iso/
}

echo "[6/6] Verifying..."
echo "=== BUILD COMPLETE ==="
ls -lh DevilCore_v3.0.iso
file DevilCore_v3.0.iso
file build/kernel.bin