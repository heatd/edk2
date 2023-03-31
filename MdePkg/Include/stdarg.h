/** @file
  ISO C stdarg.h

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _STDARG_H
#define _STDARG_H

#define __NEED_va_arg
#include <types.h>

// These uppercase macros have been defined in Base.h, included from types.h

#define va_start(Marker, Param)  VA_START(Marker, Param)

#define va_arg(Marker, TYPE)  VA_ARG(Marker, TYPE)

#define va_end(Marker)  VA_END(Marker)

#define va_copy(Dest, Start)  VA_COPY(Dest, Start)

#endif
