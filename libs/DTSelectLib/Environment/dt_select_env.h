// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear


#ifndef DT_SELECT_CUSTOM_ENV_H_
#define DT_SELECT_CUSTOM_ENV_H_
#ifdef TARGET_UEFI
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#else
/* Use below header file for using standard datatype */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#endif

#ifndef CHAR_BIT
#define CHAR_BIT  8
#endif

#ifndef size_t
#define size_t  UINTN
#endif

#ifndef true
#define true  (1 == 1)
#endif

#ifndef false
#define false  (1 == 0)
#endif

typedef INT8     int8_t;
typedef BOOLEAN  bool;

#endif /* DT_SELECT_CUSTOM_ENV_H_ */
