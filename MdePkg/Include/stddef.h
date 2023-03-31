/** @file
  ISO C stddef.h

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _STDDEF_H
#define _STDDEF_H

#include <Base.h> // For NULL
// TODO: Namespace polution

typedef INTN    ptrdiff_t;
typedef UINTN   size_t;
typedef CHAR16  wchar_t;

// offsetof taken from Base.h

#if (defined (__GNUC__) && __GNUC__ >= 4) || defined (__clang__)
#define offsetof(TYPE, Field)  ((UINTN) __builtin_offsetof(TYPE, Field))
#else
#define offsetof(TYPE, Field)  ((UINTN) &(((TYPE *)0)->Field))
#endif

#endif
