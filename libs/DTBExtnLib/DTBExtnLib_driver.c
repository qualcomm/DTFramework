// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

/** @file DTBExtnLib_driver.c
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
 * fdt_get_next_node_handle_for_compatible - return node offset w/ compatible string
 * @node: pointer to fdt_node_handle object
 * @compatible: pointer to compatible string to search for
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 *   int fdt_node_offset_by_compatible(const void *fdt, int startoffset, const char *compatible);
 */
int fdt_get_next_node_handle_for_compatible(fdt_node_handle *node, const char *compatible)
{
    int offset;                    /* FdtLib: return raw prop data */
    int ret_value = FDT_ERR_QC_NOERROR;

    #ifdef INSTRUMENTATION
    char ibuffer[DTB_LINE_BUF_SIZE];
    #endif

    PTR_CHECK(node);
    PTR_CHECK(compatible);

    offset = fdt_node_offset_by_compatible(node->blob, node->offset, compatible);
    if (offset < 0) {
        if (dtb_config.verbose) {
            SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "BAD offset[%d].[%a]", offset, compatible);
            PUTS(ibuffer);
        }
        ret_value = offset;
    } else
        node->offset = offset;

    if (dtb_config.trace) {
        SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%d][%a]", __func__, ret_value, offset, compatible);
        PUTS(ibuffer);
    }

    return ret_value;
}

/**
 * fdt_get_name_index - return index of target
* @node: pointer to fdt_node_handle object
* @pname: Name of string-list property
* @tname: Target name in string-list property
* @index: (return) index of target in list
*
* returns FDT_ERR_QC_NOERROR (success) else error indicator
* 
* FdtLib calls:
* const struct fdt_property *fdt_get_property(const void *fdt, int nodeoffset, const char *name, int *lenp);
*/
int fdt_get_name_index(fdt_node_handle *node, const char *pname, char *tname, int *index)
{
    int len;                            /* FdtLib: return errors & size */
    const struct fdt_property *fdtp;    /* FdtLib: return raw prop data */
    int ret_value = -FDT_ERR_NOTFOUND;

    int lindex = 0;            /* local index */
    int mlen;
    const char *sptr;        /* string ptr into blob */
    int i = 0;

    #ifdef INSTRUMENTATION
    char ibuffer[DTB_LINE_BUF_SIZE];
    #endif

	/* find pname (reg-names) property in blob */
	fdtp = fdt_get_property(node->blob, node->offset, pname, &len);
	if ((NULL == fdtp) || (len < 0)) {
		if (dtb_config.verbose) {
			SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "fdt_get_property returns[%d]", len);
			PUTS(ibuffer);
		}
		LEN_ERROR_CHECK(len);
		goto exit;
	} else {
		sptr = fdtp->data;			/* data should be array of strings */
		mlen = AsciiStrLen(tname)+1;		/* tname = target name being looked for */
		do {
			if (!memcmp(&sptr[i], tname, mlen)) {
				/* target (tname) found */
				*index = lindex;
				ret_value = FDT_ERR_QC_NOERROR;
				break;
			}
			i += AsciiStrLen(&sptr[i])+1;	/* walk to next name in array */
			lindex++;
		} while (i < len);
	}

exit:
	if (dtb_config.trace) {
		SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%a][%a][%d]", __func__, ret_value, pname, tname, *index);
		PUTS(ibuffer);
	}
	return ret_value;
}

static int _fdt_get_size_code(fdt_node_handle *node, int *addr_code, int *size_code)
{
    return -FDT_ERR_QC_NOTSUPPORTED;
}

/**
 * fdt_get_reg - return specific reg entry
 * @node: pointer to fdt_node_handle object
 * @reg_name:  NULL or name of index in range to return
 * @reg_index: index of range to return
 * @addr_code: indicate address size [-1..2]
 * @size_code: indicate cell size [-1..2]
 * @reg_paddr: (return) physical address
 * @reg_size:  (return) size of this region
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls: fdt_getprop()
 *               fdt32_to_cpu()
 *               fdt64_to_cpu()
 */
int fdt_get_reg(fdt_node_handle *node,
                           char *reg_name,
                            int  reg_index,
                            int  addr_code,
                            int  size_code,
                       uint64_t *reg_paddr,
                       uint64_t *reg_size)
{
    int len;                    /* FdtLib: return errors & size */
    const void *prop_value;        /* FdtLib: return property value */
    uint32_t *reg_data;
    int ret_value = FDT_ERR_QC_NOERROR;

    int reg_count = 0;            /* count of values in reg property */
    int index_size = 0;            /* size of each entry in reg<> property */
    int target_idx;                /* targeted reg<> entry */
    uint32_t d32;
    uint64_t d64;

    #ifdef INSTRUMENTATION
    char ibuffer[DTB_LINE_BUF_SIZE];
    #endif

    /* reg_name can be NULL so not checked here */
    PTR_CHECK(node);
    PTR_CHECK(reg_paddr);
    PTR_CHECK(reg_size);

    /* valid #address_cells and #size_cells values required to parse reg<> property */
    if ( (SIZE_BLOB == addr_code) || (SIZE_BLOB == size_code) ) {
        ret_value = _fdt_get_size_code(node, &addr_code, &size_code);
        if (ret_value) {
            if (dtb_config.verbose) {
                SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Reg sizes not found[%d]", ret_value);
                PUTS(ibuffer);
            }
            goto err_exit;
        }
    }
    /* validate addr_code */
    switch (addr_code) {
        case SIZE_32:
        case SIZE_64:
            break;
        case SIZE_NIL:    /* nil is not valid for #address-cells */
        default:
            if (dtb_config.verbose) {
                SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Size code[%d] BAD", addr_code);
                PUTS(ibuffer);
            }
			ret_value = -FDT_ERR_QC_INPUT_ARG_ERR;
            goto err_exit;
            break;
    }
    /* validate size_code */
    switch (size_code) {
        case SIZE_NIL:    /* nil is valid for #size-cells */
        case SIZE_32:
        case SIZE_64:
            break;
        default:
            if (dtb_config.verbose) {
                SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Size code[%d] BAD", size_code);
                PUTS(ibuffer);
            }
			ret_value = -FDT_ERR_QC_INPUT_ARG_ERR;
            goto err_exit;
            break;
    }

    /* index into reg<> can be specified as a name or an index, name takes prioritiy */
    if (NULL != reg_name) {
        ret_value = fdt_get_name_index(node, "reg-names", reg_name, &reg_index);
        if (ret_value) {
            if (dtb_config.verbose) {
                SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Reg[%a] not found[%d]",
                    reg_name, ret_value);
                PUTS(ibuffer);
            }
        }
    }

    /* validate reg_index is in proper range */
    if (reg_index < 0) {
        if (dtb_config.verbose) {
            SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Reg index[%d] < 0", ret_value);
            PUTS(ibuffer);
        }
		ret_value = -FDT_ERR_QC_REGIDX;
        goto err_exit;
    }

    /* all input parameters now checked and good to go */

    /* Step 1: determine how many reg<> values there are */
    prop_value = fdt_getprop(node->blob, node->offset, "reg", &len);
	if ((NULL == prop_value) || (len < 0)) {
        if (dtb_config.verbose) {
            SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Err[%d].Prop[%a]", len, "reg");
            PUTS(ibuffer);
        }
		LEN_ERROR_CHECK(len);
        goto err_exit;
    } else {
        reg_count = len/4;        /* len comes back as byte count */
        reg_data = (uint32_t *)prop_value;
    }

    /* Step 2: find requested index in data */
    index_size = addr_code + size_code;
    target_idx = index_size * reg_index;
	if (target_idx > (reg_count-index_size)) {
        if (dtb_config.verbose) {
            SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "Target index[%d] too large[%d]", target_idx, reg_index);
            PUTS(ibuffer);
        }
		ret_value = -FDT_ERR_QC_TARGETIDX;
        goto err_exit;
    }


    /* Step 3: extract data from blob */
    switch (addr_code) {
        case SIZE_32:
            d32 = fdt32_to_cpu(reg_data[target_idx]);
            target_idx += 1;
            *reg_paddr = (uint64_t)d32;
            break;
        case SIZE_64:
            d64 = fdt64_to_cpu((uint64_t)reg_data[target_idx] | ((uint64_t)reg_data[target_idx + 1])<<32 );
            target_idx += 2;
            *reg_paddr = d64;
            break;
        default:    /* defeat compiler/KW warnings */
            break;
    }
    switch (size_code) {
        case SIZE_32:
            d32 = fdt32_to_cpu(reg_data[target_idx]);
            *reg_size = (uint64_t)d32;
            break;
        case SIZE_64:
            d64 = fdt64_to_cpu((uint64_t)reg_data[target_idx] | ((uint64_t)reg_data[target_idx + 1])<<32 );
            *reg_size = d64;
            break;
        case SIZE_NIL:
            *reg_size = 0;
            break;
        default:    /* defeat compiler/KW warnings */
            break;
    }
    ret_value = FDT_ERR_QC_NOERROR;

err_exit:
    if (dtb_config.trace) {
        SNPRINTF(ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%d][%d][%d]",
            __func__, ret_value, reg_index, addr_code, size_code);
        PUTS(ibuffer);
    }
    return ret_value;
}
