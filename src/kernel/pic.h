#ifndef DEVILCORE_PIC_H
#define DEVILCORE_PIC_H

#include <stdint.h>

void pic_init(void);
void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);
void pic_end_of_interrupt(uint8_t irq);

#endif