#!/bin/bash
# Build ISO image

set -e

echo "[DevilISO] Creating ISO image..."

ISO_DIR="build/iso"
KERNEL="build/kernel.elf"
INITRD="build/initrd.img"

mkdir -p "$ISO_DIR/EFI/DEVILCORE"
mkdir -p "$ISO_DIR/boot"

# Copy kernel
cp "$KERNEL" "$ISO_DIR/EFI/DEVILCORE/"

# Create EFI bootloader
cp bootloader/stage2/*.efi "$ISO_DIR/EFI/DEVILCORE/" 2>/dev/null || true

# Create ISO
xorriso -as mkisofs \
    -o build/DevilCore.iso \
    -iso-level 3 \
    -e EFI/DEVILCORE/bootx64.efi \
    -no-emul-boot \
    -boot-load-size 4 \
    "$ISO_DIR"

echo "[DevilISO] ISO created: build/DevilCore.iso"
echo "Size: $(du -h build/DevilCore.iso | cut -f1)"