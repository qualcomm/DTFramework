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
      Internal helpers
==========================================================================*/

/**
 * is_registered_blob - check whether @blob is registered in the blob table
 *
 * Returns 1 if @blob matches any registered blob_id, 0 otherwise.
 * Used to guard hash-table updates: skip if the blob has no hash table.
 */
static int
is_registered_blob (
  const void  *blob
  )
{
  const void  *registered;
  int          i;

  for (i = 0; i < DTB_HASH_CTX_MAX_BLOB; i++) {
    if ((fdt_get_blob_handle (&registered, i) == FDT_ERR_QC_NOERROR) &&
        (registered == blob))
    {
      return 1;
    }
  }

  return 0;
}

/**
 * collect_prop_change_info - capture splice_offset and delta before fdt_setprop
 * @blob:              blob to query
 * @nodeoffset:        node offset within @blob
 * @prop_name:         property name
 * @new_len:           byte length of the new property value
 * @out_splice_offset: set to the byte offset of the old value data in @blob;
 *                     0 if property does not exist or blob is not registered
 * @out_delta:         set to FDT_TAGALIGN(new_len) - FDT_TAGALIGN(old_len);
 *                     0 if property does not exist or blob is not registered
 *
 * Must be called BEFORE fdt_setprop() so the old property location is valid.
 */
static void
collect_prop_change_info (
  const void  *blob,
  int         nodeoffset,
  const char  *prop_name,
  int         new_len,
  int         *out_splice_offset,
  int         *out_delta
  )
{
  const void  *old_data;
  int          old_len;

  *out_splice_offset = 0;
  *out_delta         = 0;

  if (!is_registered_blob (blob)) {
    return;
  }

  old_data = fdt_getprop (blob, nodeoffset, prop_name, &old_len);
  if (old_data == NULL) {
    return;  /* new property — incremental update not applicable */
  }

  *out_splice_offset = (int)((const char *)old_data - (const char *)blob);
  *out_delta         = FDT_TAGALIGN (new_len) - FDT_TAGALIGN (old_len);
}

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
    int  ret_value;
    int  old_struct_size = 0;

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

    /* capture struct size before insertion to compute delta */
    if (is_registered_blob (node->blob)) {
        old_struct_size = fdt_size_dt_struct (node->blob);
    }

    ret_value = fdt_add_subnode((void *)node->blob, node->offset, (const char *)path);
    if (ret_value >= 0) {
        *offset   = ret_value;
        ret_value = FDT_ERR_QC_NOERROR;

        if (old_struct_size != 0) {
            int  delta = fdt_size_dt_struct (node->blob) - old_struct_size;
            if (delta != 0) {
                /* *offset is the new node's position; shift all existing
                   hash_node entries that follow it in the structure block */
                fdt_adjust_hash_offsets (node->blob, *offset, delta);
            }
        }
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
    int  ret_value;
    int  splice_offset;
    int  delta;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

    collect_prop_change_info (node->blob, node->offset, prop_name,
                              (int)count, &splice_offset, &delta);

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count);

    if ((ret_value == 0) && (delta != 0)) {
        fdt_adjust_hash_offsets (node->blob, splice_offset, delta);
    }

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
    int  ret_value;
    int  splice_offset;
    int  delta;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

    collect_prop_change_info (node->blob, node->offset, prop_name,
                              (int)(count * sizeof (uint16_t)), &splice_offset, &delta);

	/* handle endian change inline */
    for (int i=0; i<count; i++) {
        val[i] = cpu_to_fdt16(val[i]);
    }

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count*sizeof(uint16_t));

    if ((ret_value == 0) && (delta != 0)) {
        fdt_adjust_hash_offsets (node->blob, splice_offset, delta);
    }

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
    int  ret_value;
    int  splice_offset;
    int  delta;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

    collect_prop_change_info (node->blob, node->offset, prop_name,
                              (int)(count * sizeof (uint32_t)), &splice_offset, &delta);

	/* handle endian change inline */
    for (int i=0; i<count; i++) {
        val[i] = cpu_to_fdt32(val[i]);
    }

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count*sizeof(uint32_t));

    if ((ret_value == 0) && (delta != 0)) {
        fdt_adjust_hash_offsets (node->blob, splice_offset, delta);
    }

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
    int  ret_value;
    int  splice_offset;
    int  delta;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

    collect_prop_change_info (node->blob, node->offset, prop_name,
                              (int)(count * sizeof (uint64_t)), &splice_offset, &delta);

	/* handle endian change inline */
    for (int i=0; i<count; i++) {
        val[i] = cpu_to_fdt64(val[i]);
    }

    ret_value = fdt_setprop((void *)node->blob, node->offset, prop_name, val, count*sizeof(uint64_t));

    if ((ret_value == 0) && (delta != 0)) {
        fdt_adjust_hash_offsets (node->blob, splice_offset, delta);
    }

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
    int  ret_value;
    int  splice_offset;
    int  delta;

	PTR_CHECK(node);
	PTR_CHECK(prop_name);
	PTR_CHECK(val);

    collect_prop_change_info (node->blob, node->offset, prop_name,
                              (int)(count * sizeof (char)), &splice_offset, &delta);

    ret_value = fdt_setprop(((void *)node->blob), (node->offset), (prop_name), (val), count*sizeof(char));

    if ((ret_value == 0) && (delta != 0)) {
        fdt_adjust_hash_offsets (node->blob, splice_offset, delta);
    }

    return ret_value;
}
