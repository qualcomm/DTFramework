// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear


#ifndef DT_SELECT_CUSTOM_ENV_H_
#define DT_SELECT_CUSTOM_ENV_H_

/*typedef UINTN    uintptr_t;
typedef UINTN    size_t;
typedef INT8     int8_t;
typedef UINT8    uint8_t;
typedef UINT16   uint16_t;
typedef UINT32   uint32_t;
typedef UINT64   uint64_t;
typedef BOOLEAN  bool;*/

#define true   1
#define false  0
/*
#ifdef DT_SELECT_LOGGING_ENABLE
#define snprintf(a,b,c, ...) AsciiSPrint(a, b, c, ##__VA_ARGS__)
#endif
*/

/* Use below header file for using standard datatype */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*#include <string.h>*/

#endif /* DT_SELECT_CUSTOM_ENV_H_ */
