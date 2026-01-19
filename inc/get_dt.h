// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear


#ifndef GET_DT_H
#define GET_DT_H
#include "dt_select.h"

#define GET_DT_ERR_NONE                 0x0
#define GET_DT_ERR_INVALID_PARAMETER    0x1
#define GET_DT_ERR_HANDLE_RETURN_ERROR  0x2
#define GET_DT_ERR_INVALID_BASE_DTB_SIZE 0x3
#define GET_DT_ERR_MALLOC_FAIL          0x4
#define GET_DT_ERR_SIZE_OVERFLOW        0x5
#define GET_DT_ERR_INVALID_BASE_DTB_PTR 0x6

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
	@param[in]  prop_list	        Collection of user defined parameters that will be compared to select DTB and DTBOs
  	@param[out] prop_num_entries    Number of entries in prop_list
	@param[out] dtb_addr            Returns pointer to malloced memory where Final DTB (after selecting base DTB and
                                    applying necessary SOC and Plat overlay(s)) resides
	@param[out] dtb_size            Size of Final DTB
    @param[in]  pre_or_post         Indicates which version of get_dt() to use

	@return
    Returns -1 if any operations fails, else return 0 on success
*/
int get_dt( uintptr_t     dtbs_image_start_address,
            size_t        dtbs_image_size,
            chip_plat_info_property *chip_plat_info_prop,
            property_list *prop_list,
            unsigned int  prop_num_entries,
            uintptr_t     *dtb_addr,
            size_t        *dtb_size,
            int           pre_or_post );


/**
    Function will free "dtb_size" worth of memory located at "dtb_address".
 	@param[in]  dtb_address     Address where Final DTB resides (allocated by get_dt() function)
 	@param[in]  dtb_size        Size of the Final DTB

    @return
    Returns none
*/
void get_dt_free(uintptr_t dtb_address, unsigned int dtb_size);

#endif
