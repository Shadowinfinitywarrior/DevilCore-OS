/* DevilCore OS Linker Script - Multiboot 1 ELF32
 * Kernel loads at 1MB (0x100000)
 */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    /* Load at 1MB physical */
    . = 0x100000;

    .text ALIGN(4096) :
    {
        /* Multiboot 1 header MUST be early */
        *(.multiboot)
        *(.text*)
    }

    /* Read-only data (font, strings, etc) */
    .rodata ALIGN(4096) :
    {
        *(.rodata*)
    }

    /* Initialized data */
    .data ALIGN(4096) :
    {
        *(.data*)
    }

    /* Uninitialized data and stack */
    .bss ALIGN(4096) :
    {
        *(COMMON)
        *(.bss*)
        
        /* Define heap start at end of BSS */
        . = ALIGN(4096);
        __heap_base = .;
    }

    /* Mark kernel end */
    _kernel_end = ALIGN(4096);
}