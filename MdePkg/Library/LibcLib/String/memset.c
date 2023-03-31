/** @file
  memcpy-like functions

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <string.h>
#include <Library/BaseMemoryLib.h>

void *
memset (
  void    *Buf,
  int     Val,
  size_t  Count
  )
{
  // The standard defines memset as converting Val into an unsigned char before storing,
  // so this cast is entirely safe.
  return SetMem (Buf, Count, (UINT8)Val);
}
