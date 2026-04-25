/*
 * DevilCore OS — Shared Type Definitions
 * Used across all kernel modules
 */
#ifndef DC_TYPES_H
#define DC_TYPES_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;
typedef unsigned long      size_t;
typedef unsigned long      uintptr_t;

#define NULL  ((void*)0)
#define true  1
#define false 0
typedef int bool;

#define ALIGN_UP(x, a)   (((x) + (a)-1) & ~((a)-1))
#define ALIGN_DOWN(x,a)  ((x) & ~((a)-1))
#define MIN(a,b)         ((a)<(b)?(a):(b))
#define MAX(a,b)         ((a)>(b)?(a):(b))
#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))
#define ABS(x)           ((x)<0?-(x):(x))

#endif /* DC_TYPES_H */
