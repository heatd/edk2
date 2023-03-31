/** @file
  strcmp-like implementations

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <string.h>
#include <Library/BaseLib.h>

int
strcmp (
  const char  *Str1,
  const char  *Str2
  )
{
  return (int)AsciiStrCmp (Str1, Str2);
}

int
strncmp (
  const char  *Str1,
  const char  *Str2,
  size_t      Count
  )
{
  return (int)AsciiStrnCmp (Str1, Str2, Count);
}
