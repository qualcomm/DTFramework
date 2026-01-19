// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

/** @file DTBInternals.h
  FdtLib (edk2) Extended APIs
**/

#ifndef DTBINTERNALS_H_
#define DTBINTERNALS_H_

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/

/*=========================================================================
     Default Defines
==========================================================================*/

#define PTR_CHECK(ptr) do { \
	if (NULL == ptr) return -FDT_ERR_QC_NULLPTR; \
} while(0)

#define BLOBID_CHECK(id) do { \
	if ((id < 0) || (id >= MAX_BLOB_ID)) return -FDT_ERR_QC_BLOBID; \
} while(0)

#define LEN_ERROR_CHECK(code) do { \
	if (code < 0) \
		ret_value = code; \
	else \
		ret_value = -FDT_ERR_QC_FDTLIB_ERROR; \
} while(0)

/* All lookups are string-based */
/* Convenient buffer size for static or dynamic allocation */
#define DTB_LINE_BUF_SIZE 0x100

/* Config settings for library, controls debug output */
typedef struct {
	int	client;
	int	trace;
	int	verbose;
} Config;
extern Config dtb_config;


#endif  /* DTBINTERNALS_H_ */
