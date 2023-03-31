/** @file
  strlen implementation

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <string.h>
#include <Library/BaseLib.h>

size_t
strlen (
  const char  *Str
  )
{
  return AsciiStrLen (Str);
}
