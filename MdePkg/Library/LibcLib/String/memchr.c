/** @file
  memchr-like functions

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <string.h>
#include <Library/BaseMemoryLib.h>

void *
memchr (
  const void  *Buf,
  int         Char,
  size_t      Count
  )
{
  return ScanMem8 (Buf, Count, Char);
}
