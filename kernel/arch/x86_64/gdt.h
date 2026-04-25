#ifndef _GDT_H
#define _GDT_H

#include <devil/types.h>

void gdt_init(void);
void tss_init(void);
void tss_set_kernel_stack(uint64_t stack);

#endif // _GDT_H
