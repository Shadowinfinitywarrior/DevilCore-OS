# kernel/include/limits.h
#ifndef _LIMITS_H
#define _LIMITS_H

#define CHAR_BIT    8
#define CHAR_MAX   127
#define CHAR_MIN   (-128)
#define INT_MAX    2147483647
#define INT_MIN    (-2147483647-1)
#define LONG_MAX   9223372036854775807L
#define LONG_MIN   (-9223372036854775807L-1)
#define SCHAR_MAX  127
#define SCHAR_MIN (-128)
#define SHRT_MAX  32767
#define SHRT_MIN  (-32768)
#define UCHAR_MAX 255
#define UINT_MAX  4294967295U
#define ULONG_MAX 4294967295UL
#define USHRT_MAX 65535

#define _POSIX_ARG_MAX     4096
#define _POSIX_CHILD_MAX 6
#define _POSIX_LINK_MAX  8
#define _POSIX_MAX_CANON 255
#define _POSIX_MAX_INPUT 255
#define _POSIX_NAME_MAX  255
#define _POSIX_PATH_MAX  4095
#define _POSIX_PIPE_BUF  512
#define _POSIX_SSIZE_MAX 32767

#define NAME_MAX   255
#define PATH_MAX  4095
#define PIPE_BUF  512

#endif // _LIMITS_H