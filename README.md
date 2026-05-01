# DevilCore OS

A modern, feature-rich 64-bit operating system written from scratch in C and Assembly. DevilCore OS provides a complete desktop environment with a graphical user interface, window management, and various built-in applications.

![DevilCore OS](https://img.shields.io/badge/DevilCore-OS-blue) ![License](https://img.shields.io/badge/license-MIT-green) ![Arch](https://img.shields.io/badge/arch-x86__64--64-lightgrey)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Requirements](#system-requirements)
- [Architecture](#architecture)
- [Installation](#installation)
- [Building from Source](#building-from-source)
- [Quick Start Guide](#quick-start-guide)
- [User Guide](#user-guide)
- [Applications](#applications)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [Developer Guide](#developer-guide)
- [Project Structure](#project-structure)
- [Kernel Components](#kernel-components)
- [Memory Management](#memory-management)
- [File System](#file-system)
- [Window Manager](#window-manager)
- [Graphics System](#graphics-system)
- [Device Drivers](#device-drivers)
- [Network Stack](#network-stack)
- [Security Features](#security-features)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Roadmap](#roadmap)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Overview

DevilCore OS is a hobbyist operating system project that demonstrates modern OS concepts including:

- **64-bit x86_64 architecture** with full long mode support
- **Custom kernel** written in C and Assembly
- **Graphical User Interface** with window management
- **Multitasking** with preemptive scheduler
- **Virtual File System** with FAT32 support
- **Memory Management** with paging and memory compression
- **Device Drivers** for keyboard, mouse, disk, and network
- **Built-in Applications** including browser, calculator, text editor, and more

The project is designed for educational purposes, showcasing how modern operating systems work under the hood. It serves as a learning platform for OS development, systems programming, and low-level software engineering.

### Project Goals

- Provide a complete, bootable operating system from scratch
- Demonstrate modern OS design patterns and concepts
- Create a usable desktop environment with GUI applications
- Implement essential OS services: memory management, file system, device drivers
- Maintain clean, well-documented code for educational use
- Support common hardware platforms (x86_64)

### Design Philosophy

DevilCore OS follows these design principles:

1. **Simplicity**: Keep components modular and easy to understand
2. **Performance**: Optimize critical paths for responsiveness
3. **Security**: Implement proper isolation and protection mechanisms
4. **Extensibility**: Design for easy addition of new features
5. **Education**: Prioritize code clarity over micro-optimizations

---

## Features

### Core Kernel Features

- **64-bit Long Mode**: Full x86_64 support with 64-bit addressing
- **Multitasking**: Preemptive round-robin scheduler with task priorities
- **Memory Management**: 
  - Physical memory manager with page allocation
  - Virtual memory with paging
  - Memory compression for efficient RAM usage
  - Kernel heap allocator
- **Interrupt Handling**: 
  - IDT (Interrupt Descriptor Table) setup
  - ISR (Interrupt Service Routines) for hardware interrupts
  - Exception handling for CPU faults
- **System Calls**: Syscall interface for user-kernel communication
- **ACPI Support**: Advanced Configuration and Power Interface
- **PCI Enumeration**: PCI device detection and configuration

### Graphical User Interface

- **Window Manager**: 
  - Window creation, movement, resizing, and closing
  - Window focusing and z-order management
  - Title bars with close/minimize/maximize buttons
  - Modern rounded corners and shadows
- **Desktop Environment**:
  - Desktop icons with drag-and-drop support
  - Taskbar with running applications
  - Start menu with categorized applications
  - Context menus for desktop operations
- **Graphics System**:
  - Framebuffer-based rendering
  - Double buffering for flicker-free display
  - Hardware-accelerated drawing primitives
  - Font rendering with anti-aliasing
  - Icon system with 119+ built-in icons
- **Input Handling**:
  - PS/2 keyboard driver with scancode translation
  - PS/2 mouse driver with cursor tracking
  - Event queue for input processing

### File System

- **Virtual File System (VFS)**: Abstraction layer for file operations
- **FAT32 Support**: Read/write access to FAT32 partitions
- **ATA Driver**: IDE/SATA disk controller support
- **File Operations**: Create, read, write, delete, list directories
- **Mount Points**: Support for multiple mounted file systems

### Built-in Applications

1. **Privacy Browser**: 
   - Tabbed browsing interface
   - Modern GUI with address bar and toolbar
   - Privacy-focused design with HTTPS indicators
   - Loading animations and status bar
   - Keyboard shortcuts for navigation

2. **Terminal**:
   - Command-line interface
   - Shell with built-in commands
   - Command history
   - Color output support

3. **File Manager**:
   - Graphical file browser
   - Directory navigation
   - File icons and metadata display
   - Basic file operations

4. **Text Editor**:
   - Simple text editing
   - File open/save operations
   - Basic text manipulation

5. **Calculator**:
   - GUI-based calculator
   - Basic arithmetic operations
   - Modern button layout

6. **Settings**:
   - System configuration interface
   - Display settings
   - Theme customization

7. **System Monitor**:
   - CPU usage display
   - Memory usage statistics
   - Process information

### Network Stack

- **RTL8139 Driver**: Realtek Ethernet controller support
- **Basic Network Stack**: 
  - Ethernet frame handling
  - ARP protocol implementation
  - Basic TCP/IP support (in development)

### Security Features

- **Memory Protection**: Page-level protection between processes
- **Kernel/User Mode Separation**: Privilege level enforcement
- **Memory Compression**: Reduces memory footprint and prevents leaks
- **Private Browsing**: Browser runs in isolated mode by default
- **Ad Blocking**: Built-in ad blocking in browser

---

## System Requirements

### Minimum Requirements

- **Processor**: x86_64 compatible CPU with 64-bit support
- **RAM**: 512 MB (1 GB recommended)
- **Storage**: 100 MB free space
- **Graphics**: VESA-compatible framebuffer (1024x768 minimum)
- **Input**: PS/2 keyboard and mouse

### Recommended Requirements

- **Processor**: Modern x86_64 CPU (Intel Core i5 / AMD Ryzen 5 or better)
- **RAM**: 2 GB or more
- **Storage**: 1 GB or more
- **Graphics**: 1920x1080 resolution support
- **Virtualization**: QEMU 4.0+ or VirtualBox 6.0+ for testing

### Supported Hardware

- **CPUs**: All x86_64 processors with long mode support
- **Chipsets**: Intel/AMD chipsets with standard PCI/PCIe
- **Storage**: IDE/SATA disks (AHCI support in development)
- **Network**: Realtek RTL8139/8169 Ethernet controllers
- **Graphics**: VBE 3.0+ compatible graphics cards

---

## Architecture

### System Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     User Applications                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │
│  │ Browser  │ │ Terminal │ │ File Mgr │ │ Settings │      │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │
└─────────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────────┐
│                   Window Manager (GUI)                       │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Window Management │ Event Handling │ Rendering      │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────────┐
│                      System Call Interface                   │
└─────────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────────┐
│                         Kernel Core                          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │
│  │ Scheduler│ │ Memory   │ │ VFS      │ │ Drivers  │      │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │
└─────────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────────┐
│                      Hardware Abstraction                    │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │
│  │ Interrupt│ │ Timer    │ │ ACPI     │ │ PCI      │      │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │
└─────────────────────────────────────────────────────────────┘
                            │
┌─────────────────────────────────────────────────────────────┐
│                         Hardware Layer                        │
│  CPU │ RAM │ Disk │ Network │ Graphics │ Input Devices      │
└─────────────────────────────────────────────────────────────┘
```

### Boot Process

1. **BIOS/UEFI**: System firmware loads bootloader
2. **Limine Bootloader**: Loads kernel ELF file into memory
3. **Kernel Entry**: 
   - Setup stack and segment registers
   - Initialize CPU (GDT, IDT)
   - Enable paging
4. **Kernel Initialization**:
   - Memory manager setup
   - Driver initialization
   - File system mount
   - Window manager start
5. **User Space**: First task starts, GUI becomes available

### Memory Layout

```
0xFFFFFFFFFFFFFFFF ────────────────────────────────────────
│                    Higher Half Kernel                      │
│  (Kernel code, data, heap mapped at high addresses)      │
0xFFFF800000000000 ────────────────────────────────────────
│                    Reserved                               │
0x00007FFFFFFFFFFF ────────────────────────────────────────
│                    User Space Stack                       │
│                    (Per-task stacks)                       │
0x0000800000000000 ────────────────────────────────────────
│                    User Space Heap                        │
│                    (Per-task heaps)                        │
0x0000400000000000 ────────────────────────────────────────
│                    User Space Code/Data                    │
│                    (Per-task memory regions)               │
0x0000000000100000 ────────────────────────────────────────
│                    Lower Half Reserved                     │
0x0000000000000000 ────────────────────────────────────────
```

---

## Installation

### Using QEMU (Recommended for Testing)

#### Prerequisites

- QEMU 4.0 or later
- GNU toolchain (gcc, nasm, ld)
- xorriso (for ISO creation)

#### Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/DevilCore-OS.git
cd DevilCore-OS

# Build the OS
make

# Run in QEMU
make run
```

#### QEMU Options

```bash
# Default configuration (1024x768, 512MB RAM)
qemu-system-x86_64 -cdrom dist/DevilCore.iso -m 512

# With more RAM and higher resolution
qemu-system-x86_64 -cdrom dist/DevilCore.iso -m 2048 -vga virtio

# With serial output for debugging
qemu-system-x86_64 -cdrom dist/DevilCore.iso -m 512 -serial stdio

# With KVM acceleration (Linux only)
qemu-system-x86_64 -cdrom dist/DevilCore.iso -m 512 -enable-kvm
```

### Using VirtualBox

1. Create a new VM:
   - Type: Other/Unknown (64-bit)
   - Memory: 512 MB or more
   - Storage: Empty IDE controller

2. Mount the ISO:
   - Settings > Storage > Controller: IDE
   - Add optical drive > Choose disk file > Select `dist/DevilCore.iso`

3. Start the VM

### Using Real Hardware

**Warning**: Installing on real hardware carries risks. Always backup your data first.

1. Create a bootable USB:
   ```bash
   # On Linux
   sudo dd if=dist/DevilCore.iso of=/dev/sdX bs=4M status=progress
   sync
   ```

2. Boot from USB:
   - Enter BIOS/UEFI setup
   - Set USB as first boot device
   - Save and reboot

3. DevilCore OS will boot directly from the USB

---

## Building from Source

### Prerequisites

- **GCC**: x86_64-elf cross-compiler or system GCC with 64-bit support
- **NASM**: Netwide Assembler 2.14+
- **LD**: GNU linker
- **Xorriso**: For ISO image creation
- **Limine**: Bootloader (included in third_party/)

### Setting Up Build Environment

#### On Linux (Debian/Ubuntu)

```bash
# Install build dependencies
sudo apt-get update
sudo apt-get install build-essential nasm xorriso git

# Clone the repository
git clone https://github.com/yourusername/DevilCore-OS.git
cd DevilCore-OS
```

#### On macOS

```bash
# Install dependencies via Homebrew
brew install nasm xorriso gcc

# Clone the repository
git clone https://github.com/yourusername/DevilCore-OS.git
cd DevilCore-OS
```

#### On Windows (WSL)

```bash
# Install WSL and Ubuntu
wsl --install

# Inside WSL, follow Linux instructions
```

### Build Process

The build process is automated via Makefile:

```bash
# Clean previous builds
make clean

# Build kernel and create ISO
make

# Build only kernel (no ISO)
make kernel

# Create ISO from existing build
make iso

# Run in QEMU after building
make run

# Clean all build artifacts
make clean
```

### Build Stages

1. **Assembly Files**: NASM assembles boot.asm and interrupt.asm
2. **C Files**: GCC compiles all kernel C files with freestanding flags
3. **Linking**: LD links all object files using linker.ld
4. **ISO Creation**: xorriso creates bootable ISO with Limine bootloader

### Custom Build Options

Edit `Makefile` to customize:

```makefile
# Compiler flags
CFLAGS = -std=c17 -O2 -Wall -Wextra -ffreestanding

# Debug build (add -g and -O0)
CFLAGS += -g -O0

# Release build (add -O3 and remove debug symbols)
CFLAGS += -O3 -s
```

---

## Quick Start Guide

### First Boot

1. **Boot the OS**: Using QEMU, VirtualBox, or real hardware
2. **Wait for Initialization**: Serial output shows boot progress
3. **Desktop Appears**: GUI loads with desktop icons and taskbar
4. **Terminal Opens**: A terminal window opens automatically

### Basic Navigation

- **Mouse**: Move cursor to interact with windows and icons
- **Click**: Single-click to select, double-click to open
- **Drag**: Click and hold to move windows
- **Taskbar**: Shows running applications and start button
- **Start Menu**: Click start button or press Win key

### Opening Applications

1. **Desktop Icons**: Double-click any desktop icon
2. **Start Menu**: 
   - Press Win key or click start button
   - Navigate categories: Productivity, System, Power
   - Click application to launch
3. **Keyboard Shortcuts**: 
   - Win+T: Terminal
   - Win+E: File Manager
   - Win+N: Text Editor
   - Win+C: Calculator
   - Win+B: Browser
   - Win+I: Settings

### Using the Terminal

The terminal provides a command-line interface:

```bash
# List files
ls

# Change directory
cd /path/to/directory

# Display help
help

# View system information
sysinfo

# Reboot system
reboot
```

---

## User Guide

### Desktop Environment

#### Desktop Icons

Desktop icons provide quick access to applications:

- **Terminal**: Command-line interface
- **File Mgr**: Graphical file browser
- **Text Ed**: Text editor application
- **Calc**: Calculator
- **Browser**: Privacy-focused web browser
- **Settings**: System configuration
- **System**: System monitor
- **Notes**: Notes application (placeholder)
- **Music**: Music player (placeholder)
- **Pictures**: Image viewer (placeholder)
- **Videos**: Video player (placeholder)
- **Downloads**: Downloads folder
- **Documents**: Documents folder
- **Disk**: Disk management
- **Network**: Network settings

#### Taskbar

The taskbar at the bottom shows:

- **Start Button**: Opens start menu (left side)
- **Running Applications**: Shows open windows
- **Clock**: Displays current time (right side)
- **System Tray**: Shows system status (right side)

#### Start Menu

The start menu is organized into categories:

**Productivity:**
- Terminal (Win+T)
- File Manager (Win+E)
- Text Editor (Win+N)
- Calculator (Win+C)
- Notes
- Music
- Pictures
- Videos

**System:**
- Privacy Browser (Win+B)
- Settings (Win+I)
- System Monitor
- About DevilCore
- Disk Manager
- Network
- Logs
- Tasks

**Power:**
- Lock Screen (Win+L)
- Show Desktop (Win+D)
- Sleep
- Shutdown
- Restart

#### Window Management

**Window Controls:**
- **Close Button (×)**: Closes the window
- **Minimize Button (_)**: Hides window (shows in taskbar)
- **Maximize Button (□)**: Expands to full screen

**Window Operations:**
- **Move**: Click and drag title bar
- **Resize**: Click and drag window edges
- **Focus**: Click window to bring to front
- **Close**: Click × button or press Alt+F4

### Applications

#### Privacy Browser

The Privacy Browser provides a modern web browsing experience:

**Features:**
- Tabbed browsing (up to 8 tabs)
- Address bar with security indicators
- Navigation toolbar (back, forward, home, reload)
- Loading animations
- Status bar with connection info
- Privacy mode enabled by default
- Ad blocking support

**Keyboard Shortcuts:**
- Ctrl+L: Focus address bar
- Ctrl+T: New tab
- Ctrl+W: Close tab
- Ctrl+R: Reload page
- W/S: Scroll up/down

**Using the Browser:**
1. Open browser from start menu or desktop
2. Click address bar to enter URL
3. Use navigation buttons to move between pages
4. Click tabs to switch between open pages
5. Click × on tab to close it

#### Terminal

The terminal provides a command-line interface:

**Available Commands:**
- `help`: Display command list
- `ls`: List directory contents
- `cd`: Change directory
- `cat`: Display file contents
- `sysinfo`: Show system information
- `reboot`: Reboot system
- `clear`: Clear screen

**Command Examples:**
```bash
# List files in current directory
ls

# Change to root directory
cd /

# Display system information
sysinfo

# Reboot the system
reboot
```

#### File Manager

The File Manager provides graphical file browsing:

**Features:**
- Directory tree view
- File icons
- File metadata display
- Basic navigation

**Using File Manager:**
1. Open from start menu or desktop
2. Navigate directories by double-clicking
3. Use back button to go up
4. View file information in the side panel

#### Text Editor

The Text Editor provides basic text editing:

**Features:**
- Text input and editing
- File open/save operations
- Simple interface

**Using Text Editor:**
1. Open from start menu or desktop
2. Type or paste text
3. Use menu to save file

#### Calculator

The Calculator provides arithmetic operations:

**Features:**
- Basic operations: +, -, ×, ÷
- Clear button (C)
- Equals button (=)
- Modern button layout

**Using Calculator:**
1. Open from start menu or desktop
2. Click number buttons to input
3. Click operation buttons
4. Click = to calculate result

#### Settings

The Settings application allows system configuration:

**Features:**
- Display settings
- Theme customization
- System preferences

**Using Settings:**
1. Open from start menu or desktop
2. Navigate settings categories
3. Adjust options as needed

#### System Monitor

The System Monitor displays system statistics:

**Features:**
- CPU usage
- Memory usage
- Running processes
- System information

**Using System Monitor:**
1. Open from start menu or desktop
2. View real-time statistics
3. Monitor system performance

---

## Keyboard Shortcuts

### Global Shortcuts

| Shortcut | Action |
|----------|--------|
| Win | Open/Close Start Menu |
| Win+D | Show Desktop |
| Win+L | Lock Screen |
| Win+R | Run dialog |
| Alt+F4 | Close focused window |
| Alt+Tab | Switch between windows |

### Application Shortcuts

| Shortcut | Application | Action |
|----------|-------------|--------|
| Win+T | Terminal | Open Terminal |
| Win+E | File Manager | Open File Manager |
| Win+N | Text Editor | Open Text Editor |
| Win+C | Calculator | Open Calculator |
| Win+B | Browser | Open Browser |
| Win+I | Settings | Open Settings |

### Browser Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+L | Focus address bar |
| Ctrl+T | New tab |
| Ctrl+W | Close tab |
| Ctrl+R | Reload page |
| W | Scroll up |
| S | Scroll down |

### Terminal Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+C | Cancel current command |
| Ctrl+L | Clear screen |
| Arrow Up | Previous command in history |
| Arrow Down | Next command in history |

---

## Developer Guide

### Development Environment Setup

#### Required Tools

- **GCC**: x86_64-elf cross-compiler recommended
- **NASM**: Version 2.14 or later
- **GDB**: For debugging (optional)
- **QEMU**: For testing
- **Git**: Version control

#### Setting Up Cross-Compiler (Optional but Recommended)

```bash
# Download and build binutils
wget https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz
tar -xf binutils-2.40.tar.xz
mkdir build-binutils
cd build-binutils
../binutils-2.40/configure --target=x86_64-elf --prefix=/usr/local
make -j$(nproc)
sudo make install

# Download and build GCC
wget https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.xz
tar -xf gcc-12.2.0.tar.xz
cd gcc-12.2.0
./contrib/download_prerequisites
mkdir build-gcc
cd build-gcc
../gcc-12.2.0/configure --target=x86_64-elf --prefix=/usr/local \
    --enable-languages=c --disable-threads --disable-libssp \
    --disable-libstdc++-v3
make -j$(nproc) all-gcc all-target-libgcc
sudo make install-gcc install-target-libgcc
```

### Code Structure

#### Kernel Entry Point

The kernel entry point is in `src/kernel/boot.asm`:

```assembly
; Boot entry point
[bits 64]
[extern kernel_main]

call kernel_main
jmp $
```

#### Main Kernel Function

The main kernel function is in `src/kernel/main.c`:

```c
void kernel_main(void) {
    // Initialize serial for debugging
    serial_init();
    
    // Initialize GDT
    gdt_init();
    
    // Initialize IDT
    idt_init();
    
    // Initialize memory
    pmm_init();
    vmm_init();
    
    // Initialize drivers
    keyboard_init();
    mouse_init();
    
    // Initialize window manager
    wm_init();
    
    // Start scheduler
    scheduler_start();
}
```

### Adding New Features

#### Adding a New System Call

1. Define syscall number in `src/kernel/syscall.h`:
```c
#define SYS_MY_SYSCALL 42
```

2. Add syscall handler in `src/kernel/syscall.c`:
```c
int64_t syscall_my_function(uint64_t arg1, uint64_t arg2) {
    // Implementation
    return 0;
}
```

3. Register in syscall table:
```c
syscalls[SYS_MY_SYSCALL] = syscall_my_function;
```

#### Adding a New Device Driver

1. Create driver file: `src/kernel/mydriver.c`
2. Implement initialization:
```c
void mydriver_init(void) {
    // Detect device
    // Initialize hardware
    // Register interrupt handler
}
```

3. Call from `kernel_main()`:
```c
mydriver_init();
```

#### Adding a New Application

1. Create application file: `src/kernel/myapp.c`
2. Implement application:
```c
static struct wm_window *myapp_window = NULL;

void myapp_open(void) {
    myapp_window = wm_create_window("My App", 100, 100, 400, 300);
    // Setup window
}

void myapp_close(void) {
    wm_destroy_window(myapp_window);
}
```

3. Add to start menu in `src/kernel/wm.c`:
```c
const char *prod_apps[] = {..., "My App"};
const char *prod_icons[] = {..., "myapp_icon"};
```

4. Add desktop icon in `src/kernel/main.c`:
```c
wm_add_icon("My App", "myapp_icon", 40, 500, myapp_open);
```

### Debugging

#### Serial Debugging

The kernel outputs debug information via serial port:

```bash
# Run QEMU with serial output
qemu-system-x86_64 -cdrom dist/DevilCore.iso -serial stdio

# Or save to file
qemu-system-x86_64 -cdrom dist/DevilCore.iso -serial file:serial.log
```

#### GDB Debugging

Build with debug symbols:

```makefile
CFLAGS += -g
```

Run QEMU with GDB server:

```bash
qemu-system-x86_64 -cdrom dist/DevilCore.iso -s -S
```

Connect GDB:

```bash
gdb build/kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

### Testing

#### Unit Testing

Create test functions for critical components:

```c
void test_memory_allocation(void) {
    void *ptr = kmalloc(1024);
    assert(ptr != NULL);
    kfree(ptr);
}
```

#### Integration Testing

Test components together:

```c
void test_filesystem(void) {
    vfs_mount("/dev/sda1", "/");
    struct vfs_file *file = vfs_open("/test.txt", "w");
    vfs_write(file, "Hello", 5);
    vfs_close(file);
}
```

---

## Project Structure

```
DevilCore-OS/
├── src/
│   └── kernel/
│       ├── boot.asm          # Kernel entry point
│       ├── interrupt.asm     # Interrupt handlers
│       ├── main.c            # Main kernel function
│       ├── gdt.c             # Global Descriptor Table
│       ├── idt.c             # Interrupt Descriptor Table
│       ├── memory.c          # Memory management
│       ├── memory_compress.c # Memory compression
│       ├── scheduler.c       # Task scheduler
│       ├── syscall.c         # System call interface
│       ├── vfs.c             # Virtual File System
│       ├── fat.c             # FAT32 implementation
│       ├── framebuffer.c     # Graphics/framebuffer
│       ├── wm.c              # Window manager
│       ├── icons.c           # Icon system
│       ├── keyboard.c        # Keyboard driver
│       ├── mouse.c           # Mouse driver
│       ├── ata.c             # ATA disk driver
│       ├── rtl8139.c         # Network driver
│       ├── pci.c             # PCI enumeration
│       ├── acpi.c            # ACPI support
│       ├── shell.c           # Shell/terminal
│       ├── browser.c         # Browser application
│       ├── calculator.c      # Calculator application
│       ├── texteditor.c      # Text editor application
│       ├── filemanager.c     # File manager application
│       ├── settings.c        # Settings application
│       └── *.h               # Header files
├── third_party/
│   └── limine/               # Limine bootloader
├── build/                    # Build artifacts
├── dist/                     # Distribution files
│   └── DevilCore.iso         # Bootable ISO
├── docs/                     # Documentation
├── Makefile                  # Build configuration
├── linker.ld                # Linker script
├── limine.conf              # Bootloader config
├── logo.png                 # OS logo
├── convert_logo.py          # Logo conversion script
├── generate_all_icons.py   # Icon generation script
├── CHECKLIST.md            # Development checklist
├── plan.txt                # Development plan
└── README.md               # This file
```

---

## Kernel Components

### Global Descriptor Table (GDT)

The GDT defines memory segments for the x86_64 architecture:

```c
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
};
```

**GDT Entries:**
- Entry 0: Null descriptor
- Entry 1: Kernel code segment (64-bit)
- Entry 2: Kernel data segment
- Entry 3: User code segment (64-bit)
- Entry 4: User data segment
- Entry 5: TSS (Task State Segment)

### Interrupt Descriptor Table (IDT)

The IDT handles interrupts and exceptions:

```c
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
};
```

**Interrupt Types:**
- Exceptions (0-31): CPU faults (divide by zero, page fault, etc.)
- IRQs (32-47): Hardware interrupts (timer, keyboard, etc.)
- Syscalls (128): Software interrupts for system calls
- Custom (48-255): User-defined interrupts

### Memory Management

#### Physical Memory Manager (PMM)

Manages physical RAM pages:

```c
void pmm_init(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *addr);
uint64_t pmm_total_pages(void);
uint64_t pmm_free_pages(void);
```

**Features:**
- Bitmap-based page tracking
- 4KB page granularity
- Aligned allocations
- Free page counting

#### Virtual Memory Manager (VMM)

Manages virtual address space:

```c
void vmm_init(void);
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap_page(uint64_t virt);
```

**Features:**
- 4-level page tables (PML4, PDPT, PD, PT)
- Identity mapping for kernel
- User space mappings
- Page protection flags

#### Memory Compression

Compresses memory pages to reduce usage:

```c
void memory_compression_init(void);
void memory_compression_compress(void *addr);
void memory_compression_decompress(void *addr);
void memory_compression_stats(uint64_t *saved, uint32_t *ratio, uint32_t *blocks);
```

**Algorithm:**
- LZ77-based compression
- Dictionary encoding
- Adaptive compression level

### Scheduler

Preemptive round-robin scheduler:

```c
void scheduler_init(void);
void scheduler_start(void);
void task_create(const char *name, void (*entry)(void), uint32_t flags);
void scheduler_yield(void);
```

**Scheduling Algorithm:**
- Round-robin with time slices
- Task priorities (kernel tasks higher)
- Preemptive multitasking
- Context switching via task gates

**Task States:**
- READY: Waiting to run
- RUNNING: Currently executing
- BLOCKED: Waiting for I/O
- TERMINATED: Finished execution

### System Calls

Interface for user-kernel communication:

```c
int64_t syscall_read(uint64_t fd, uint64_t buf, uint64_t count);
int64_t syscall_write(uint64_t fd, uint64_t buf, uint64_t count);
int64_t syscall_open(uint64_t path, uint64_t flags);
int64_t syscall_close(uint64_t fd);
```

**Syscall Numbers:**
- 0: read
- 1: write
- 2: open
- 3: close
- 4: stat
- 5: mmap
- 6: munmap

---

## File System

### Virtual File System (VFS)

Abstraction layer for file operations:

```c
struct vfs_file {
    char path[256];
    uint64_t size;
    uint64_t offset;
    void *private_data;
};

struct vfs_file *vfs_open(const char *path, const char *mode);
int64_t vfs_read(struct vfs_file *file, void *buf, uint64_t count);
int64_t vfs_write(struct vfs_file *file, const void *buf, uint64_t count);
int vfs_close(struct vfs_file *file);
```

**VFS Operations:**
- Mount/unmount file systems
- Open/close files
- Read/write operations
- Directory traversal
- File metadata

### FAT32 Implementation

Read/write support for FAT32 partitions:

```c
struct fat_volume {
    uint8_t *boot_sector;
    struct fat_bpb *bpb;
    uint32_t fat_start;
    uint32_t data_start;
    uint32_t root_cluster;
};

int fat_init(struct ata_device *dev);
int fat_read(struct fat_volume *vol, const char *path, void *buf, uint64_t size);
int fat_write(struct fat_volume *vol, const char *path, const void *buf, uint64_t size);
```

**FAT32 Features:**
- Long filename support (LFN)
- Directory traversal
- File allocation table parsing
- Cluster chain following

---

## Window Manager

### Architecture

The window manager (WM) handles all GUI operations:

```c
struct wm_desktop {
    struct wm_widget widget;
    struct wm_taskbar taskbar;
    struct wm_start_menu start_menu;
    struct wm_context_menu context_menu;
    struct wm_icon icons[WM_MAX_ICONS];
    uint32_t icon_count;
    struct wm_window *windows[WM_MAX_WINDOWS];
    uint32_t window_count;
    struct wm_window *focused_window;
    int32_t mouse_x, mouse_y;
    uint8_t mouse_buttons;
};
```

### Widget System

Base class for all GUI elements:

```c
enum wm_widget_type {
    WM_WIDGET_GENERIC,
    WM_WIDGET_WINDOW,
    WM_WIDGET_TASKBAR,
    WM_WIDGET_MENU,
    WM_WIDGET_ICON
};

struct wm_widget {
    enum wm_widget_type type;
    char name[32];
    int32_t x, y;
    uint32_t width, height;
    uint8_t visible;
    void (*draw)(struct wm_widget *widget);
    void (*on_event)(struct wm_widget *widget, struct wm_event *event);
    struct wm_widget *parent;
    struct wm_widget *children[WM_MAX_CHILDREN];
    uint32_t child_count;
};
```

### Event System

Input events are queued and processed:

```c
enum wm_event_type {
    WM_EVENT_MOUSE_PRESS,
    WM_EVENT_MOUSE_RELEASE,
    WM_EVENT_MOUSE_MOVE,
    WM_EVENT_KEY,
    WM_EVENT_RESIZE
};

struct wm_event {
    enum wm_event_type type;
    int32_t x, y;
    uint8_t buttons;
    char key;
    uint8_t modifiers;
};
```

### Window Management

Window operations:

```c
struct wm_window *wm_create_window(const char *title, int32_t x, int32_t y, uint32_t w, uint32_t h);
void wm_destroy_window(struct wm_window *win);
void wm_focus_window(struct wm_window *win);
void wm_move_window(struct wm_window *win, int32_t x, int32_t y);
void wm_resize_window(struct wm_window *win, uint32_t w, uint32_t h);
```

**Window Properties:**
- Title bar with close/minimize/maximize
- Client area for content
- Border and shadow
- Focus state
- Z-order (stacking order)

---

## Graphics System

### Framebuffer

Direct framebuffer access for graphics:

```c
struct framebuffer {
    void *address;
    void *backbuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint16_t bpp;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};
```

**Framebuffer Operations:**
- Pixel plotting
- Rectangle filling
- Line drawing
- String rendering
- Bitmap drawing
- Alpha blending

### Drawing Primitives

Basic graphics functions:

```c
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void fb_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg);
void fb_draw_bitmap(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t *bitmap);
```

### Double Buffering

Flicker-free rendering:

```c
void fb_flip(void);
```

**Process:**
1. Draw to backbuffer
2. Call fb_flip() to swap buffers
3. Front buffer displays updated frame

### Icon System

119+ built-in icons:

```c
struct icon_data {
    const char *name;
    uint32_t width;
    uint32_t height;
    const uint32_t *pixels;
};

const struct icon_data *icon_get(const char *name);
void draw_icon(uint32_t x, uint32_t y, const struct icon_data *icon);
```

**Icon Categories:**
- Applications (browser, calculator, terminal, etc.)
- System (settings, network, disk, etc.)
- Folders (folder, documents, downloads, etc.)
- Media (music, pictures, videos, etc.)
- Power (power, restart, sleep, etc.)

---

## Device Drivers

### Keyboard Driver

PS/2 keyboard driver:

```c
void keyboard_init(void);
char keyboard_read_scancode(void);
void keyboard_handler(void);
```

**Features:**
- Scancode translation
- Modifier key tracking (Shift, Ctrl, Alt, Win)
- Key repeat handling
- Event queue

### Mouse Driver

PS/2 mouse driver:

```c
void mouse_init(void);
void mouse_handler(void);
int32_t mouse_get_x(void);
int32_t mouse_get_y(void);
uint8_t mouse_get_buttons(void);
```

**Features:**
- Movement tracking
- Button state (left, right, middle)
- Scroll wheel (basic support)
- Cursor rendering

### ATA Driver

IDE/SATA disk controller driver:

```c
void ata_init(void);
int ata_read(struct ata_device *dev, uint64_t lba, void *buf, uint32_t sectors);
int ata_write(struct ata_device *dev, uint64_t lba, const void *buf, uint32_t sectors);
```

**Features:**
- PIO mode data transfer
- LBA addressing
- Primary/secondary channels
- Master/slave drives

### RTL8139 Driver

Realtek Ethernet controller driver:

```c
void rtl8139_init(void);
void rtl8139_send_packet(const void *data, uint32_t len);
void rtl8139_receive_packet(void *buf, uint32_t *len);
```

**Features:**
- Ethernet frame transmission
- Ethernet frame reception
- MAC address configuration
- Interrupt handling

### PCI Driver

PCI bus enumeration:

```c
void pci_init(void);
void pci_enumerate_devices(void (*callback)(uint32_t bus, uint32_t slot, uint32_t func));
uint32_t pci_read_config(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);
void pci_write_config(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t value);
```

**Features:**
- Device detection
- Configuration space access
- IRQ routing
- BAR (Base Address Register) parsing

---

## Network Stack

### Ethernet Layer

Handles Ethernet frames:

```c
struct eth_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
};

void eth_send_frame(const uint8_t *dest_mac, uint16_t ethertype, const void *data, uint32_t len);
void eth_receive_frame(void *data, uint32_t *len);
```

### ARP Protocol

Address Resolution Protocol:

```c
struct arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_size;
    uint8_t proto_size;
    uint16_t opcode;
    uint8_t src_mac[6];
    uint32_t src_ip;
    uint8_t dest_mac[6];
    uint32_t dest_ip;
};

void arp_send_request(uint32_t target_ip);
void arp_handle_packet(const struct arp_packet *packet);
```

### TCP/IP (In Development)

Basic TCP/IP stack planned:

- IP packet routing
- TCP connection management
- UDP datagram support
- Socket API

---

## Security Features

### Memory Protection

Page-level protection between processes:

- Kernel pages marked as supervisor-only
- User pages marked as user-accessible
- Write protection for code pages
- No-execute (NX) bit support

### Privilege Levels

x86_64 privilege levels (rings):

- Ring 0: Kernel mode (full access)
- Ring 3: User mode (restricted access)
- Rings 1-2: Reserved (not used)

### Memory Compression

Reduces memory footprint and prevents leaks:

- Compresses idle memory pages
- Decompresses on access
- Reduces memory usage by ~30-50%
- Prevents memory exhaustion

### Private Browsing

Browser runs in isolated mode:

- No persistent cookies
- No browsing history
- No cache storage
- Ad blocking enabled

---

## Troubleshooting

### Common Issues

#### OS Won't Boot

**Symptoms:** System hangs during boot, no GUI appears

**Solutions:**
1. Check serial output for error messages
2. Ensure sufficient RAM (512MB minimum)
3. Try lower resolution in QEMU: `-vga std`
4. Verify ISO integrity

#### Graphics Issues

**Symptoms:** Corrupted display, wrong resolution

**Solutions:**
1. Try different QEMU VGA options: `-vga virtio`, `-vga qxl`
2. Reduce resolution in bootloader config
3. Check framebuffer initialization in logs

#### Mouse/Keyboard Not Working

**Symptoms:** No input response

**Solutions:**
1. Ensure PS/2 controller is enabled in BIOS
2. Check interrupt handler registration
3. Verify device initialization order

#### File System Errors

**Symptoms:** Cannot read/write files

**Solutions:**
1. Check ATA driver initialization
2. Verify disk is detected
3. Check FAT32 partition integrity

#### Build Errors

**Symptoms:** Compilation fails

**Solutions:**
1. Ensure all dependencies are installed
2. Check GCC version (7.0+ required)
3. Verify NASM installation
4. Clean build: `make clean && make`

### Debug Mode

Enable debug output:

```c
// In main.c
#define DEBUG 1

#ifdef DEBUG
    serial_write_string("Debug: Entering function\n");
#endif
```

### Serial Output

View kernel messages:

```bash
# Run QEMU with serial output
qemu-system-x86_64 -cdrom dist/DevilCore.iso -serial stdio

# Or save to file
qemu-system-x86_64 -cdrom dist/DevilCore.iso -serial file:debug.log
```

---

## Contributing

### Contribution Guidelines

We welcome contributions! Please follow these guidelines:

1. **Code Style**: Follow existing code style (C17, 4-space indentation)
2. **Comments**: Document functions and complex logic
3. **Testing**: Test changes thoroughly before submitting
4. **Commits**: Write clear, descriptive commit messages
5. **Pull Requests**: Explain changes in PR description

### Areas for Contribution

**High Priority:**
- Network stack completion (TCP/IP)
- File system improvements (ext4 support)
- More applications (image viewer, music player)
- Driver improvements (AHCI, USB)

**Medium Priority:**
- Sound system (audio driver, mixer)
- Print support (printer driver)
- Bluetooth support
- WiFi support

**Low Priority:**
- Theme system
- Desktop widgets
- Screen saver
- Package manager

### Submitting Changes

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make changes and test
4. Commit changes: `git commit -am "Add feature"`
5. Push to branch: `git push origin feature-name`
6. Create pull request

### Reporting Bugs

Report bugs via GitHub Issues with:

- OS version/build
- Hardware configuration
- Steps to reproduce
- Expected behavior
- Actual behavior
- Serial output (if applicable)

---

## Roadmap

### Version 0.2 (Current)

- [x] Basic GUI with window manager
- [x] Desktop icons and taskbar
- [x] Start menu
- [x] Basic applications (terminal, calculator, browser)
- [x] File system (FAT32)
- [x] Memory management with compression
- [x] Device drivers (keyboard, mouse, disk)

### Version 0.3 (Next)

- [ ] Complete TCP/IP network stack
- [ ] Socket API
- [ ] Web server
- [ ] SSH client
- [ ] Improved file manager
- [ ] Text editor improvements
- [ ] System monitor enhancements

### Version 0.4

- [ ] ext4 file system support
- [ ] AHCI disk driver
- [ ] USB driver stack
- [ ] Sound system (audio driver)
- [ ] Media player (audio/video)
- [ ] Image viewer
- [ ] Theme system

### Version 0.5

- [ ] Multi-user support
- [ ] Login system
- [ ] Permissions system
- [ ] Package manager
- [ ] Software repository
- [ ] Auto-updater
- [ ] Bluetooth support
- [ ] WiFi support

### Version 1.0 (Future)

- [ ] POSIX compatibility layer
- [ ] Linux binary compatibility
- [ ] Virtualization support
- [ ] Container system
- [ ] Cloud integration
- [ ] Mobile support

---

## License

DevilCore OS is licensed under the MIT License:

```
MIT License

Copyright (c) 2024 DevilCore OS Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Acknowledgments

DevilCore OS wouldn't be possible without the following:

### Projects and Resources

- **Limine**: Modern bootloader for x86_64 systems
- **OSDev Wiki**: Comprehensive OS development documentation
- **Writing an OS in Rust**: Phil Opp's excellent OS development tutorial
- **OSDev.org**: Community for OS developers
- **FreeRTOS**: Inspiration for scheduler design

### Libraries and Tools

- **GCC**: GNU Compiler Collection
- **NASM**: Netwide Assembler
- **QEMU**: Quick Emulator for testing
- **Xorriso**: ISO creation tool
- **Limine Bootloader**: Bootloader implementation

### Contributors

- All contributors who have helped with code, testing, and documentation

### Special Thanks

- The OS development community for invaluable resources
- Open source projects that inspired this work
- Everyone who has tested and provided feedback

---

## Contact

- **GitHub**: https://github.com/yourusername/DevilCore-OS
- **Issues**: https://github.com/yourusername/DevilCore-OS/issues
- **Discussions**: https://github.com/yourusername/DevilCore-OS/discussions

---

## References

### Learning Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Writing an OS in Rust](https://os.phil-opp.com/)
- [Intel 64 and IA-32 Architectures Software Developer's Manual](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- [AMD64 Architecture Programmer's Manual](https://www.amd.com/system/files/TechDocs/24593.pdf)

### Standards

- [ELF Specification](https://refspecs.linuxfoundation.org/elf/elf.pdf)
- [FAT32 Specification](https://academy.cba.mit.edu/classes/863.16/people/hannah.jordan/files/FAT32.pdf)
- [PCI Specification](https://pcisig.com/specifications)
- [ACPI Specification](https://uefi.org/specifications)

### Related Projects

- [ToaruOS](https://github.com/klange/toaruos)
- [HelenOS](https://www.helenos.org/)
- [Redox OS](https://www.redox-os.org/)
- [HaydenOS](https://github.com/haydenr/HaydenOS)

---

**Last Updated**: 2024
**Version**: 0.2
**Status**: Active Development
