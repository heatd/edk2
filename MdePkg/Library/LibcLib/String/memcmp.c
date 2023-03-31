/** @file
  memcmp-like functions

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <string.h>
#include <Library/BaseMemoryLib.h>

int
memcmp (
  const void  *S1,
  const void  *S2,
  size_t      Count
  )
{
  return (int)CompareMem (S1, S2, Count);
}
