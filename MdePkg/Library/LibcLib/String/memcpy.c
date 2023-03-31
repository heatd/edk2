/** @file
  memcpy-like functions

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <string.h>
#include <Library/BaseMemoryLib.h>

void *
memcpy (
  void *restrict        Dst,
  const void *restrict  Src,
  size_t                Count
  )
{
  return CopyMem (Dst, Src, Count);
}

void *
memmove (
  void        *Dst,
  const void  *Src,
  size_t      Count
  )
{
  return CopyMem (Dst, Src, Count);
}
