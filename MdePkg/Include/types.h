/** @file
  ISO C auxiliary types file

  Copyright (c) 2023 Pedro Falcato All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/* C has a variety of types we must define in a lot of header files, without
   including the "canonical" header file due to namespace pollution reasons.
   So define them here and use __NEED_ macros to ask for certain definitions.
 */

#if defined (__NEED_size_t) && !defined (__defined_size_t)
typedef UINTN size_t;
#define __defined_size_t
#endif

#if defined (__NEED_NULL) && !defined (__defined_NULL)
  #include <Base.h>
#define __defined_NULL
// TODO: Namespace pollution
#endif

#if defined (__NEED_va_arg) && !defined (__defined_va_arg)

  #include <Base.h>
typedef VA_LIST va_list;
#define __defined_va_arg

#endif
