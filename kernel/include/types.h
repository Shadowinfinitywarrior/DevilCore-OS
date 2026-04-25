#ifndef DC_TYPES_H
#define DC_TYPES_H

/* Fixed 32-bit types for standalone kernel build */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

typedef uint32_t           size_t;
typedef uint32_t           uintptr_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define true  1
#define false 0
typedef int bool;

#endif
