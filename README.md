# DevilCore OS (v0.4 "Advanced Architecture")

A modern, high-performance 64-bit operating system with an advanced compositing GUI, fair-share scheduling, and a robust object-oriented widget toolkit. DevilCore OS provides a complete desktop environment with a sophisticated window management system and a modular kernel architecture designed for both performance and educational clarity.

![DevilCore OS](https://img.shields.io/badge/DevilCore-OS-blue) ![License](https://img.shields.io/badge/license-MIT-green) ![Arch](https://img.shields.io/badge/arch-x86__64--64-lightgrey) ![Status](https://img.shields.io/badge/status-v0.4--Advanced-orange) ![Build](https://img.shields.io/badge/build-passing-brightgreen)

---

## Table of Contents

1.  [Overview](#1-overview)
2.  [Key Architectural Pillars](#2-key-architectural-pillars)
    - [The DevilComp Compositor](#the-devilcomp-compositor)
    - [Completely Fair Scheduler (CFS)](#completely-fair-scheduler-cfs)
    - [DevilUI Toolkit](#devilui-toolkit)
    - [Slab Allocator](#slab-allocator)
    - [Memory Compression](#memory-compression)
3.  [Kernel Subsystems](#3-kernel-subsystems)
    - [Memory Management (PMM/VMM)](#memory-management-pmmvmm)
    - [Virtual File System (VFS)](#virtual-file-system-vfs)
    - [Interrupt & Exception Handling](#interrupt--exception-handling)
    - [System Calls](#system-calls)
4.  [DevilUI Framework](#4-devilui-framework)
    - [Painter API](#painter-api)
    - [Widget Hierarchy](#widget-hierarchy)
    - [Animation Engine](#animation-engine)
    - [Layout Management](#layout-management)
5.  [Built-in Applications](#5-built-in-applications)
    - [Privacy Browser](#privacy-browser)
    - [Terminal (DevilShell)](#terminal-devilshell)
    - [File Manager](#file-manager)
    - [Settings (System Configuration)](#settings-system-configuration)
    - [Calculator](#calculator)
    - [System Monitor](#system-monitor)
6.  [Hardware Support & Drivers](#6-hardware-support--drivers)
    - [Disk (ATA/IDE)](#disk-ataide)
    - [Network (RTL8139)](#network-rtl8139)
    - [Input (PS/2)](#input-ps2)
    - [Bus (PCI/ACPI)](#bus-pciacpi)
7.  [Technical Reference](#7-technical-reference)
    - [File-by-File Breakdown](#file-by-file-breakdown)
    - [System Call Table](#system-call-table)
    - [Widget API Reference](#widget-api-reference)
    - [Icon Library (All 119 Icons)](#icon-library-all-119-icons)
8.  [Developer Guide](#8-developer-guide)
    - [Build System](#build-system)
    - [Adding New Syscalls](#adding-new-syscalls)
    - [Creating DevilUI Apps](#creating-devilui-apps)
    - [Debugging Techniques](#debugging-techniques)
9.  [Keyboard Shortcuts](#9-keyboard-shortcuts)
10. [Project Structure](#10-project-structure)
11. [Troubleshooting & FAQ](#11-troubleshooting--faq)
12. [Roadmap](#12-roadmap)
13. [License & Acknowledgments](#13-license--acknowledgments)

---

## 1. Overview

DevilCore OS is a from-scratch operating system developed for the x86_64 architecture. It transitions away from monolithic, "flat-framebuffer" designs toward a modern, **compositing-based** windowing system similar to macOS or modern Wayland-based Linux desktops.

### Core Philosophy
- **Compositing by Default**: Every window is its own surface, allowing for alpha transparency, shadows, and smooth animations without tearing.
- **Perfect Fairness**: A Red-Black Tree based CFS ensures that background tasks never starve the UI.
- **Minimalism**: The kernel is optimized for size (< 400KB) while maintaining a rich feature set.
- **Safety**: Memory compression and slab allocation prevent leaks and maximize the utility of available RAM.

---

## 2. Key Architectural Pillars

### The DevilComp Compositor
Unlike legacy window managers that draw directly to the screen, DevilComp maintains a hierarchical tree of **Surfaces**.

- **Double Buffering per Surface**: Each window has its own back-buffer. When a window draws, it modifies its own buffer. The compositor then blends these buffers into the final screen.
- **Damage Tracking**: The compositor only re-composites regions of the screen that have changed (e.g., the area under a moving cursor or a blinking text caret).
- **Z-Order Management**: Surfaces are sorted in a linked list. Translucent surfaces (like window shadows) are blended using high-performance ARGB alpha-blending.
- **Wallpaper Tiling**: Optimized background rendering with support for scaling and centering.

### Completely Fair Scheduler (CFS)
DevilCore implements a modern scheduling algorithm inspired by the Linux kernel's CFS.

- **Virtual Runtime (vruntime)**: Each task tracks how much CPU time it has consumed, normalized by its priority. The scheduler always picks the task with the smallest `vruntime`.
- **Red-Black Tree Data Structure**: Runnable tasks are stored in a self-balancing RB-tree, allowing for $O(\log n)$ insertion and selection.
- **Priority Weights**: "Nice" values from -20 to 19 map to geometric weights, ensuring that a high-priority task gets a mathematically fair larger share of the CPU.

### DevilUI Toolkit
DevilUI is a C-based, object-oriented GUI framework that uses **vtable polymorphism** to provide a rich set of widgets.

- **Painter API**: Supports anti-aliased rounded rectangles, transformation stacks (translate/scale), and clipping regions.
- **Theme Engine**: Centralized theme management for consistent colors, font sizes, and radii across all applications.
- **Event Loop**: Decoupled event processing allows for responsive UIs that don't block on kernel I/O.

### Slab Allocator
The kernel uses a high-speed slab allocator for common fixed-size structures.

- **Cache Optimization**: Objects are kept in caches (64, 128, 256, 512, 1024, 2048 bytes) to reduce fragmentation.
- **Zero-Latency Allocation**: Free objects are maintained in a LIFO stack for $O(1)$ allocation and deallocation.

### Memory Compression
DevilCore includes a built-in memory compression engine.

- **LZ77-based Algorithm**: Compresses idle pages in memory to effectively double the available RAM for light workloads.
- **Transparent Decompression**: Pages are decompressed on-the-fly when accessed by the CPU.

---

## 3. Kernel Subsystems

### Memory Management (PMM/VMM)
- **PMM (Physical Memory Manager)**: A bitmap-based allocator managing 4KB pages. It handles the initial memory map provided by the bootloader (Limine).
- **VMM (Virtual Memory Manager)**: Implements 4-level paging (PML4). It provides isolation between the kernel and user space, and between different processes.
- **Heap**: A hybrid allocator using Slab for small objects and a page-based allocator for large segments.

### Virtual File System (VFS)
The VFS provides a unified interface for all file-like objects.
- **FAT32 Support**: Full read/write support for standard FAT32 partitions.
- **Path Resolution**: Handles absolute and relative paths, including `.` and `..` symlink-like behavior.
- **Mount Points**: Allows mounting different hardware or pseudo-filesystems into the root tree.

### Interrupt & Exception Handling
- **IDT**: 256 entries handling hardware IRQs, software interrupts, and CPU exceptions.
- **ISR**: Optimized assembly wrappers for saving and restoring CPU state during context switches.
- **PIC/APIC**: Support for legacy 8259 PIC and modern APIC for multi-core scalability.

### System Calls
DevilCore uses a standard syscall interface (via `int 0x80` or `syscall` instruction).
- **Process Control**: `fork`, `exec`, `exit`, `wait`.
- **File I/O**: `open`, `read`, `write`, `close`, `lseek`.
- **Memory**: `mmap`, `brk`, `munmap`.
- **IPC**: `msg_send`, `msg_recv`, `sync_wait`.

---

## 4. DevilUI Framework

### Painter API
The Painter is the core drawing engine used by all widgets.
```c
void painter_fill_round_rect(painter_t *p, rect_t *r, uint32_t radius);
void painter_draw_text(painter_t *p, int32_t x, int32_t y, const char *text);
void painter_save(painter_t *p);    // Save transform/clip state
void painter_restore(painter_t *p); // Restore state
```

### Widget Hierarchy
- **Widget**: Base class with properties for `x`, `y`, `width`, `height`, and `visibility`.
- **Window**: A top-level container with title bar, shadows, and macOS-style traffic light buttons.
- **Button**: Interactive element with hover and click states.
- **Label**: Multi-line text display with automatic wrapping.
- **Slider/Progress**: Visual indicators for values and progress.
- **ListView**: Optimized scrolling list for large datasets.

### Animation Engine
DevilCore includes a fixed-point animation engine for smooth UI transitions.
- **Curves**: Linear, Quadratic (Ease-In/Out), Bounce, and Elastic.
- **Non-blocking**: Animations run in the background without halting the main application logic.

### Layout Management
- **VBox/HBox**: Automatic vertical and horizontal stacking.
- **Grid**: Table-like layout with spanning support.
- **Anchor**: Responsive layout that scales with window resizing.

---

## 5. Built-in Applications

### Privacy Browser
A modern, tabbed web browser designed for the DevilCore environment.
- **Tabbed Interface**: Switch between multiple pages seamlessly.
- **Navigation**: Back, Forward, Reload, and Home buttons.
- **Address Bar**: Integrated search and URL entry with security indicators.
- **Ad-Blocker**: Built-in kernel-level filtering for common ad domains.

### Terminal (DevilShell)
A powerful CLI with support for advanced shell features.
- **History**: Use Up/Down arrows to navigate previous commands.
- **Piping**: Redirect output between commands (basic support).
- **Environment**: Support for `PATH` and custom environment variables.

### File Manager
A graphical explorer for the FAT32 filesystem.
- **Breadcrumbs**: Easy navigation through directory hierarchies.
- **Previews**: Thumbnails for supported image types.
- **Operations**: Copy, Paste, Delete, and Rename (integrated with VFS).

### Settings (System Configuration)
The central hub for all system settings, organized into tabs:
- **General**: Themes (Dark, Blue, Purple, etc.), Wallpaper, and Animations.
- **Display**: Resolution, Refresh Rate, and Font Scaling.
- **Network**: IP configuration, MAC cloning, and Traffic monitoring.
- **Security**: Firewall rules, Encryption toggles, and Permission logs.
- **System**: Live CPU/Memory graphs, Uptime, and Kernel version.

### Calculator
A sleek, responsive calculator for daily tasks.
- **History Tape**: View previous calculations.
- **Scientific Mode**: (Planned) Support for trig and log functions.

### System Monitor
A real-time dashboard for kernel performance.
- **Process List**: View vruntime, PID, and memory usage for all tasks.
- **Resource Graphs**: Animated graphs for CPU load and memory pressure.

---

## 6. Hardware Support & Drivers

### Disk (ATA/IDE)
- **LBA48 Support**: Handles disks larger than 128GB.
- **PIO/DMA**: Optimized data transfer modes.
- **Partitioning**: Support for MBR and GPT partition tables.

### Network (RTL8139)
- **Ethernet**: Full 10/100 Mbps support.
- **ARP/IP**: Basic packet routing and address resolution.
- **Buffer Management**: Ring-buffer based packet reception for zero packet loss.

### Input (PS/2)
- **Keyboard**: Full US-layout support with scancode translation.
- **Mouse**: 3-button support with scroll wheel integration.

### Bus (PCI/ACPI)
- **PCI Discovery**: Recursively scans all buses and functions.
- **ACPI**: Power management (Sleep/Shutdown) and hardware discovery via RSDP.

---

## 7. Technical Reference

### File-by-File Breakdown

| File | Description |
|------|-------------|
| `acpi.c/h` | Advanced Configuration and Power Interface implementation. |
| `animation.c` | Fixed-point animation engine with multiple easing curves. |
| `ata.c/h` | ATA/IDE disk controller driver with LBA48 support. |
| `boot.asm` | Initial kernel entry point and 64-bit long mode transition. |
| `browser.c` | The "Privacy Browser" application logic and UI. |
| `calculator.c/h` | GUI-based scientific calculator application. |
| `cfs_scheduler.c/h` | Linux-inspired Completely Fair Scheduler with Red-Black Tree. |
| `compositor.c/h` | Advanced window compositor with damage tracking and alpha blending. |
| `devilui.c/h` | Core object-oriented widget toolkit and event loop. |
| `event_loop.c` | Kernel-level event distribution and task wakeup system. |
| `fat.c/h` | FAT32 filesystem implementation with LFN support. |
| `filemanager.c/h` | Graphical file explorer application. |
| `font.c/h` | Anti-aliased PSF2 font rendering engine. |
| `framebuffer.c/h` | Low-level pixel plotting and screen initialization. |
| `gdt.c/h` | Global Descriptor Table setup for segment isolation. |
| `icons.c/h` | Icon management and drawing utility. |
| `idt.c/h` | Interrupt Descriptor Table and exception handlers. |
| `interrupt.asm` | Low-level interrupt entry/exit wrappers. |
| `ipc.c/h` | Message-passing and synchronization primitive system. |
| `isr.c/h` | High-level interrupt service routine handlers. |
| `keyboard.c/h` | PS/2 keyboard driver with US-QWERTY layout. |
| `kprintf.c/h` | Kernel debugging output (Serial/COM1). |
| `main.c/h` | Kernel initialization sequence and core loops. |
| `memory.c/h` | PMM, VMM, and hybrid heap management. |
| `memory_compress.c/h`| LZ77-based memory compression algorithm. |
| `mouse.c/h` | PS/2 mouse driver with scroll and button support. |
| `net.c/h` | Common networking abstractions (Ethernet/ARP). |
| `painter.c` | High-level drawing API (Rounded rects, Transforms). |
| `pci.c/h` | PCI bus enumeration and device discovery. |
| `pic.c/h` | Legacy 8259 Programmable Interrupt Controller. |
| `rtl8139.c/h` | Realtek 8139 Ethernet controller driver. |
| `runtime.c` | C runtime support (string, memory, math stubs). |
| `scheduler.c/h` | Base task management and context switching. |
| `settings.c/h` | Advanced system configuration application. |
| `shell.c/h` | Command-line interface and built-in shell. |
| `slab.c/h` | High-performance cache-based memory allocator. |
| `syscall.c/h` | System call interface and handler registration. |
| `texteditor.c/h` | Simple GUI-based text editing application. |
| `timer.c/h` | PIT (Programmable Interval Timer) management. |
| `vfs.c/h` | Virtual File System abstraction layer. |
| `wm.c/h` | Window manager, decorations, and focus logic. |

### System Call Table

| ID | Function | Arguments | Description |
|----|----------|-----------|-------------|
| 0 | `sys_read` | `fd, buf, count` | Read data from a file descriptor. |
| 1 | `sys_write` | `fd, buf, count` | Write data to a file descriptor. |
| 2 | `sys_open` | `path, flags` | Open a file and return a descriptor. |
| 3 | `sys_close` | `fd` | Close an open file descriptor. |
| 4 | `sys_lseek` | `fd, offset, whence`| Change the file offset. |
| 5 | `sys_fork` | `None` | Create a child process. |
| 6 | `sys_execve`| `path, argv, envp` | Execute a program file. |
| 7 | `sys_exit` | `status` | Terminate the current process. |
| 8 | `sys_wait` | `pid, status, options`| Wait for a process to terminate. |
| 9 | `sys_brk` | `addr` | Change the data segment size. |
| 10| `sys_mmap` | `addr, len, prot, etc`| Map files or devices into memory. |
| 11| `sys_munmap`| `addr, len` | Unmap memory regions. |
| 20| `sys_getpid`| `None` | Get the process ID. |
| 30| `sys_send` | `pid, msg, len` | Send a message to another process. |
| 31| `sys_recv` | `pid, msg, len` | Receive a message from a process. |
| 40| `sys_yield` | `None` | Yield CPU to the next task. |
| 41| `sys_sleep` | `ms` | Put the process to sleep for N ms. |

### Widget API Reference

DevilUI widgets are initialized using a consistent pattern:
`widget_t* type_create(widget_t* parent, ... parameters ...)`

- **Window**: `window_create(title, x, y, w, h)`
- **Button**: `button_create(parent, text, x, y, w, h)`
- **Label**: `label_create(parent, text, x, y, w, h)`
- **TextField**: `textfield_create(parent, x, y, w, h)`
- **Slider**: `slider_create(parent, x, y, w, h, min, max)`
- **Progress**: `progress_create(parent, x, y, w, h)`
- **ListView**: `listview_create(parent, x, y, w, h)`
- **TabView**: `tabview_create(parent, x, y, w, h)`

### Icon Library (All 119 Icons)

DevilCore features a comprehensive set of icons stored in the `src/kernel/` directory.

**General Icons:**
- `add`, `back`, `forward`, `up`, `more`, `search`, `home`, `menu`, `close`, `minimize`, `maximize`.

**System Icons:**
- `cpu`, `gpu`, `memory`, `disk`, `network`, `settings`, `logs`, `system`, `kernel`, `bios`, `bootloader`.
- `battery_full`, `battery_med`, `battery_low`, `battery_critical`, `battery_charging`.
- `ethernet_on`, `ethernet_off`, `wifi_high`, `wifi_med`, `wifi_low`, `wifi_none`, `bluetooth`.
- `lock`, `unlock`, `key`, `shield`, `firewall`, `encryption`.

**File & Storage Icons:**
- `folder`, `folder_open`, `folder_hidden`, `folder_shared`, `folder_network`.
- `document`, `notes`, `pictures`, `videos`, `music`, `media_player`, `image_viewer`.
- `root_fs`, `ext_drive`, `usb`, `cdrom`, `trash_empty`, `trash_full`.
- `copy`, `cut`, `paste`, `delete`, `edit`, `save`, `save_as`, `undo`, `redo`.

**Application Icons:**
- `browser`, `terminal`, `calculator`, `texteditor`, `filemanager`, `system_monitor`, `package_manager`.
- `camera`, `mic`, `bell`, `calendar`, `email`, `message`, `share`.

**Installation & Maintenance Icons:**
- `pkg_install`, `pkg_remove`, `pkg_update`, `update`, `refresh`, `sync`, `backup`, `recovery`, `restore`.
- `service_running`, `service_stopped`, `safemode`, `crash`, `error`, `warning`, `info`, `debug`.

**Power Icons:**
- `power`, `restart`, `sleep`, `shutdown`.

---

## 8. Developer Guide

### Build System
DevilCore uses a highly optimized Makefile.
- `make`: Builds the kernel and generates a bootable ISO.
- `make run`: Launches QEMU with recommended settings (512MB RAM, serial logging).
- `make clean`: Removes all build artifacts.

**Optimization Flags**:
- `-Os`: Optimize for binary size.
- `-ffunction-sections`: Allows the linker to garbage-collect unused code.
- `-flto`: Link-time optimization for cross-module inlining.

### Adding New Syscalls
1. Define the syscall ID in `src/kernel/syscall.h`.
2. Implement the handler in `src/kernel/syscall.c`.
3. Add the entry point to the `syscall_table`.

### Creating DevilUI Apps
Applications are typically implemented as a `struct app` with `init`, `event`, and `draw` callbacks.
```c
void my_app_init() {
    window_t *win = window_create("My App", 100, 100, 400, 300);
    button_t *btn = button_create(win, "Click Me", 10, 10, 100, 30);
    button_set_onclick(btn, my_callback);
}
```

### Debugging Techniques
- **Serial Output**: The kernel logs all `kprintf` calls to COM1. Use `-serial stdio` in QEMU.
- **GDB**: Run `make debug` to start a GDB server on port 1234.
- **Stack Traces**: Automatic stack-unwinding on kernel panics.

---

## 9. Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **Win** | Toggle Start Menu |
| **Win + D** | Show/Hide Desktop (Minimize All) |
| **Win + L** | Lock Screen |
| **Alt + Tab** | Switch Focus between Windows |
| **Alt + F4** | Close Active Window |
| **Win + T** | Open Terminal |
| **Win + E** | Open File Manager |
| **Win + B** | Open Privacy Browser |
| **Win + I** | Open Settings |
| **Win + C** | Open Calculator |
| **Ctrl + Alt + Del** | Force Reboot |
| **PrintScreen** | Save Screenshot to `/screenshots/` |

---

## 10. Project Structure

```text
DevilCore/
├── src/kernel/
│   ├── main.c           # Kernel entry and initialization
│   ├── compositor.c     # Window composition and damage tracking
│   ├── cfs_scheduler.c  # Fair-share task scheduling
│   ├── devilui.c        # Widget toolkit and theme engine
│   ├── painter.c        # Drawing primitives and transforms
│   ├── memory.c         # PMM, VMM, and Slab allocation
│   ├── memory_compress.c# LZ77 memory compression logic
│   ├── fat.c            # FAT32 filesystem driver
│   ├── vfs.c            # Virtual filesystem abstraction
│   ├── ata.c            # Disk controller driver
│   ├── rtl8139.c        # Network interface driver
│   ├── isr.c            # Interrupt service routines
│   ├── syscall.c        # System call implementations
│   └── *_icon.h         # ARGB icon data (119+ files)
├── iso_root/            # Root directory for the bootable ISO
│   ├── limine.conf      # Bootloader configuration
│   └── boot/            # Kernel binary and bootloader files
├── build/               # Compiled object files and binaries
├── docs/                # Extended documentation and whitepapers
├── Makefile             # Optimized build configuration
├── linker.ld            # Kernel memory layout
└── limine.conf          # Root-level bootloader config
```

---

## 11. Troubleshooting & FAQ

**Q: The system hangs at "Initialising PCI..."**
A: This usually indicates an incompatible virtual device. Ensure you are using QEMU with `-machine q35`.

**Q: Mouse movement is choppy.**
A: Check if `vsync` is enabled in Settings. Disabling it can reduce input latency on some hosts.

**Q: How do I install more apps?**
A: Currently, apps must be compiled into the kernel. User-space loading is planned for v0.5.

**Q: Does it support Wi-Fi?**
A: Not yet. Use the RTL8139 Ethernet emulation in QEMU.

---

## 12. Roadmap

### v0.4 (Current)
- [x] Advanced Compositing Engine
- [x] CFS Scheduler (RB-Tree)
- [x] DevilUI Widget Toolkit
- [x] Memory Compression

### v0.5 (Upcoming)
- [ ] ELF Executable Loading
- [ ] User-space Isolation (Ring 3)
- [ ] Shared Memory IPC
- [ ] SATA/AHCI Support

### v0.6
- [ ] Full TCP/IP Stack
- [ ] USB 2.0 (EHCI) Support
- [ ] Audio Driver (Intel HD Audio)

---

## 13. License & Acknowledgments

DevilCore OS is licensed under the **MIT License**.

**Special Thanks to:**
- The **OSDev Wiki** community for invaluable documentation.
- The **Limine** project for a modern, reliable bootloader.
- **Intel & AMD** for their extensive software developer manuals.

---
**Version**: 0.4.2-Advanced
**Last Updated**: May 2026
**Developer**: DarkDevil (and the DevilCore Community)
