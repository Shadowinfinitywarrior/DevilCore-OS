// kernel/init/main.c
// Kernel Main Entry Point

#include <devil/types.h>
#include <devil/config.h>

extern void gdt_init(void);
extern void idt_init(void);
extern void paging_init(void);
extern void pmm_init(uint64_t mem_size);
extern void vmm_init(void);
extern void scheduler_init(void);
extern void process_init(void);
extern void vfs_init(void);
extern void console_init(void);

extern void panic(const char *msg);

void kernel_main(void *boot_info) {
    // Early console
    console_init();
    printf("[DevilKernel] Starting DevilCore OS v%s.%s.%s\n",
           KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);
    
    // Get memory info from boot info
    uint64_t mem_size = 0x10000000; // 256MB default
    
    // Initialize memory management
    printf("[DevilKernel] Initializing memory management...\n");
    pmm_init(mem_size);
    paging_init();
    vmm_init();
    
    // Initialize GDT/IDT
    printf("[DevilKernel] Setting up GDT/IDT...\n");
    gdt_init();
    idt_init();
    
    // Initialize scheduler
    printf("[DevilKernel] Initializing scheduler...\n");
    scheduler_init();
    process_init();
    
    // Initialize VFS
    printf("[DevilKernel] Initializing VFS...\n");
    vfs_init();
    
    // Mount filesystems
    printf("[DevilKernel] Mounting filesystems...\n");
    vfs_mount(NULL, "tmpfs", "/tmp");
    vfs_mount(NULL, "devfs", "/dev");
    vfs_mount(NULL, "procfs", "/proc");
    
    // Start init process
    printf("[DevilKernel] Starting init...\n");
    extern void start_init(void);
    start_init();
    
    // Should never reach here
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void panic(const char *msg) {
    printf("[PANIC] %s\n", msg);
    for (;;) {
        __asm__ volatile("hlt");
    }
}