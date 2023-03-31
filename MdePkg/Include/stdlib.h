/** @file
  ISO C stdlib.h

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _STDLIB_H
#define _STDLIB_H

#define __NEED_NULL
#include <types.h>

unsigned long
strtoul (
  const char  *Nptr,
  char        **EndPtr,
  int         Base
  );

#endif
