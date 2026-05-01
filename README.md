# DevilCore OS (v2.0 'Premium Edition')

**Developer:** Mr. Nithish Kathiravan
**Contact:** nithishkathiravan123@gmail.com / infonity404@gmail.com
**Phone:** +91 9342358022

---

## 1. Overview
DevilCore OS is an advanced 64-bit operating system designed from the ground up to provide a robust, aesthetic, and functional environment for ethical hacking and systems research. It features a custom micro-kernel inspired architecture with a focus on high-performance graphics, fair scheduling, and modular hardware abstraction.

## 2. Technical Architecture Deep Dive

### 2.1 Boot Process & Initialisation
DevilCore utilizes the Limine bootloader to transition from the firmware environment (BIOS/UEFI) to 64-bit long mode. The boot process follows these stages:
1. **Bootloader Handover:** Limine provides a memory map, framebuffer information, and a higher-half direct mapping of physical memory.
2. **GDT & IDT Setup:** The Global Descriptor Table (GDT) is initialized with kernel and user segments. The Interrupt Descriptor Table (IDT) is populated with 256 entries, handling CPU exceptions and hardware IRQs.
3. **Memory Management:** The Physical Memory Manager (PMM) is initialized using the Limine memory map. The Virtual Memory Manager (VMM) sets up the kernel's own page tables, ensuring isolation and security.
4. **Slab Allocator:** A specialized allocator is initialized to provide efficient allocation for small kernel structures, minimizing fragmentation.

### 2.2 Memory Management (PMM & VMM)
The PMM uses a bitmap-based approach to track page frame availability. Each bit represents a 4KB page of physical RAM.
The VMM manages the 4-level paging structure (PML4, PDPT, PD, PT). It provides functions for mapping, unmapping, and remapping virtual addresses to physical frames.

#### Memory Compression (DevilCompress)
DevilCore features a unique RLE-based memory compression algorithm that compresses idle pages in the background, effectively increasing the available memory without physical hardware upgrades.

### 2.3 CFS Scheduler (Completely Fair Scheduler)
The DevilCore scheduler is based on the CFS algorithm used in Linux.
- **Vruntime:** Each task tracks its "virtual runtime," which is the amount of time it has spent on the CPU, weighted by its priority.
- **Red-Black Tree:** Tasks are stored in a self-balancing red-black tree, indexed by their vruntime.
- **Selection:** The scheduler always picks the task with the smallest vruntime (the leftmost node in the tree), ensuring O(log N) complexity for both insertion and selection.

### 2.4 Virtual File System (VFS)
The VFS provides a unified interface for accessing different filesystem types and hardware devices.
- **Node-based Abstraction:** Every file, directory, and device is represented as a `vfs_node`.
- **Mount Points:** Filesystems like FAT32 are mounted onto the VFS tree.
- **DevFS:** Special device files (e.g., `/dev/mouse`, `/dev/fb0`) allow standard file operations (read/write) to interact with hardware drivers.

## 3. The DevilComp Compositor & GUI

### 3.1 Graphical Pipeline
DevilCore's graphics stack is built for beauty and performance.
1. **Framebuffer Driver:** Manages the linear framebuffer provided by the bootloader.
2. **Backbuffer:** All rendering is performed on an off-screen backbuffer to prevent flickering.
3. **Compositing Engine:** Instead of windows drawing directly to the screen, each window has its own surface. The compositor blends these surfaces together, supporting:
    - **Alpha Blending:** Transparent window borders and shadows.
    - **Damage Tracking:** Only redrawing the parts of the screen that actually changed.
    - **Glass Effects:** Real-time gradient and transparency effects on title bars.

### 3.2 DevilUI Widget Toolkit
DevilUI is an object-oriented toolkit written in C.
- **Hierarchy:** Widgets are organized in a parent-child tree.
- **Event Propagation:** Mouse and keyboard events flow through the tree, allowing for complex interactive components.
- **Theme System:** Supports dynamic theme switching (Dark, Blue, Green, etc.) by modifying a global theme structure.

## 4. Integrated Applications

### 4.1 DevilCore Terminal
A high-performance terminal emulator supporting ASCII art, color escape codes, and a custom shell. The terminal is optimized for minimal lag and smooth scrolling.

### 4.2 Privacy Browser
A simulated web browser environment focused on privacy. It features tabbed browsing, a secure address bar, and integrated ad-blocking logic.

### 4.3 System Monitor
A live dashboard showing real-time CPU usage (with a scrolling graph), memory consumption, and a process list. It allows for process management, including killing unresponsive tasks.

### 4.4 Quick Notes & Calculator
Productivity tools designed with a "hacker" aesthetic. The Notes app supports multi-line editing and persistent storage (planned), while the Calculator includes a scientific mode with history tracking.

### 4.5 Calendar
A newly added calendar application that provides a clean, monthly view of the year 2026, integrated directly into the taskbar and start menu.

## 5. Security Features
- **Ethical Hacking Environment:** Integrated tools for network scanning (ARP/ICMP) and system auditing.
- **Isolation:** Kernel/User mode separation and paging-based memory protection.
- **Encrypted Filesystem (Planned):** Future support for AES-XTS encrypted partitions.

## 6. Development & Building

### Prerequisites
- `x86_64-elf-gcc` (or `x86_64-linux-gnu-gcc`)
- `nasm`
- `make`
- `xorriso` (for ISO generation)
- `qemu-system-x86_64`

### Build Commands
```bash
make all      # Build the kernel and ISO
make run      # Launch in QEMU
make clean    # Remove build artifacts
```

## 7. Contact & Support
For support, contributions, or inquiries, please contact:
**Mr. Nithish Kathiravan**
- **Email:** nithishkathiravan123@gmail.com / infonity404@gmail.com
- **Phone:** +91 9342358022
