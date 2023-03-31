/** @file
  ISO C limits.h

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _LIMITS_H
#define _LIMITS_H

#if defined (MDE_CPU_X64) || defined (MDE_CPU_AARCH64)
// Hint for LONG_* stuff
#define __LIMITS_64BIT
#endif

#ifndef __GNUC__
// TODO: MSVC support is missing (some types are not exactly the same)
// Should this whole logic be in ProcessorBind.h or something?
  #error "MSVC support TODO"
#endif

#define CHAR_BIT  8

/* Char limits - for char, signed char, unsigned char */
#define SCHAR_MIN  -128
#define SCHAR_MAX  127
#define UCHAR_MAX  255

// Note: We must check if chars are signed or unsigned here. 0xff = -128 for signed chars
#if '\xff' < 0
#define __CHAR_IS_SIGNED
#endif

#ifdef __CHAR_IS_SIGNED
#define CHAR_MIN  SCHAR_MIN
#define CHAR_MAX  SCHAR_MAX
#else
#define CHAR_MIN  0
#define CHAR_MAX  UCHAR_MAX
#endif

/* Short limits - for short, unsigned short */
#define SHRT_MIN   (-1 - 0x7fff)
#define SHRT_MAX   0x7fff
#define USHRT_MAX  0xffff

/* Int limits - for int, unsigned int */
#define INT_MIN   (-1 - 0x7fffffff)
#define INT_MAX   0x7fffffff
#define UINT_MAX  0xffffffffU

/* Long limits - for long, unsigned long and long long variants */

#ifdef __LIMITS_64BIT
#define LONG_MAX   0x7fffffffffffffffL
#define LONG_MIN   (-1 - 0x7fffffffffffffffL)
#define ULONG_MAX  0xffffffffffffffffUL
#else
#define LONG_MAX   0x7fffffffL
#define LONG_MIN   (-1 - 0x7fffffffL)
#define ULONG_MAX  0xffffffffUL
#endif

/* long long must always be 64-bit for EFI UINT64 */
#define LLONG_MIN   (-1 - 0x7fffffffffffffffLL)
#define LLONG_MAX   0x7fffffffffffffffLL
#define ULLONG_MAX  0xffffffffffffffffULL

#endif
