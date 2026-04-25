#ifndef _DEVIL_TYPES_H
#define _DEVIL_TYPES_H

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

typedef uint64_t            size_t;
typedef int64_t             ssize_t;
typedef int64_t             off_t;
typedef int32_t             pid_t;
typedef uint32_t            uid_t;
typedef uint32_t            gid_t;
typedef uint32_t            mode_t;
typedef uint64_t            ino_t;
typedef int64_t             time_t;
typedef uint32_t            dev_t;
typedef uint64_t            blkcnt_t;
typedef uint32_t            blksize_t;
typedef uint64_t            uintptr_t;
typedef int64_t             intptr_t;

typedef _Bool               bool;
#define true                1
#define false               0
#define NULL                ((void*)0)

#define PACKED              __attribute__((packed))
#define ALIGNED(x)          __attribute__((aligned(x)))
#define UNUSED              __attribute__((unused))
#define NORETURN            __attribute__((noreturn))

// Page size
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           (~(PAGE_SIZE - 1))

// Align macros
#define ALIGN_UP(x, a)      (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))

// Min/Max
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

// Bit manipulation
#define BIT(n)              (1ULL << (n))
#define SET_BIT(x, n)       ((x) |= BIT(n))
#define CLEAR_BIT(x, n)     ((x) &= ~BIT(n))
#define TEST_BIT(x, n)      ((x) & BIT(n))

#endif // _DEVIL_TYPES_H
