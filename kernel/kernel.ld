/* DevilCore OS Linker Script - Multiboot2 ELF32
 * Kernel loads at 1MB (0x100000)
 * .multiboot2 section MUST be within first 32KB of the image for GRUB
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* Load at 1MB physical */
    . = 0x100000;

    /* Multiboot2 header must be first */
    .multiboot2 ALIGN(8) :
    {
        KEEP(*(.multiboot2))
    }

    /* Code */
    .text ALIGN(4096) :
    {
        *(.text*)
    }

    /* Read-only data (font, strings, ISR table) */
    .rodata ALIGN(4096) :
    {
        *(.rodata*)
    }

    /* Initialized data */
    .data ALIGN(4096) :
    {
        *(.data*)
    }

    /* Uninitialized data */
    .bss ALIGN(4096) :
    {
        *(COMMON)
        *(.bss*)
    }

    /* Mark kernel end for PMM */
    _kernel_end = ALIGN(4096);
}