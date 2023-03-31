/** @file
  strcpy-like implementations

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <string.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

char *
strcpy (
  char        *restrict  Dest,
  const char  *restrict  Source
  )
{
  char  *Ret;

  Ret = Dest;

  for ( ; *Source != '\0'; Source++, Dest++) {
    *Dest = *Source;
  }

  *Dest = '\0';

  return Ret;
}

char *
strncpy (
  char        *restrict  Dest,
  const char  *restrict  Source,
  size_t                 Count
  )
{
  char  *Ret;

  Ret = Dest;

  while (Count--) {
    if (*Source != '\0') {
      *Dest++ = *Source++;
    } else {
      *Dest++ = '\0';
    }
  }

  return Ret;
}

char *
strcat (
  char        *restrict  Dest,
  const char  *restrict  Source
  )
{
  return strcpy (Dest + strlen (Dest), Source);
}
