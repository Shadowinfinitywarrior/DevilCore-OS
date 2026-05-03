#ifndef DEVILCORE_SMP_H
#define DEVILCORE_SMP_H

#include <stdint.h>
#include "limine.h"
#include "spinlock.h"

void smp_init(struct limine_smp_response *smp);
int smp_get_cpu_id(void);

#endif
