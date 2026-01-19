// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

/** @file DTBExtnLib_blob.c
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
static const void  *dtb_blob[MAX_BLOB_ID] = { NULL, NULL, NULL, NULL, NULL };

/*=========================================================================
     Globals
==========================================================================*/
Config  dtb_config = { 0, 0, 0 };

/*=========================================================================
      Public APIs
==========================================================================*/

/**
 * fdt_check_for_valid_blob - Sanity check blob pointer
 * @blob: pointer to the device tree blob
 * @bsize: size of blob
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls: fdt_check_header()
 */
int
fdt_check_for_valid_blob (
  const void  *blob,
  size_t      bsize
  )
{
  int  ret_value;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (blob);

  /* sanity check dtb blob */
  /* 1) passed in size must be >= FDT header */
  if (bsize < FDT_V17_SIZE) {
    ret_value = -FDT_ERR_NOSPACE;
    goto exit;
  }

  /* 2) use internal function to check header */
  ret_value = fdt_check_header (blob);
  if (FDT_ERR_QC_NOERROR != ret_value) {
    if (dtb_config.verbose) {
      PUTS ("blob failed fdt_check_header");
    }

    ret_value = -FDT_ERR_BADMAGIC;
    goto exit;
  }

  /* 3) Sanity check each field in header... */
  /* 3.a) confirm blob will fit into buffer (it isn't truncated) */
  if (bsize < fdt_totalsize (blob)) {
    ret_value = -FDT_ERR_NOSPACE;
    goto exit;
  }

  /* 3.b) confirm offset to structure is within bsize */
  if (bsize < fdt_off_dt_struct (blob)) {
    ret_value = -FDT_ERR_NOSPACE;
    goto exit;
  }

  /* 3.c) confirm offset to strings is within bsize */
  if (bsize < fdt_off_dt_strings (blob)) {
    ret_value = -FDT_ERR_NOSPACE;
    goto exit;
  }

  /* 3.d) confirm offset to memory reserve map is within bsize */
  if (bsize < fdt_off_mem_rsvmap (blob)) {
    ret_value = -FDT_ERR_NOSPACE;
    goto exit;
  }

  /* 3.e) confirm whole strings is within bsize */
  if (bsize < (fdt_off_dt_strings (blob)+fdt_size_dt_strings (blob))) {
    ret_value = -FDT_ERR_NOSPACE;
    goto exit;
  }

  /* 3.f) confirm whole structure is within bsize */
  if (bsize < (fdt_off_dt_struct (blob)+fdt_size_dt_struct (blob))) {
    ret_value = -FDT_ERR_NOSPACE;
    goto exit;
  }

exit:
  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d]..[%p][%ld]", __func__, ret_value, blob, bsize);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_set_blob_handle - init DTB handle to point to blob in memory
 * @blob: pointer to the device tree blob
 * @bsize: size of blob
 * @blob_id: index for requested blob
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls: fdt_check_header()
 */
int
fdt_set_blob_handle (
  const void  *blob,
  size_t      bsize,
  int         blob_id
  )
{
  int              ret_value;
  fdt_node_handle  node;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (blob);
  BLOBID_CHECK (blob_id);

  /* sanity check dtb blob */
  ret_value = fdt_check_for_valid_blob (blob, bsize);

  if (FDT_ERR_QC_NOERROR == ret_value) {
    /* blob passes checks... */
    ret_value = fdt_get_node_handle (&node, blob, "/sw/boot");
    if (FDT_ERR_QC_NOERROR == ret_value) {
      ret_value = fdt_get_uint32_prop_list (&node, "config", (void *)&dtb_config, sizeof (Config));
      if (FDT_ERR_QC_NOERROR == ret_value) {
        SNPRINTF (
                  ibuffer,
                  DTB_LINE_BUF_SIZE,
                  "DTBExtnLib config  client[%x].trace[%x].verbose[%x]",
                  dtb_config.client,
                  dtb_config.trace,
                  dtb_config.verbose
                  );
        PUTS (ibuffer);
      }
    }

    ret_value         = FDT_ERR_QC_NOERROR;
    dtb_blob[blob_id] = blob;
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d]..[%p][%ld][%d]", __func__, ret_value, blob, bsize, blob_id);
    PUTS (ibuffer);
  }

  ret_value = CreatDtbHashTable (
                                 blob_id,
                                 (void *)blob
                                 );
  return ret_value;
}

/**
 * fdt_get_blob_handle - return DTB handle
 * @blob: pointer to the device tree blob
 * @blob_id: index for requested blob
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 */
int
fdt_get_blob_handle (
  const void  **blob,
  int         blob_id
  )
{
  int  ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (blob);
  BLOBID_CHECK (blob_id);

  if (NULL == dtb_blob[blob_id]) {
    if (dtb_config.verbose) {
      SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "Requested Blob Handle is NULL [%d]", blob_id);
      PUTS (ibuffer);
    }

    ret_value = -FDT_ERR_QC_NULLPTR;
  } else {
    *blob = dtb_blob[blob_id];
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d]", __func__, ret_value);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_get_blob_size - return size of blob in memory
 * @node: pointer to fdt_node_handle object
 * @size: pointer to uint32 to hold DTB size (in bytes)
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 */
int
fdt_get_blob_size (
  fdt_node_handle  *node,
  uint32_t         *size
  )
{
  int                ret_value = FDT_ERR_QC_NOERROR;
  struct fdt_header  *fhdr;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (node->blob);
  PTR_CHECK (size);

  fhdr = (struct fdt_header *)node->blob;

  *size = fdt_totalsize (fhdr);

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d]..size[%d]", __func__, ret_value, (fdt32_t)*size);
    PUTS (ibuffer);
  }

  return ret_value;
}

/*
 * The fdt_init_root_handle APIs are included here because they
 * need access to the blob_id[] table that is static to this file.
 *
 * Logically, these functions would be included in the _driver file.
 */

/**
 * fdt_init_root_handle_for_driver - return fdt_node_handle
 * @node: pointer to fdt_node_handle object
 * @blob: pointer to the device tree blob
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 */
int
fdt_init_root_handle_for_driver (
  fdt_node_handle  *node,
  const void       *blob
  )
{
  PTR_CHECK (node);
  PTR_CHECK (blob);

  node->blob   = blob;
  node->offset = -1;

  if (dtb_config.trace) {
    PUTS ((char *)__func__);
  }

  return FDT_ERR_QC_NOERROR;
}

/**
 * fdt_init_root_handle_for_driver_by_id - return fdt_node_handle
 * @node: pointer to fdt_node_handle object
 * @blob_id: index into blob handle array
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 */
int
fdt_init_root_handle_for_driver_by_id (
  fdt_node_handle  *node,
  int              blob_id
  )
{
  PTR_CHECK (node);
  BLOBID_CHECK (blob_id);

  if (dtb_config.trace) {
    PUTS ((char *)__func__);
  }

  return fdt_init_root_handle_for_driver (node, dtb_blob[blob_id]);
}
