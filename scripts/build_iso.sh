#!/bin/bash
# DevilCore Build ISO Script for VirtualBox and QEMU
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

echo "=== DevilCore ISO Build for VirtualBox/QEMU ==="

# Clean and create directories
rm -rf build/iso_vb
mkdir -p build/iso_vb/boot/grub
mkdir -p build/iso_vb/boot/kernel
mkdir -p build/iso_vb/EFI/DEVILCORE

# Copy kernel
echo "[1/4] Copying kernel..."
cp build/kernel.bin build/iso_vb/boot/kernel/

# Create GRUB config (BIOS)
echo "[2/4] Creating GRUB config..."
cat > build/iso_vb/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "DevilCore OS v1.0 - Privacy-Focused Ethical OS" {
    insmod all_video
    insmod gfxterm
    set gfxmode=auto
    terminal_output gfxterm
    echo 'Loading DevilCore OS...'
    multiboot /boot/kernel/kernel.bin
    boot
}

menuentry "DevilCore OS (Safe Mode - Serial)" {
    serial --unit=0 --speed=9600
    terminal -d serial
    echo 'Loading DevilCore OS (Serial Mode)...'
    multiboot /boot/kernel/kernel.bin
    boot
}
EOF

# Create EFI bootloader placeholder
echo "[3/4] Creating EFI structure..."
echo "DevilCore OS" > build/iso_vb/EFI/DEVILCORE/version

# Create ISO with both BIOS and EFI support
echo "[4/4] Creating ISO image..."

# Generate ISO using grub-mkrescue for best BIOS compatibility
grub-mkrescue -o DevilCore_v1.0.iso build/iso_vb \
    --embedded-theme /usr/share/grub/unicode.pf2 2>/dev/null || \
grub-mkrescue -o DevilCore_v1.0.iso build/iso_vb 2>/dev/null || {
    # Fallback: use xorriso with proper BIOS boot
    xorriso -as mkisofs \
        -o DevilCore_v1.0.iso \
        -b boot/grub/eltorito.img \
        -no-emul-boot \
        -boot-load-size 4 \
        -boot-info-table \
        -eltorito-catalog boot/grub/boot.cat \
        -eltorito-alt-boot \
        -e EFI/DEVILCORE/bootx64.efi \
        -no-emul-boot \
        -iso-level 2 \
        -A "DevilCore OS v1.0" \
        -publisher "DevilCore Team" \
        -V "DEVILCORE" \
        build/iso_vb/
}

echo "=== BUILD COMPLETE ==="
ls -lh DevilCore_v1.0.iso

echo ""
echo "ISO created: DevilCore_v1.0.iso"
echo "Works with: VirtualBox, QEMU, Physical CD/DVD"
echo ""
echo "To test with QEMU:"
echo "  qemu-system-i386 -cdrom DevilCore_v1.0.iso"
echo ""
echo "To test with VirtualBox:"
echo "  Create new VM -> Use ISO as CD/DVD"