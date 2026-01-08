// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "Environment/dt_select_env.h"
#include "dt_select_internal.h"
#include "DTBExtnLib.h"
#include "dt_select.h"
#include "get_dt.h"

#define PRE  0
#define POST 1

/**
    Function internally calls dt_select_open and dt_select_read to get Base DTB and apply necessary SOC and Plat overlays.
    Before exiting, this function closes the handle by calling dt_select_close.
        @param[in]  dtbs_image_start_address  Address pointing to start of the blob which contains appended dtb(s)/dtbo(s)
        @param[in]  dtbs_image_size           Size for entire dtb/dtbo elf. This argument can be set to 0x0.
                                          DT selection algorithm goes through each DTB/DTBO(appended back to back) starting at
                                          dtbs_image_start_address, till it exhausts dtbs_image_size(if not set to 0) &&
                                          valid DTB/DTBO is found
        @param[in]  chip_plat_info_prop       Collection of Chipinfo, Platforminfo & compatible string parameters that
                                          will be required to find DTB and DTBOs
        @param[in]  prop_list           Collection of user defined parameters that will be compared to select DTB and DTBOs
        @param[out] prop_num_entries    Number of entries in prop_list
        @param[out] dtb_addr            Returns pointer to malloced memory where Final DTB (after selecting base DTB and
                                    applying necessary SOC and Plat overlay(s)) resides
        @param[out] dtb_size            Size of Final DTB
        @param[in]  pre_or_post         Indicates which version of get_dt() to use

        @return
    Returns -1 if any operations fails, else return 0 on success
*/
int
get_dt (
  uintptr_t                dtbs_image_start_address,
  size_t                   dtbs_image_size,
  chip_plat_info_property  *chip_plat_info_prop,
  property_list            *prop_list,
  unsigned int             prop_num_entries,
  uintptr_t                *dtb_addr,
  size_t                   *dtb_size,
  int                      pre_or_post
  )
{

  int        return_status    = GET_DT_ERR_NONE;
  uintptr_t  dt_select_handle = 0;
  uintptr_t  base_dtb_ptr     = 0x0;
  uintptr_t  soc_dtbo_ptr     = 0x0;
  uintptr_t  plat_dtbo_ptr    = 0x0;
  size_t     base_dtb_size    = 0;
  size_t     soc_dtbo_size    = 0;
  size_t     plat_dtbo_size   = 0;
  size_t     total_blob_size  = 0;

  if (!dtbs_image_start_address || !chip_plat_info_prop || !dtb_addr || !dtb_size || (!prop_list && prop_num_entries)) {
    /* return error */
    return_status = GET_DT_ERR_INVALID_PARAMETER;
    goto exit;
  }

  base_dtb_ptr     = 0;
  dt_select_handle = 0;
  total_blob_size  = 0;
  fdt_node_handle  node;

  node.blob     = 0x0;
  node.offset   = 0x0;
  return_status = dt_select_open (
                                  &dt_select_handle,
                                  dtbs_image_start_address,
                                  dtbs_image_size,
                                  chip_plat_info_prop,
                                  prop_list,
                                  prop_num_entries,
                                  &base_dtb_size,
                                  &soc_dtbo_size,
                                  &plat_dtbo_size
                                  );
  if (return_status) {
    goto exit;
  }

  if (!dt_select_handle) {
    /* return error */
    return_status = GET_DT_ERR_HANDLE_RETURN_ERROR;
    goto exit;
  }

  if (!base_dtb_size) {
    /* return error */
    return_status = GET_DT_ERR_INVALID_BASE_DTB_SIZE;
    goto exit;
  }

  total_blob_size = base_dtb_size + soc_dtbo_size;
  if ((total_blob_size < soc_dtbo_size) || ((total_blob_size + plat_dtbo_size) < plat_dtbo_size)) {
    return_status = GET_DT_ERR_SIZE_OVERFLOW;
    goto exit;
  }

  total_blob_size += plat_dtbo_size;

  if (pre_or_post == POST) {
    /* Allocate enough memory to apply all overlay(s) to Base DT */
    base_dtb_ptr = (uintptr_t)MALLOC_WRAPPER (total_blob_size);
    if (!base_dtb_ptr) {
      /* return error */
      return_status = GET_DT_ERR_MALLOC_FAIL;
      goto exit;
    }
  } else {
    base_dtb_ptr = 0x0;
  }

  /* Read Base DTB */
  return_status = dt_select_read_blob (
                                       dt_select_handle,
                                       BASE_DTB,
                                       &base_dtb_ptr,
                                       base_dtb_size
                                       );
  if (return_status) {
    /* return error */
    goto exit;
  }

  if (soc_dtbo_size) {
    /* Handle Chipset/SOC Overlay */
    /* Allocate memory to store SOC dtbo */
    soc_dtbo_ptr = (uintptr_t)MALLOC_WRAPPER (soc_dtbo_size);
    if (!soc_dtbo_ptr) {
      /* return error */
      return_status = GET_DT_ERR_MALLOC_FAIL;
      goto exit;
    }

    /* Read Base DTB */
    return_status = dt_select_read_blob (
                                         dt_select_handle,
                                         SOC_DTBO,
                                         &soc_dtbo_ptr,
                                         soc_dtbo_size
                                         );
    if (return_status) {
      /* return error */
      goto exit;
    }
  }

  if (plat_dtbo_size) {
    /* Handle Platform Overlay */
    /* Allocate memory to store Platform dtbo */
    plat_dtbo_ptr = (uintptr_t)MALLOC_WRAPPER (plat_dtbo_size);
    if (!plat_dtbo_ptr) {
      /* return error */
      return_status = GET_DT_ERR_MALLOC_FAIL;
      goto exit;
    }

    /* Read Base DTB */
    return_status = dt_select_read_blob (
                                         dt_select_handle,
                                         PLATFORM_DTBO,
                                         &plat_dtbo_ptr,
                                         plat_dtbo_size
                                         );
    if (return_status) {
      /* return error */
      goto exit;
    }
  }

  if (pre_or_post == PRE && dtbs_image_start_address != base_dtb_ptr) {
    if (base_dtb_ptr < dtbs_image_start_address) {
      return_status = GET_DT_ERR_INVALID_BASE_DTB_PTR;
      goto exit;
    }
    base_dtb_size = MEMCPY_WRAPPER((void *)dtbs_image_start_address, dtbs_image_size, (const void *)base_dtb_ptr, base_dtb_size);
  }

  if (soc_dtbo_size) {
    /* Apply SOC DTB Overlay */
    return_status = fdt_merge_overlay ((const void *)base_dtb_ptr, base_dtb_size, (void *)soc_dtbo_ptr, soc_dtbo_size, (void *)base_dtb_ptr, base_dtb_size + soc_dtbo_size);
    if (return_status) {
      /* return error */
      goto exit;
    }

    /* Need to update size for base DTB before we apply Platform overlay (if applied) */
    node.blob     = (const void *)base_dtb_ptr;
    return_status = fdt_get_blob_size (&node, (uint32_t *)&base_dtb_size);
    if (return_status) {
      goto exit;
    }

    FREE_WRAPPER ((void *)soc_dtbo_ptr, soc_dtbo_size);
    soc_dtbo_ptr = (uintptr_t)NULL;
  }

  if (plat_dtbo_size) {
    /* Apply Platform DTB Overlay */
    return_status = fdt_merge_overlay ((const void *)base_dtb_ptr, base_dtb_size, (void *)plat_dtbo_ptr, plat_dtbo_size, (void *)base_dtb_ptr, total_blob_size);
    if (return_status) {
      /* return error */
      goto exit;
    }

    FREE_WRAPPER ((void *)plat_dtbo_ptr, plat_dtbo_size);
    plat_dtbo_ptr = (uintptr_t)NULL;
  }

exit:
  if (return_status) {
    /* If error, then free all buffers */
    if (base_dtb_ptr) {
      FREE_WRAPPER ((void *)base_dtb_ptr, total_blob_size);
    }

    if (soc_dtbo_ptr) {
      FREE_WRAPPER ((void *)soc_dtbo_ptr, soc_dtbo_size);
    }

    if (plat_dtbo_ptr) {
      FREE_WRAPPER ((void *)plat_dtbo_ptr, plat_dtbo_size);
    }

    base_dtb_ptr  = (uintptr_t)NULL;
    soc_dtbo_ptr  = (uintptr_t)NULL;
    plat_dtbo_ptr = (uintptr_t)NULL;
  } else {
    /* Return pointer & size of Base DT on success */
    *dtb_addr = base_dtb_ptr;
    *dtb_size = total_blob_size;
  }

  /* Close dt_select handle before exiting */
  dt_select_close (dt_select_handle);
  dt_select_handle = (uintptr_t)NULL;
  return return_status;
}

void
get_dt_free (
  uintptr_t     dtb_address,
  unsigned int  dtb_size
  )
{
  if (dtb_address && dtb_size) {
    FREE_WRAPPER ((void *)dtb_address, dtb_size);
  }
}
