// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

/** @file DTBExtnLib_overlay.c
  FdtLib (edk2) Extended APIs
**/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/

#include "libfdt.h"
#include "DTBExtnLib.h"
#include "DTBInternals.h"
#include "DTBExtnLib_env.h"

/*=========================================================================
     Default Defines
==========================================================================*/

/*=========================================================================
     Local Static Variables
==========================================================================*/

/*=========================================================================
     Globals
==========================================================================*/

/*=========================================================================
      Public APIs
==========================================================================*/

/**
 * fdt_merge_overlay - Merge overlay into primary blob
 * @primary_blob:    Primary DTB tree
 * @pb_size:         size of Primary Blob
 * @overlay_blob:    Overlay DTB tree
 * @ob_size:         size of Overlay Blob
 * @merge_blob:      Merged  DTB tree
 * @mb_size: 		 size of Merge Blob
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_totalsize()
 * 		fdt_open_into()
 * 		fdt_overlay_apply() 
 */
int fdt_merge_overlay(const void *primary_blob, size_t pb_size, void *overlay_blob, size_t ob_size, void *merge_blob, size_t mb_size)
{
	int ret_value;

	#ifdef INSTRUMENTATION
	char ibuffer[DTB_LINE_BUF_SIZE];
	#endif

	PTR_CHECK(primary_blob);
	PTR_CHECK(overlay_blob);
	PTR_CHECK(merge_blob);

	/* primary_blob should be a valid DTB */
	ret_value = fdt_check_for_valid_blob(primary_blob, pb_size);
	if (FDT_ERR_QC_NOERROR != ret_value) goto exit;
	/* overlay_blob should be a valid DTBO */
	ret_value = fdt_check_for_valid_blob(overlay_blob, ob_size);
	if (FDT_ERR_QC_NOERROR != ret_value) goto exit;
	/* mb_size has to be larger than ob_size and pb_size */
	if ( (mb_size <= fdt_totalsize(overlay_blob)) || (mb_size <= fdt_totalsize(primary_blob)) ) {
		if (dtb_config.verbose) {
			SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Merge Buffer too small[%ld]", mb_size);
			PUTS(ibuffer);
		}
		ret_value = -FDT_ERR_QC_BUF2SMALL;
		goto exit;
	}

	/* basic parameter sanity checks now complete & underflow/overflow should not occur from here */
	if ( fdt_totalsize(primary_blob) > (mb_size - fdt_totalsize(overlay_blob)) ) {
		if (dtb_config.verbose) {
			SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Merge Buffer too small[%ld]", mb_size);
			PUTS(ibuffer);
		}
		ret_value = -FDT_ERR_QC_BUF2SMALL;
		goto exit;
	} else {
		ret_value = fdt_open_into(primary_blob, merge_blob, mb_size);
		if (FDT_ERR_QC_NOERROR != ret_value) {
			if (dtb_config.verbose) {
				SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "fdt_open_into ERROR[%d]", ret_value);
				PUTS(ibuffer);
			}
		} else {
			ret_value = fdt_overlay_apply(merge_blob, overlay_blob);
			if (FDT_ERR_QC_NOERROR != ret_value) {
				if (dtb_config.verbose) {
					SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "fdt_overlay_apply ERROR[%d]", ret_value);
					PUTS(ibuffer);
				}
			}
		}
	}

	if (FDT_ERR_QC_NOERROR == ret_value) {
		/* sanity check dtb blob */
		ret_value = fdt_check_header(merge_blob);
		if (FDT_ERR_QC_NOERROR != ret_value) {
			if (dtb_config.verbose) {
				PUTS("merge_blob failed fdt_check_header");
			}
			ret_value = -FDT_ERR_BADMAGIC;
		}
		
	}

exit:
	if (dtb_config.trace) {
		SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d]", __func__, ret_value);
		PUTS(ibuffer);
	}

	return ret_value;
}
