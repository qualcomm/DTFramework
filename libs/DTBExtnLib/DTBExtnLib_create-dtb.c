// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

/** @file DTBExtnLib_create-dtb.c
  FdtLib (edk2) Extended APIs
**/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/

#include <com_dtypes.h>
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
 * fdt_add_tree -	Create empty tree in memory
 * @node:			node handle pointer, node->blob must be populated by caller
 * @size:			size of buffer for new tree
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_create_empty_tree() 
 */
int fdt_add_tree(fdt_node_handle *node, int size)
{
    int ret_value;

	PTR_CHECK(node);
	PTR_CHECK(node->blob);

    node->offset = 0;

    ret_value = fdt_create_empty_tree((void *)node->blob, size);
    return ret_value;
}

/**
 * fdt_add_node -	Add node to new tree in memory
 * @node:			node handle pointer
 * @path:			name of node to add
 * @offset:			offset of newly created node
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_add_subnode() 
 */
int fdt_add_node(fdt_node_handle *node, char *path, int *offset)
{
    int ret_value; 
    
    PTR_CHECK(node);
    PTR_CHECK(path);
    PTR_CHECK(offset);

	/* basic sanity checks */
	if (node->offset < 0) {
		ret_value = -FDT_ERR_BADOFFSET;
		goto exit;
	}
	if (ScanMem8(path, AsciiStrSize(path),'/') != NULL) {
		ret_value = -FDT_ERR_BADPATH;
		goto exit;
	}

    ret_value = fdt_add_subnode((void *)node->blob, node->offset, (const char *)path);
    if (ret_value >= 0) {
		*offset = ret_value;
		ret_value = FDT_ERR_QC_NOERROR;
	}

exit:
    return ret_value;
}

/**
 * fdt_add_prop_list_u8 - Add u8 prop_list to new tree in memory
 * @node:			      node handle pointer
 * @prop_name:			  name of property to add
 * @val:			      u8 array of values to add
 * @count:                count of elements in list
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_setprop() 
 */
int fdt_add_prop_list_u8(fdt_node_handle *node, char *prop_name, uint8_t *val, uint32_t count)
{
    int ret_value;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count);
    return ret_value;
}

/**
 * fdt_add_prop_list_u16 - Add u16 prop_list to new tree in memory
 * @node:			       node handle pointer
 * @prop_name:			   name of property to add
 * @val:			       u16 array of values to add
 * @count:                 count of elements in list
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_setprop() 
 */
int fdt_add_prop_list_u16(fdt_node_handle *node, char *prop_name, uint16_t *val, uint32_t count)
{
    int ret_value;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

	/* handle endian change inline */
    for (int i=0; i<count; i++) {
        val[i] = cpu_to_fdt16(val[i]);
    }

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count*sizeof(uint16_t));
    return ret_value;
}

/**
 * fdt_add_prop_list_u32 - Add u32 prop_list to new tree in memory
 * @node:			       node handle pointer
 * @prop_name:			   name of property to add
 * @val:			       u32 array of values to add
 * @count:                 count of elements in list
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_setprop() 
 */
int fdt_add_prop_list_u32(fdt_node_handle *node, char *prop_name, uint32_t *val, uint32_t count)
{
    int ret_value;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

	/* handle endian change inline */
    for (int i=0; i<count; i++) {
        val[i] = cpu_to_fdt32(val[i]);
    }

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count*sizeof(uint32_t));
    return ret_value;
}

/**
 * fdt_add_prop_list_u64 - Add u64 prop_list to new tree in memory
 * @node:			       node handle pointer
 * @prop_name:			   name of property to add
 * @val:			       u64 array of values to add
 * @count:                 count of elements in list
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_setprop() 
 */
int fdt_add_prop_list_u64(fdt_node_handle *node, char *prop_name, uint64_t *val, uint32_t count)
{
    int ret_value;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

	/* handle endian change inline */
    for (int i=0; i<count; i++) {
        val[i] = cpu_to_fdt64(val[i]);
    }

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count*sizeof(uint64_t));
    return ret_value;
}

/**
 * fdt_add_prop_list_string - Add string prop_list to new tree in memory
 * @node:			          node handle pointer
 * @prop_name:			      name of property to add
 * @val:			          array of string values to add
 * @count:                    count of elements in list
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 * 
 * FdtLib calls:
 * 		fdt_setprop() 
 */
int fdt_add_prop_list_string(fdt_node_handle *node, char *prop_name, char *val, uint32_t count)
{
    int ret_value;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

    ret_value = fdt_setprop(((void *)node->blob), (node->offset), (prop_name), (val), count*sizeof(char));
    return ret_value;
}
