# DevilCore OS v3.0 - Complete

## Files Created

| File | Size | Description |
|------|------|-------------|
| `DevilCore.iso` | 1.8MB | Bootable CD-ROM |
| `DevilCore_floppy.img` | 1.44MB | Bootable floppy |

## Features

### Bootloader (Stage 1)
- 16-bit real mode
- Shows boot menu
- Loads kernel from disk
- Switches to 32-bit protected mode
- Beep sounds on boot

### Kernel (32-bit)
- VESA graphics support (800x600x32)
- GUI Desktop with:
  - Title bar with "DevilCore OS v3.0"
  - Desktop icons: My PC, Documents, Calculator, Trash
  - Taskbar with Start button
  - Real-time clock
- Windows system
- Keyboard driver (PS/2)
- Mouse support

### Apps
- About window
- Calculator
- Documents
- Help system

## Keyboard Shortcuts
- `A` - About window
- `C` - Calculator  
- `D` - Documents
- `F1` - Help

## Boot
```bash
# Floppy (recommended)
qemu-system-i386 -fda DevilCore_floppy.img -m 256

# CD-ROM
qemu-system-i386 -cdrom DevilCore.iso -m 256

# VirtualBox
# New VM → Other/Unknown (32-bit) → Mount DevilCore.iso
```

## Technical Details

- Boot sector: 512 bytes (0x7C00)
- Kernel: 32KB at 0x10000
- Bootloader loads sectors 1-64
- GDT for protected mode
- A20 line enabled

## Build From Source

```bash
cd DevilCore
nasm -f bin kernel/bootloader.asm -o build/boot.bin
nasm -f bin kernel/kernel.asm -o build/kernel.bin
dd if=/dev/zero of=build/floppy.img bs=1024 count=1440
dd if=build/boot.bin of=build/floppy.img conv=notrunc
dd if=build/kernel.bin of=build/floppy.img seek=1 conv=notrunc
```