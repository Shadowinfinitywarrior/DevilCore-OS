# DevilCore OS (v0.5 'Next Generation')

Modern 64-bit OS kernel with advanced compositing GUI, fair-share scheduling, and object-oriented widget toolkit.
Complete desktop environment for QEMU with DevilComp compositor, CFS scheduler, DevilUI toolkit, and modular kernel.

[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/DarkDevil404/DevilCore/actions)

---

## Table of Contents
1. [Overview](#1-overview)
2. [Key Features](#2-key-features)
3. [Architecture](#3-architecture)
4. [Getting Started](#4-getting-started)
5. [Building from Source](#5-building-from-source)
6. [Contributing](#6-contributing)
7. [Roadmap](#7-roadmap)
8. [License](#8-license)

---

## 1. Overview
DevilCore OS is an educational and research-oriented 64-bit operating system kernel written in C and x86-64 assembly. It demonstrates modern OS principles including preemptive multitasking, virtual memory management, compositing graphics, and hardware abstraction. The system is designed to run in QEMU and provides a fully functional, aesthetic desktop environment.

## 2. Key Features
- **64-bit Architecture**: Native x86-64 long mode, 4-level paging, and HHDM (Higher Half Direct Mapping) for robust kernel memory access.
- **CFS Scheduler**: A custom Completely Fair Scheduler (CFS) utilizing red-black trees to ensure O(log N) task selection and efficient CPU time distribution.
- **DevilComp Compositor**: Advanced per-window alpha blending, smooth window animations, and damage tracking for high-performance UI rendering.
- **DevilUI Toolkit**: A complete, hierarchical, object-oriented widget system featuring event propagation, layout managers, and custom skinning support.
- **Memory Management**: Comprehensive physical/virtual memory managers, a slab allocator for kernel objects, and a custom RLE memory compression algorithm.
- **Virtual File System (VFS)**: A powerful abstraction layer supporting FAT32 filesystems, device file nodes, and intuitive path resolution.
- **Networking**: Full TCP/IP stack with a dedicated RTL8139 PCI network card driver, enabling network connectivity.
- **Drivers**: Extensive hardware support including ATA/IDE, PS/2 input (keyboard/mouse), PCI/ACPI enumeration, PIT/APIC timers, and linear framebuffers.

## 3. Architecture
The DevilCore kernel architecture is built upon modular principles, ensuring separation of concerns and maintainability.
- **Core Kernel**: Manages interrupts (IDT/ISR), GDT, memory management, and process scheduling.
- **Hardware Abstraction Layer (HAL)**: Normalizes device communication across PCI, ACPI, and various bus protocols.
- **Userland Environment**: DevilUI and DevilComp form the desktop shell, providing a windowed environment for applications.

## 4. Getting Started
To get started with DevilCore OS in QEMU, ensure you have a standard GCC cross-compiler toolchain configured.

1. **Clone the repo**: `git clone https://github.com/DarkDevil404/DevilCore.git`
2. **Setup Dependencies**: Ensure `qemu-system-x86_64`, `nasm`, and a cross-compiled `x86_64-elf-gcc` are available in your path.
3. **Run**: `make run`

## 5. Building from Source
DevilCore uses a standard Makefile system.
- `make`: Builds the kernel binary and generates the bootable ISO.
- `make clean`: Removes intermediate build artifacts.
- `make debug`: Launches QEMU with GDB stub attached for kernel debugging.

## 6. Contributing
Contributions are welcome! Please follow these guidelines:
- Adhere to the existing C style and modular structure.
- Add unit tests for new features.
- Ensure no kernel warnings persist during build.

## 7. Roadmap
- [ ] Improve networking throughput and add DHCP/DNS support.
- [ ] Transition to a microkernel IPC model for drivers.
- [ ] Add support for hardware-accelerated drivers.

## 8. License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
