/** @file
  ISO C assert.h

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _ASSERT_H
#define _ASSERT_H

#include <Library/DebugLib.h>

#ifdef NDEBUG
#define assert(...)  ((void)0)
#else
#define assert(expression)  ASSERT(expression)
#endif

#endif
