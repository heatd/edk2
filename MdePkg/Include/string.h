/** @file
  ISO C string.h

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _STRING_H
#define _STRING_H

#define __NEED_size_t
#define __NEED_NULL
#include <types.h>

void *
memcpy (
  void *restrict        Dst,
  const void *restrict  Src,
  size_t                Count
  );

void *
memmove (
  void        *Dst,
  const void  *Src,
  size_t      Count
  );

void *
memset (
  void    *Buf,
  int     Val,
  size_t  Count
  );

void *
memchr (
  const void  *Buf,
  int         Char,
  size_t      Count
  );

int
memcmp (
  const void  *S1,
  const void  *S2,
  size_t      Count
  );

size_t
strlen (
  const char  *Str
  );

char *
strcpy (
  char *restrict        Dest,
  const char *restrict  Source
  );

char *
strncpy (
  char        *restrict  Dest,
  const char  *restrict  Source,
  size_t                 Count
  );

char *
strcat (
  char        *restrict  Dest,
  const char  *restrict  Source
  );

char *
strchr (
  const char  *Str,
  int         Char
  );

char *
strrchr (
  const char  *Str,
  int         Char
  );

#endif
