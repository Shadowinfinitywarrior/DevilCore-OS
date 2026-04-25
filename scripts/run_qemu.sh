#!/bin/bash
# DevilCore QEMU Launcher

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

ISO_FILE="$SCRIPT_DIR/DevilCore.iso"

echo "=== DevilCore OS QEMU Launcher ==="

# Find QEMU
QEMU=""
for q in qemu-system-i386 qemu-system-x86_64 qemu; do
    if command -v $q &>/dev/null; then
        QEMU=$q
        break
    fi
done

if [ -z "$QEMU" ]; then
    echo "Error: QEMU not installed"
    exit 1
fi

echo "Running: $QEMU"
exec $QEMU -m 512M -cdrom "$ISO_FILE" -boot d -vga std -device intel-hda -device hda-output -rtc base=localtime