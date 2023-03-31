/** @file
  ISO C stdint.h

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _STDINT_H
#define _STDINT_H

// INT(N), UINT(N) taken from ProcessorBind.h
typedef INT8   int8_t;
typedef INT16  int16_t;
typedef INT32  int32_t;
typedef INT64  int64_t;

typedef UINT8   uint8_t;
typedef UINT16  uint16_t;
typedef UINT32  uint32_t;
typedef UINT64  uint64_t;

typedef int8_t   int_least8_t;
typedef int16_t  int_least16_t;
typedef int32_t  int_least32_t;
typedef int64_t  int_least64_t;

typedef uint8_t   uint_least8_t;
typedef uint16_t  uint_least16_t;
typedef uint32_t  uint_least32_t;
typedef uint64_t  uint_least64_t;

typedef int8_t   int_fast8_t;
typedef int16_t  int_fast16_t;
typedef int32_t  int_fast32_t;
typedef int64_t  int_fast64_t;

typedef uint8_t   uint_fast8_t;
typedef uint16_t  uint_fast16_t;
typedef uint32_t  uint_fast32_t;
typedef uint64_t  uint_fast64_t;

typedef INTN   intptr_t;
typedef UINTN  uintptr_t;

typedef INT64   intmax_t;
typedef UINT64  uintmax_t;

/* Limits for the types declared above */
#define INT8_MIN    -128
#define INT8_MAX    127
#define UINT8_MAX   255
#define INT16_MIN   (-1 - 0x7fff)
#define INT16_MAX   0x7fff
#define UINT16_MAX  0xffff
#define INT32_MIN   (-1 - 0x7fffffff)
#define INT32_MAX   0x7fffffff
#define UINT32_MAX  0xffffffffU
#define INT64_MIN   (-1 - 0x7fffffffffffffffLL)
#define INT64_MAX   0x7fffffffffffffffLL
#define UINT64_MAX  0xffffffffffffffffULL

#define INT_LEAST8_MIN    INT8_MIN
#define INT_LEAST8_MAX    INT8_MAX
#define UINT_LEAST8_MAX   UINT8_MAX
#define INT_LEAST16_MIN   INT16_MIN
#define INT_LEAST16_MAX   INT16_MAX
#define UINT_LEAST16_MAX  UINT16_MAX
#define INT_LEAST32_MIN   INT32_MIN
#define INT_LEAST32_MAX   INT32_MAX
#define UINT_LEAST32_MAX  UINT32_MAX
#define INT_LEAST64_MIN   INT64_MIN
#define INT_LEAST64_MAX   INT64_MAX
#define UINT_LEAST64_MAX  UINT64_MAX

#define INT_FAST8_MIN    INT8_MIN
#define INT_FAST8_MAX    INT8_MAX
#define UINT_FAST8_MAX   UINT8_MAX
#define INT_FAST16_MIN   INT16_MIN
#define INT_FAST16_MAX   INT16_MAX
#define UINT_FAST16_MAX  UINT16_MAX
#define INT_FAST32_MIN   INT32_MIN
#define INT_FAST32_MAX   INT32_MAX
#define UINT_FAST32_MAX  UINT32_MAX
#define INT_FAST64_MIN   INT64_MIN
#define INT_FAST64_MAX   INT64_MAX
#define UINT_FAST64_MAX  UINT64_MAX

#define INTPTR_MIN   (1 - MAX_INTN)
#define INTPTR_MAX   MAX_INTN
#define UINTPTR_MAX  MAX_UINTN

#define INTMAX_MIN   INT64_MIN
#define INTMAX_MAX   INT64_MAX
#define UINTMAX_MAX  UINT64_MAX

#define PTRDIFF_MIN  INTPTR_MIN
#define PTRDIFF_MAX  INTPTR_MAX
#define SIZE_MAX     MAX_UINTN

// TODO: SIG_ATOMIC, WCHAR, WINT

/* Macros to declare (u)int(N)_t constants */

#define INT8_C(c)    c
#define INT16_C(c)   c
#define INT32_C(c)   c
#define INT64_C(c)   c ## LL
#define UINT8_C(c)   c
#define UINT16_C(c)  c
#define UINT32_C(c)  c ## U
#define UINT64_C(c)  c ## ULL

#define INTMAX_C(c)   c ## LL
#define UINTMAX_C(c)  c ## ULL

#endif
