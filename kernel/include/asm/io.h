/* kernel/include/asm/io.h */
#ifndef _ASM_IO_H
#define _ASM_IO_H

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline void insb(uint16_t port, void *buffer, uint32_t count) {
    __asm__ volatile("rep insb" : "+D"(buffer), "+c"(count) : "d"(port));
}

static inline void insw(uint16_t port, void *buffer, uint32_t count) {
    __asm__ volatile("rep insw" : "+D"(buffer), "+c"(count) : "d"(port));
}

static inline void insl(uint16_t port, void *buffer, uint32_t count) {
    __asm__ volatile("rep insl" : "+D"(buffer), "+c"(count) : "d"(port));
}

static inline void outsb(uint16_t port, const void *buffer, uint32_t count) {
    __asm__ volatile("rep outsb" : "+S"(buffer), "+c"(count) : "d"(port));
}

static inline void outsw(uint16_t port, const void *buffer, uint32_t count) {
    __asm__ volatile("rep outsw" : "+S"(buffer), "+c"(count) : "d"(port));
}

static inline void outsl(uint16_t port, const void *buffer, uint32_t count) {
    __asm__ volatile("rep outsl" : "+S"(buffer), "+c"(count) : "d"(port));
}

static inline void iowait(void) {
    outb(0x80, 0);
}

#endif /* _ASM_IO_H */