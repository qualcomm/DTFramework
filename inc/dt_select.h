// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef DT_SELECT_H
#define DT_SELECT_H

typedef struct {
    uint16_t  chip_family;
    uint16_t  chip_id;
    uint16_t  chip_maj_version ;
    uint16_t  chip_min_version;
    uint8_t   platform_type;
    uint8_t   platform_subtype;
    uint8_t   platform_maj_version;
    uint8_t   platform_min_version;
    uint32_t  oem_var;
    char      *dtb_compatible_string_starts_with;
    char      *soc_dtbo_compatible_string_starts_with;
    char      *plat_dtbo_compatible_string_starts_with;
}chip_plat_info_property;

typedef struct {
    char            *prop_name;
    unsigned int    prop_size_bytes;
    void            *value_buffer;
}property_list;

enum blob_type {
    BASE_DTB    = 0x0,
    SOC_DTBO,
    PLATFORM_DTBO,
    READ_BLOB_MAX = 0xFFFFFFFF
};

/* Functions exposed */

/**
    Function parses the raw dtb elf and selects the best base dtb, soc overlay and platform overlay blobs.
    @param[out] handle              create & return "Handle" to keep track of instance and its DT/DTBO data
    @param[in]  dtbs_image_start_address Address pointing to start of raw dtb
    @param[in]  dtbs_image_size     Size for entire dtb/dtbo elf.
                                    This argument can be set to 0x0.
    @param[in]  Input_chip_plat_info_prop Collection of Chipinfo, Platforminfo & compatible string parameters that
                                    will be required to find DTB and DTBOs
    @param[in]  Input_prop_list     Collection of user defined parameters that will be compared to find DTB and DTBOs
    @param[in]  prop_num_entries    Number of properties in Input_prop_list
    @param[out] base_dtb_size       Return size of base Device Tree Blob
    @param[out] soc_dtbo_size       Return size of SOC Device Tree Overlay
    @param[out] plat_dtbo_size      Return size of Platform Device Tree Overlay
    @return
    Returns non-zero if any operations fails, else return 0 on success
*/
int dt_select_open(
    uintptr_t               *handle,
    uintptr_t               dtbs_image_start_address,
    size_t                  dtbs_image_size,
    chip_plat_info_property *chip_plat_info_prop,
    property_list  	        *prop_list,
    unsigned int            prop_num_entries,
    size_t                  *base_dtb_size,
    size_t                  *soc_dtbo_size,
    size_t                  *plat_dtbo_size
);

/**
    Function to read the Device Tree Blob (DTB/DTBO) based on blob type and pointer.
    Based on read_blob_type, this function will read Best selected blob from within the image memory.
    If blob pointer is not Null, then this function will copy the best selected blob to that location.
    If blob pointer is pointing to NULL, then it will return base address of the Best selected blob
    @param[in]  handle              Instance of Handle data
    @param[in]  read_blob_type      Type of blob data that
    @param[in]  blob_ptr            Instance of Handle data
    @param[in]  blob_size           Instance of Handle data
    @return
    Returns -1 if any operations fails, else return 0 on success
*/
int dt_select_read_blob(
    uintptr_t       handle,
    enum blob_type  read_blob_type,
    uintptr_t       *blob_ptr,
    size_t          blob_size
);

/**
    Function to free handle data
    @param[in]  handle              Instance of Handle data
    @return
    Returns -1 if any operations fails, else return 0 on success
*/
int dt_select_close(
    uintptr_t           handle
);

#endif
