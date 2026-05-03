#ifndef DEVILCORE_SPINLOCK_H
#define DEVILCORE_SPINLOCK_H

#include <stdint.h>

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

static inline void spin_lock(spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        __asm__ volatile ("pause");
    }
}

static inline void spin_unlock(spinlock_t *lock) {
    __sync_lock_release(&lock->lock);
}

static inline void spin_init(spinlock_t *lock) {
    lock->lock = 0;
}

#endif
