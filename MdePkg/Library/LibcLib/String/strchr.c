/** @file
  strchr-like implementations

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Base.h>

// Very quick notes:
// We only go through the string once for both functions
// They are minimal implementations (not speed optimized) of ISO C semantics
// strchr and strrchr also include the null terminator as part of the string
// so the code gets a bit clunky to handle that case specifically.

char *
strchr (
  const char  *Str,
  int         Char
  )
{
  char  *S;

  S = (char *)Str;

  for ( ; ; S++) {
    if (*S == Char) {
      return S;
    }

    if (*S == '\0') {
      return NULL;
    }
  }
}

char *
strrchr (
  const char  *Str,
  int         Char
  )
{
  char  *S, *last;

  S    = (char *)Str;
  last = NULL;

  for ( ; ; S++) {
    if (*S == Char) {
      last = S;
    }

    if (*S == '\0') {
      return last;
    }
  }
}
