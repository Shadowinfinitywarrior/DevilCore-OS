/*
 * DevilCore OS — PIC (8259A) Driver Header
 */
#ifndef DC_PIC_H
#define DC_PIC_H

#include "../include/types.h"

void pic_init(void);
void pic_eoi(uint32_t irq);

#endif
