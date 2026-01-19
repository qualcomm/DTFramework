// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear


#include "dt_select_internal.h"
char  buffer[BUFFER_SIZE];

#ifdef DT_SELECT_LOGGING_ENABLE
unsigned int  blob_index = 1;
#endif

static int
_handle_check (
  handle_data *
  );

static int
_select_dtb_and_dtbos (
  handle_data *,
  handle_input_data *
  );

static int
_is_fdtheader_at_end_of_image (
  uintptr_t,
  handle_data *,
  handle_input_data *,
  handle_entry_data *,
  bool *
  );

static int
_address_within_image_range_and_pointer_overflow_check (
  uintptr_t,
  uintptr_t,
  size_t,
  bool *
  );

static int
_is_it_dtb_dtbo_candidate (
  handle_entry_data *,
  handle_input_data *
  );

static int
_is_chipinfo_family_match (
  handle_entry_data *,
  handle_input_data *
  );

static int
_is_compatible_string_match (
  handle_entry_data *,
  handle_input_data *
  );

static int
_is_property_list_valid (
  handle_entry_data *,
  handle_input_data *
  );

static int
_validate_and_select_platform_dtbo (
  handle_entry_data *,
  handle_input_data *,
  handle_selected_dtbo *
  );

static int
_validate_and_select_base_dtb (
  handle_entry_data *,
  handle_data *
  );

static int
_validate_and_select_soc_dtbo (
  handle_entry_data *,
  handle_input_data *,
  handle_selected_dtbo *
  );

static int
_platform_dtbo_version_check (
  handle_entry_data *,
  handle_input_data *,
  handle_selected_dtbo *
  );

static int
_validate_oem_variant (
  handle_entry_data *,
  handle_input_data *
  );

static int
_init_handle_data (
  handle_data *,
  uintptr_t
  );

static int
_init_chipinfo (
  uint16_t *
  );

static int
_init_platinfo (
  uint8_t *
  );

static int
_init_fdt_node_handle (
  fdt_node_handle  *node
  );

static int
_init_handle_entry_data (
  handle_entry_data *
  );

static int
_init_handle_input_data (
  size_t,
  handle_input_data *,
  chip_plat_info_property *,
  property_list *,
  unsigned
  );

static int
_init_handle_selected_dtb_dtbo (
  handle_selected_dtbo *
  );

static int
_print_input_parameters (
  chip_plat_info_property *,
  property_list *,
  unsigned
  );

static char *
_get_size_tag (
  uint32_t
  );

static int
_print_input_parameters (
  chip_plat_info_property *,
  property_list *,
  unsigned
  );

static int
_print_property_value (
  fdt_node_handle *,
  char *,
  char *,
  int,
  uint32_t,
  uint32_t,
  bool
  );

static int
_print_board_id (
  uintptr_t,
  property_list *,
  unsigned
  );

static bool
_is_safe_to_add_pointer (
  const uintptr_t  pointer,
  uint32_t         offset
  )
{
  return (pointer + offset) < pointer ? UNSAFE : SAFE;
}

static bool
_is_safe_to_subtract_uint32 (
  const uint32_t  a,
  const uint32_t  b
  )
{
  /* Based on arithmetice operation result = a - b */
  return (a < b) ? UNSAFE : SAFE;
}

/*
  Function that initializes handle_data structure variables
*/
static int
_init_handle_data (
  handle_data  *handle_ptr,
  uintptr_t    dtbs_image_start_address
  )
{
  if (!handle_ptr) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Guard data to check the sanity of local handle_data */
  handle_ptr->data_guard               = DT_SELECT_LIBRARY_ID;
  handle_ptr->dtbs_image_start_address = dtbs_image_start_address;
  handle_ptr->base_dtb_ptr             = 0x0;
  handle_ptr->base_dtb_size            = 0x0;
  handle_ptr->soc_dtbo_ptr             = 0x0;
  handle_ptr->soc_dtbo_size            = 0x0;
  handle_ptr->plat_dtbo_ptr            = 0x0;
  handle_ptr->plat_dtbo_size           = 0x0;
  return SUCCESS;
}

/*
  Function that initializes chipinfo array variables
*/
static int
_init_chipinfo (
  uint16_t  *chipinfo_selected
  )
{
  if (!chipinfo_selected) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  chipinfo_selected[CHIPINFO_FAMILY] = 0;
  chipinfo_selected[CHIPINFO_ID]     = 0;
  chipinfo_selected[CHIPINFO_MAJOR]  = 0;
  chipinfo_selected[CHIPINFO_MINOR]  = 0;
  return SUCCESS;
}

/*
  Function that initializes platinfo array variables
*/
static int
_init_platinfo (
  uint8_t  *platforminfo_selected
  )
{
  if (!platforminfo_selected) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return FAIL;
  }

  platforminfo_selected[PLATFORMINFO_TYPE]     = 0;
  platforminfo_selected[PLATFORMINFO_SUB_TYPE] = 0;
  platforminfo_selected[PLATFORMINFO_MAJOR]    = 0;
  platforminfo_selected[PLATFORMINFO_MINOR]    = 0;
  return SUCCESS;
}

/*
  Function that initializes fdt_node_handle structure variables
*/
static int
_init_fdt_node_handle (
  fdt_node_handle  *node
  )
{
  if (!node) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  node->blob   = NULL;
  node->offset = 0;
  return SUCCESS;
}

/*
  Function that initializes handle_entry_data structure variables
*/
static int
_init_handle_entry_data (
  handle_entry_data  *current_entry_data
  )
{
  if (!current_entry_data) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  int  return_status = _init_fdt_node_handle (&(current_entry_data->node));

  if (SUCCESS != return_status) {
    return return_status;
  }

  current_entry_data->device_tree_blob_size = 0;
  current_entry_data->device_tree_type      = BASE_DTB;

  return_status = _init_chipinfo (current_entry_data->DT_chipinfo_selected);
  if (SUCCESS != return_status) {
    return return_status;
  }

  return_status = _init_platinfo (current_entry_data->DT_platforminfo_selected);
  if (SUCCESS != return_status) {
    return return_status;
  }

  current_entry_data->rank                      = -1;
  current_entry_data->candidacy_status          = INVALID_CANDIDATE;
  current_entry_data->chipinfo_version_calc     = 0;
  current_entry_data->platforminfo_version_calc = 0;
  return SUCCESS;
}

/*
  Function that initializes handle_input_data structure variables based on Input parameters that
  client has provided
*/
static int
_init_handle_input_data (
  size_t                   dtbs_image_size,
  handle_input_data        *input_parameters,
  chip_plat_info_property  *Input_chip_plat_info_prop,
  property_list            *Input_prop_list,
  unsigned                 prop_num_entries
  )
{
  int  return_status = INVALID_PARAMETER;

  if (!input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return return_status;
  }

  /* If Property List pointer is Null or Number of properties is 0x0,
     then mark it as empty */
  if (!Input_prop_list || !prop_num_entries) {
    Input_prop_list  = NULL;
    prop_num_entries = 0x0;
  }

  input_parameters->dtbs_image_size           = dtbs_image_size;
  input_parameters->Input_chip_plat_info_prop = Input_chip_plat_info_prop;
  input_parameters->Input_prop_list           = Input_prop_list;
  input_parameters->prop_num_entries          = prop_num_entries;

  /* Calculate Integer form of Chipinfo and Platforminfo Versions (Major version and Minor version) */
  input_parameters->chipinfo_version_calc = CALCULATE_CHIP_PLAT_INFO_VERSION (
                                                                              Input_chip_plat_info_prop->chip_maj_version,
                                                                              Input_chip_plat_info_prop->chip_min_version
                                                                              );
  input_parameters->platforminfo_version_calc = CALCULATE_CHIP_PLAT_INFO_VERSION (
                                                                                  Input_chip_plat_info_prop->platform_maj_version,
                                                                                  Input_chip_plat_info_prop->platform_min_version
                                                                                  );
  return_status = _print_input_parameters (
                                           Input_chip_plat_info_prop,
                                           Input_prop_list,
                                           prop_num_entries
                                           );
  return return_status;
}

static char *
_get_size_tag (
  uint32_t  property_datatype_size
  )
{
  switch (property_datatype_size) {
    case BYTE_SIZE_BYTES:
      return BYTE_SIZE_TAG;
      break;
    case WORD_SIZE_BYTES:
      return WORD_SIZE_TAG;
      break;
    case DWORD_SIZE_BYTES:
      return DWORD_SIZE_TAG;
      break;
    case QWORD_SIZE_BYTES:
      return QWORD_SIZE_TAG;
      break;
    default:
      return BYTE_SIZE_TAG;
  }

  return BYTE_SIZE_TAG;
}

static int
_print_input_parameters (
  chip_plat_info_property  *Input_chip_plat_info_prop,
  property_list            *Input_prop_list,
  unsigned                 prop_num_entries
  )
{
  int   len           = 0x0;
  char  *print_buffer = NULL;

  if (!Input_chip_plat_info_prop || (!Input_prop_list && prop_num_entries) || (Input_prop_list && !prop_num_entries)) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s", __func__);
    return INVALID_PARAMETER;
  }

  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Input parameters for Device Tree Selection\n");
  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "%s {\n", ROOT_NODE_NAME);
  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  /* %s = %s <<Family> <ID> <Major> <MINOR>>; */\n", CHIPINFO_PROP_NAME, _get_size_tag (SINGLE_CHIPINFO_ELEMENT_SIZE_BYTES));
  DEFAULT_LOG_MESSAGE (
                       buffer,
                       BUFFER_SIZE,
                       "  %s = %s <0x%x 0x%x 0x%x 0x%x>;\n",
                       CHIPINFO_PROP_NAME,
                       _get_size_tag (SINGLE_CHIPINFO_ELEMENT_SIZE_BYTES),
                       Input_chip_plat_info_prop->chip_family,
                       Input_chip_plat_info_prop->chip_id,
                       Input_chip_plat_info_prop->chip_maj_version,
                       Input_chip_plat_info_prop->chip_min_version
                       );
  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  /* %s = %s <<Type> <Subtype> <Major> <MINOR>> */\n", PLATFORMINFO_PROP_NAME, _get_size_tag (SINGLE_PLATFORMINFO_ELEMENT_SIZE_BYTES));
  DEFAULT_LOG_MESSAGE (
                       buffer,
                       BUFFER_SIZE,
                       "  %s = %s <0x%x 0x%x 0x%x 0x%x>;\n",
                       PLATFORMINFO_PROP_NAME,
                       _get_size_tag (SINGLE_PLATFORMINFO_ELEMENT_SIZE_BYTES),
                       Input_chip_plat_info_prop->platform_type,
                       Input_chip_plat_info_prop->platform_subtype,
                       Input_chip_plat_info_prop->platform_maj_version,
                       Input_chip_plat_info_prop->platform_min_version
                       );

  DEFAULT_LOG_MESSAGE (
                       buffer,
                       BUFFER_SIZE,
                       "  %s = %s < 0x%x >;\n",
                       OEM_VAR_PROP_NAME,
                       _get_size_tag (sizeof (Input_chip_plat_info_prop->oem_var)),
                       Input_chip_plat_info_prop->oem_var
                       );
  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  /* %s string starts with: */\n", COMPATIBLE_PROP_NAME);
  if(Input_chip_plat_info_prop->dtb_compatible_string_starts_with != NULL && strlen(Input_chip_plat_info_prop->dtb_compatible_string_starts_with) != 0x0)
	DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  [for Base DTB] %s = \"%s\";\n", COMPATIBLE_PROP_NAME, Input_chip_plat_info_prop->dtb_compatible_string_starts_with);
  else
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  [for Base DTB] %s = \"\";\n", COMPATIBLE_PROP_NAME);

  if(Input_chip_plat_info_prop->soc_dtbo_compatible_string_starts_with != NULL && strlen(Input_chip_plat_info_prop->soc_dtbo_compatible_string_starts_with) != 0x0)
	DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  [for SOC DTBO] %s = \"%s\";\n", COMPATIBLE_PROP_NAME, Input_chip_plat_info_prop->soc_dtbo_compatible_string_starts_with);
  else
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  [for SOC DTBO] %s = \"\";\n", COMPATIBLE_PROP_NAME);

  if(Input_chip_plat_info_prop->plat_dtbo_compatible_string_starts_with != NULL && strlen(Input_chip_plat_info_prop->plat_dtbo_compatible_string_starts_with) != 0x0)
	DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  [for Platform <Board> DTBO] %s = \"%s\";\n", COMPATIBLE_PROP_NAME, Input_chip_plat_info_prop->plat_dtbo_compatible_string_starts_with);
  else
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  [for Platform <Board> DTBO] %s = \"\";\n", COMPATIBLE_PROP_NAME);

  if (Input_prop_list) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "  /* Property List */\n");
    print_buffer = (char *)MALLOC_WRAPPER (BUFFER_SIZE);
    for (int index = 0; index < prop_num_entries; index++) {
      len = 0x0;
      len = SNPRINTF_WRAPPER (print_buffer, BUFFER_SIZE, "    %s", Input_prop_list[index].prop_name);
      switch (Input_prop_list[index].prop_size_bytes) {
        case BYTE_SIZE_BYTES:
          len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, " = %s < 0x%x >;\n", _get_size_tag (BYTE_SIZE_BYTES), *((uint8_t *)Input_prop_list[index].value_buffer));
          break;
        case WORD_SIZE_BYTES:
          len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, " = %s < 0x%x >;\n", _get_size_tag (WORD_SIZE_BYTES), *((uint16_t *)Input_prop_list[index].value_buffer));
          break;
        case DWORD_SIZE_BYTES:
          len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, " = %s < 0x%x >;\n", _get_size_tag (DWORD_SIZE_BYTES), *((uint32_t *)Input_prop_list[index].value_buffer));
          break;
        case QWORD_SIZE_BYTES:
          len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, " = %s < 0x%x >;\n", _get_size_tag (QWORD_SIZE_BYTES), *((uint64_t *)Input_prop_list[index].value_buffer));
          break;
        default:
          DEFAULT_LOG_MESSAGE (print_buffer, BUFFER_SIZE, " = <Unsupported size>\n");
      }

      DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "%s", print_buffer);
    }

    if (NULL != print_buffer) {
      FREE_WRAPPER (print_buffer, BUFFER_SIZE);
    }
  }

  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "};\n");
  return SUCCESS;
}

static int
_print_property_value (
  fdt_node_handle  *node,
  char             *current_string_ptr,
  char             *print_buffer,
  int              len,
  uint32_t         property_value_buffer_size,
  uint32_t         property_datatype_size,
  bool             is_proc_name_property
  )
{
  const void  *vptr         = NULL;
  int         return_status = INVALID_PARAMETER;
  uint8_t     *current_ptr  = NULL;
  int         current_len   = 0x0;

  if ((NULL == node) || !node->blob || (NULL == current_string_ptr)) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  vptr = fdt_getprop (node->blob, node->offset, current_string_ptr, (int *)&property_value_buffer_size);
  if (!vptr) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "FDT Get prop returning Null pointer\n");
    return INVALID_RETURN_PTR;
  }

  current_ptr = (uint8_t *)vptr;

  /* "proc-name" needs to be printed differently than other integer value properties */
  if (is_proc_name_property) {
    while (property_value_buffer_size) {
      current_len  = strlen ((char *)current_ptr);
      current_len += 1;     /* Consider NULL character into length */

      /* check for "wrap around" of unsigned int */
      if (SAFE != _is_safe_to_subtract_uint32 (property_value_buffer_size, current_len)) {
        return_status = UINT32_ARITHMETIC_OVERFLOW;
        goto EXIT_POINT (_print_property_value);
      }

      property_value_buffer_size -= current_len;

      len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, "\"%s\"", (char *)current_ptr);

      /* Check for pointer overflow before we increment size to current_string_ptr pointer */
      if (SAFE != _is_safe_to_add_pointer ((uintptr_t)current_ptr, (current_len * sizeof (*current_ptr)))) {
        return_status = POINTER_ARITHMETIC_OVERFLOW;
        goto EXIT_POINT (_print_property_value);
      }

      current_ptr = current_ptr + current_len;
      len        += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, "%s", (property_value_buffer_size == 0x0 ? ";" : ","));
    }

    return_status = SUCCESS;
  } else {
    len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, "<");
    while (property_value_buffer_size) {
      len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, " 0x");
      for (uint32_t index = 0; index < property_datatype_size; index++) {
        len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, "%02X", *current_ptr);
        current_ptr++;
      }

      if (SAFE != _is_safe_to_subtract_uint32 (property_value_buffer_size, property_datatype_size)) {
        return_status = UINT32_ARITHMETIC_OVERFLOW;
        goto EXIT_POINT (_print_property_value);
      }

      property_value_buffer_size -= property_datatype_size;
    }

    len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, " >;");
  }

  return_status = SUCCESS;
  EXIT_POINT (_print_property_value) :
    return return_status;
}

static int
_print_board_id (
  uintptr_t      fdtheader,
  property_list  *Input_prop_list,
  unsigned       prop_num_entries
  )
{
  uint32_t         DT_board_id_node_property_name_size = 0x0;
  uint32_t         property_name_string_buffer_size    = 0x0;
  uint32_t         property_value_buffer_size          = 0x0;
  char             *DT_board_id_node_property_names    = NULL;
  char             *current_string_ptr                 = NULL;
  char             *property_value_buffer_ptr          = NULL;
  int              len                                 = 0x0;
  char             *print_buffer                       = NULL;
  fdt_node_handle  node;

  if (0x0 == fdtheader) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* If Property List pointer is Null or Number of properties is 0x0,
     then mark it as empty */
  if (!Input_prop_list || !prop_num_entries) {
    Input_prop_list  = NULL;
    prop_num_entries = 0x0;
  }

  _init_fdt_node_handle (&node);
  node.blob = (const void *)fdtheader;

  /* Get board-id node from the current dtbo blob before ROOT node properties are printed */
  int  return_status = fdt_get_node_handle (&node, node.blob, ROOT_NODE);

  if ((FDT_ERR_QC_NOERROR != return_status) || (NULL == node.blob) || (NULL == node.blob)) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unable to fetch %s Node! Skip printing for this DTB/DTBO\n", ROOT_NODE_NAME);
    return SUCCESS;
  }

  /* Fetch the size for all the property names */
  return_status = fdt_get_prop_names_size_of_node (&node, &DT_board_id_node_property_name_size);
  if (FDT_ERR_QC_NOERROR != return_status) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\": Unable to fetch the node\n", ROOT_NODE);
    goto EXIT_POINT (_print_board_id);
  }

  DT_board_id_node_property_names = (char *)MALLOC_WRAPPER (DT_board_id_node_property_name_size);
  if (!DT_board_id_node_property_names) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memory allocation failed\n");
    return MALLOC_FAIL;
  }

  return_status = fdt_get_prop_names_of_node (&node, DT_board_id_node_property_names, DT_board_id_node_property_name_size);
  if (FDT_ERR_QC_NOERROR != return_status) {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\": Unable to fetch the node\n", ROOT_NODE);
    goto EXIT_POINT (_print_board_id);
  }

  property_name_string_buffer_size = DT_board_id_node_property_name_size;
  current_string_ptr               = DT_board_id_node_property_names;
  print_buffer                     = (char *)MALLOC_WRAPPER (BUFFER_SIZE);
  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "%s {\n", ROOT_NODE_NAME);

  /* Fetch and print values for every property names within board-id node */
  while (property_name_string_buffer_size > 0) {
    property_value_buffer_size = 0x0;

    /* Add 0x1 for Null character */
    int  size = strlen (current_string_ptr) + 0x1;
    /* if no datatype size is provided for this Property, mark it as 1 Byte i.e. similar to "/bits/ 8" */
    uint32_t  property_datatype_size = 0x1;

    /* check for "wrap around" of unsigned int */
    if (SAFE != _is_safe_to_subtract_uint32 (property_name_string_buffer_size, size)) {
      return UINT32_ARITHMETIC_OVERFLOW;
    }

    property_name_string_buffer_size -= size;

    len = SNPRINTF_WRAPPER (print_buffer, BUFFER_SIZE, "  %s ", current_string_ptr);

    return_status = fdt_get_prop_size (&node, current_string_ptr, &property_value_buffer_size);
    if (FDT_ERR_QC_NOERROR != return_status) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unable to fetch \"%s\"! Moving to next Blob\n", current_string_ptr);
      goto EXIT_POINT (_print_board_id);
    }

    if (property_value_buffer_size == 0x0) {
      /* Boolean property */
      /* Print Property Name */
      len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, "; /* True [boolean property] */");
    } else {
      len                      += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, "= ", current_string_ptr);
      property_value_buffer_ptr = (char *)MALLOC_WRAPPER (property_value_buffer_size);
      if (0x0 == strncmp (current_string_ptr, CHIPINFO_PROP_NAME, strlen (CHIPINFO_PROP_NAME))) {
        property_datatype_size = SINGLE_CHIPINFO_ELEMENT_SIZE_BYTES;
      } else if (0x0 == strncmp (current_string_ptr, PLATFORMINFO_PROP_NAME, strlen (PLATFORMINFO_PROP_NAME))) {
        property_datatype_size = SINGLE_PLATFORMINFO_ELEMENT_SIZE_BYTES;
      } else if (0x0 == strncmp (current_string_ptr, COMPATIBLE_PROP_NAME, strlen (COMPATIBLE_PROP_NAME))) {
        /* Mark this property entry as proc-name string */
        property_datatype_size = -1;
      } else if (0x0 == strncmp (current_string_ptr, OEM_VAR_PROP_NAME, strlen (OEM_VAR_PROP_NAME))) {
        property_datatype_size = SINGLE_OEM_VARIANT_ELEMENT_SIZE_BYTES;
      } else {
        /* For non "chip-info" and non "board-info" properties, look for datatype size in the property list */
        for (uint32_t index = 0; index < prop_num_entries; index++) {
          char  *property_name_ptr = Input_prop_list[index].prop_name;
          if (0x0 == strncmp (current_string_ptr, property_name_ptr, strlen (property_name_ptr))) {
            property_datatype_size = Input_prop_list[index].prop_size_bytes;
            break;
          }
        }
      }

      /* Print Property size tag. No size tag required for string i.e. "proc-name" */
      if (-1 != property_datatype_size) {
        /* Print Property size tag */
        len += SNPRINTF_WRAPPER (print_buffer + len, BUFFER_SIZE - len, "%s ", _get_size_tag (property_datatype_size));
      }

      return_status = _print_property_value (
                                             &node,
                                             current_string_ptr,
                                             print_buffer,
                                             len,
                                             property_value_buffer_size,
                                             property_datatype_size,
                                             (property_datatype_size == -1) ? true : false
                                             );
      if (SUCCESS != return_status) {
        goto EXIT_POINT (_print_board_id);
      }

      FREE_WRAPPER (property_value_buffer_ptr, property_value_buffer_size);
      property_value_buffer_ptr = NULL;
    }

    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "%s\n", print_buffer);

    /* Check for pointer overflow before we increment size to current_string_ptr pointer */
    if (SAFE != _is_safe_to_add_pointer ((uintptr_t)current_string_ptr, (size * sizeof (*current_string_ptr)))) {
      return POINTER_ARITHMETIC_OVERFLOW;
    }

    current_string_ptr += size;
  }

  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "};\n");

  EXIT_POINT (_print_board_id) :
    if (NULL != DT_board_id_node_property_names) {
    FREE_WRAPPER (DT_board_id_node_property_names, DT_board_id_node_property_name_size);
    DT_board_id_node_property_names = NULL;
  }

  if (NULL != property_value_buffer_ptr) {
    FREE_WRAPPER (property_value_buffer_ptr, property_value_buffer_size);
    property_value_buffer_ptr = NULL;
  }

  if (NULL != print_buffer) {
    FREE_WRAPPER (print_buffer, BUFFER_SIZE);
    print_buffer = NULL;
  }

  return return_status;
}

static int
_print_selected_dtb_dtbos (
  handle_data        *handle_ptr,
  handle_input_data  *input_parameters
  )
{
  int  return_status = INVALID_PARAMETER;

  if (NULL == handle_ptr) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return return_status;
  }

  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" Node for selected entries:\n", ROOT_NODE_NAME);
  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "1. Base DTB:\n");
  if ((0x0 != handle_ptr->base_dtb_ptr) && (0x0 != handle_ptr->base_dtb_size)) {
    return_status = _print_board_id (
                                     handle_ptr->base_dtb_ptr,
                                     input_parameters->Input_prop_list,
                                     input_parameters->prop_num_entries
                                     );
    if (SUCCESS != return_status) {
      return return_status;
    }
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Info: BASE DTB selected address: %p size: 0x%X\n", (long unsigned int)handle_ptr->base_dtb_ptr, handle_ptr->base_dtb_size);
  }
  else
  {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Entry Not Selected\n");
  }

  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "2. SOC (Chip) DTBO:\n");
  if ((0x0 != handle_ptr->soc_dtbo_ptr) && (0x0 != handle_ptr->soc_dtbo_size)) {
    return_status = _print_board_id (
                                     handle_ptr->soc_dtbo_ptr,
                                     input_parameters->Input_prop_list,
                                     input_parameters->prop_num_entries
                                     );
    if (SUCCESS != return_status) {
      return return_status;
    }
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Info: SOC DTBO selected address: %p size: 0x%X\n", (long unsigned int)handle_ptr->soc_dtbo_ptr, handle_ptr->soc_dtbo_size);
  }
  else
  {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Entry Not Selected\n");
  }

  DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "3. Platform (Board) DTBO:\n");
  if ((0x0 != handle_ptr->plat_dtbo_ptr) && (0x0 != handle_ptr->plat_dtbo_size)) {
    return_status = _print_board_id (
                                     handle_ptr->plat_dtbo_ptr,
                                     input_parameters->Input_prop_list,
                                     input_parameters->prop_num_entries
                                     );
    if (SUCCESS != return_status) {
      return return_status;
    }
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Info: Platform (Board) DTBO selected address: %p size: 0x%X\n", (long unsigned int)handle_ptr->plat_dtbo_ptr, handle_ptr->plat_dtbo_size);
  }
  else
  {
    DEFAULT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Entry Not Selected\n");
  }

  return return_status;
}

/*
  Function that initializes handle_selected_dtbo structure variables
*/
static int
_init_handle_selected_dtb_dtbo (
  handle_selected_dtbo  *selected_dtb_dtbo
  )
{
  if (!selected_dtb_dtbo) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  selected_dtb_dtbo->rank                      = -1;
  selected_dtb_dtbo->device_tree_blob_size     = 0x0;
  selected_dtb_dtbo->chipinfo_version_calc     = 0x0;
  selected_dtb_dtbo->platforminfo_version_calc = 0x0;
  return SUCCESS;
}

/*
  Function that checks for data corruption in data_guard. Returns FAIL if corruption found
*/
static int
_handle_check (
  handle_data  *handle
  )
{
  if (!handle || (DT_SELECT_LIBRARY_ID != handle->data_guard)) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Handle Check Failed\n");
    return FAIL;
  }

  return SUCCESS;
}

/**
  This function validates whether Root node (Packaging) Chip Family matches Input Chip Family
    - Root Node Chipinfo property can be a list, then this function will parse over every chipinfo
      entry and compare the Family value against Input's Chipinfo Family.
    - If match is found, then it will store that Root Node's Chipinfo entry into current dtb/dtbo
      entry (struct handle_entry_data)
    - If no entry is found within Root Node's Chipinfo list, then it will return Invalid entry
  @return
    Returns validity status of the current dtb/dtbo entry based on above validations.
      - If entry is valid, then it returns VALID_CANDIDATE
      - If entry is invalid, then it returns INVALID_CANDIDATE
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_is_chipinfo_family_match (
  handle_entry_data  *current_entry_data,
  handle_input_data  *input_parameters
  )
{
  /* Packaging Root Node's Chipinfo Family against Device's (input) Chip Family */
  if (!current_entry_data || !input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Get fdt handle node for current dtb/dtbo entry */
  fdt_node_handle  *node                      = &(current_entry_data->node);
  uint16_t         *DT_chipinfo_property      = NULL;
  uint32_t         DT_chipinfo_property_size  = 0x0;
  uint32_t         number_of_chipinfo_entries = 0x0;

  /* By default, mark this entry as Invalid */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /* Get Chipinfo from ROOT node */
  int  return_status = fdt_get_prop_size (node, CHIPINFO_PROP_NAME, &DT_chipinfo_property_size);

  if (FDT_ERR_QC_NOERROR != return_status) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unable to fetch \"%s\"! Moving to next Blob\n", CHIPINFO_PROP_NAME);
    goto EXIT_POINT (_is_chipinfo_family_match);
  }

  /* DT property size should be a multiple of Input property size */
  if (DT_chipinfo_property_size % SINGLE_CHIPINFO_SIZE_IN_BYTES) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "DT property size for \"%s\" is not a multiple of Input property size! \
                          Moving to next Blob\n", CHIPINFO_PROP_NAME);
    goto EXIT_POINT (_is_chipinfo_family_match);
  }

  DT_chipinfo_property = (uint16_t *)MALLOC_WRAPPER (DT_chipinfo_property_size);
  if (!DT_chipinfo_property) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memory allocation failed\n");
    return MALLOC_FAIL;
  }

  return_status = fdt_get_uint16_prop_list (node, CHIPINFO_PROP_NAME, DT_chipinfo_property, DT_chipinfo_property_size);
  if (FDT_ERR_QC_NOERROR != return_status) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unable to fetch \"%s\"! Moving to next Blob\n", CHIPINFO_PROP_NAME);
    goto EXIT_POINT (_is_chipinfo_family_match);
  }

  /* Calculate Number of Chipinfo(s) DT provided */
  number_of_chipinfo_entries = DT_chipinfo_property_size / SINGLE_CHIPINFO_SIZE_IN_BYTES;
  /* Check for potential pointer arithmetic errors */
  if (SAFE != _is_safe_to_add_pointer ((uintptr_t)DT_chipinfo_property, DT_chipinfo_property_size)) {
    return POINTER_ARITHMETIC_OVERFLOW;
  }

  for (uint32_t current_property_index = 0; current_property_index < number_of_chipinfo_entries; current_property_index++) {
    uint16_t  *current_chipinfo = (uint16_t *)((uintptr_t)DT_chipinfo_property + (SINGLE_CHIPINFO_SIZE_IN_BYTES * current_property_index));

    if (current_chipinfo[CHIPINFO_FAMILY] == input_parameters->Input_chip_plat_info_prop->chip_family) {
      /* if correct Chipinfo Family entry found, break and move ahead with further DTB/DTBO selection process.
         Entire DT_chipinfo_selected is equal to 0x0 when handle_entry_data is initialized i.e. _init_handle_entry_data().
      */
      current_entry_data->DT_chipinfo_selected[CHIPINFO_FAMILY] = current_chipinfo[CHIPINFO_FAMILY];
      current_entry_data->DT_chipinfo_selected[CHIPINFO_ID]     = current_chipinfo[CHIPINFO_ID];
      current_entry_data->DT_chipinfo_selected[CHIPINFO_MAJOR]  = current_chipinfo[CHIPINFO_MAJOR];
      current_entry_data->DT_chipinfo_selected[CHIPINFO_MINOR]  = current_chipinfo[CHIPINFO_MINOR];

      /* Update the return flag */
      current_entry_data->candidacy_status = VALID_CANDIDATE;
      break;
    }
  }

  /* Entry not found, move to next entry */
  if (VALID_CANDIDATE != current_entry_data->candidacy_status) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" Chipinfo Family mismatch! Moving to next Blob\n", CHIPINFO_PROP_NAME);
  }

  EXIT_POINT (_is_chipinfo_family_match) :
    if (DT_chipinfo_property) {
    FREE_WRAPPER (DT_chipinfo_property, DT_chipinfo_property_size);
  }

  /* Return success to move to next Blob entry (in DT file) based on its validity (candidacy_status) */
  return SUCCESS;
}

/**
  This function determines the type of blob the current dtb/dtbo entry (struct handle_entry_data) represents:
    a. DTB
    b. DTBO
      1. SOC DTBO
      2. Platform DTBO
  Identification is performed in following steps:
    - If "is-overlay" property does not exists in the Root Node, then it represents DTB entry.
    - If "is-overlay" property exists, then:
      - if "platinfo" (a.k.a Platforminfo) property exists, then it represents Platform DTBO.
      - if "platinfo" (a.k.a Platforminfo) property does not exists, then it represents SOC DTBO.
  @return
    Returns validity status of the current dtb/dtbo entry based on above validations.
      - If entry is valid, then it returns VALID_CANDIDATE
      - If entry is invalid, then it returns INVALID_CANDIDATE
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_identify_blob_type_and_get_platforminfo (
  handle_entry_data  *current_entry_data,
  handle_input_data  *input_parameters
  )
{
  if (!current_entry_data || !input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Get fdt handle node for current dtb/dtbo entry */
  fdt_node_handle  *node                          = &(current_entry_data->node);
  uint8_t          *DT_platforminfo_property      = NULL;
  uint32_t         DT_platforminfo_property_size  = 0x0;
  uint32_t         number_of_platforminfo_entries = 0x0;

  /* By default, mark this entry as Invalid */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /*
     Condition for deciding type of device tree entry:

                        -----------------------------------------------
                       |            Property in Root Node              |
     ------------------------------------------------------------------
     |Idx | Type       |    "is-overlay"       |       "platinfo"      |
     ------------------------------------------------------------------
     | A. | DTB        |      Not Present      |           X           |
     | B. | SOC DTBO   |       Present         |      Not Present      |
     | C. | PLAT DTBO  |       Present         |        Present        |
     ------------------------------------------------------------------
  */

  /* Defaulting current_entry_data->device_tree_type to BASE_DTB */
  current_entry_data->device_tree_type = BASE_DTB;
  int  return_status = fdt_get_bool_prop (node, IS_OVERLAY_PROP_NAME);

  if (return_status) {
    if ((-FDT_ERR_NOTFOUND) == return_status) {
      /* Type of Blob = DTB */
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Property \"%s\" does not exist; hence treating this entry as DTB\n", IS_OVERLAY_PROP_NAME);
      current_entry_data->candidacy_status = VALID_CANDIDATE;
    } else {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" error in reading! Moving to next Blob", IS_OVERLAY_PROP_NAME);
      goto EXIT_POINT (_identify_blob_type_and_get_platforminfo);
    }
  } else {
    /* If this blob is an overlay, then evaluate whether it is SOC or PLAT DTBO.
       Defaulting it to SOC_DTBO
    */
    current_entry_data->device_tree_type = SOC_DTBO;

    return_status = fdt_get_prop_size (node, PLATFORMINFO_PROP_NAME, &DT_platforminfo_property_size);

    /* If Platform info is absent in Packaging Root Node, then the current Blob entry is SOC DTBO;
       Else the current entry is PLAT DTBO
    */

    /* Check for any unexpected error while fetching "platinfo" property */
    if ((FDT_ERR_QC_NOERROR != return_status) && ((-FDT_ERR_NOTFOUND) != return_status)) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Error in getting size for \"%s\"! Moving to next Blob\n", PLATFORMINFO_PROP_NAME);
      goto EXIT_POINT (_identify_blob_type_and_get_platforminfo);
    }

    if ((-FDT_ERR_NOTFOUND) == return_status) {
      /* Type of Blob = SOC DTBO */
      /* Update the return flag */
      current_entry_data->candidacy_status = VALID_CANDIDATE;
    } else {
      /* "platinfo" property is present in Root Node */
      if (DT_platforminfo_property_size % SINGLE_PLATFORMINFO_SIZE_IN_BYTES) {
        PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "DT property size for \"%s\" \
                is not a multiple of Input property size! Moving to next Blob\n", PLATFORMINFO_PROP_NAME);
        goto EXIT_POINT (_identify_blob_type_and_get_platforminfo);
      }

      /* Platform DTBO */
      current_entry_data->device_tree_type = PLATFORM_DTBO;

      DT_platforminfo_property = (uint8_t *)MALLOC_WRAPPER (DT_platforminfo_property_size);
      if (!DT_platforminfo_property) {
        PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memory allocation failed\n");
        return MALLOC_FAIL;
      }

      return_status = fdt_get_uint8_prop_list (
                                               node,
                                               PLATFORMINFO_PROP_NAME,
                                               (uint8_t *)DT_platforminfo_property,
                                               DT_platforminfo_property_size
                                               );
      if (FDT_ERR_QC_NOERROR != return_status) {
        PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" error in reading! Moving to next Blob\n", PLATFORMINFO_PROP_NAME);
        goto EXIT_POINT (_identify_blob_type_and_get_platforminfo);
      }

      /* Calculate Number of Chipinfo(s) DT provided */
      number_of_platforminfo_entries = DT_platforminfo_property_size / SINGLE_PLATFORMINFO_SIZE_IN_BYTES;
      /* Check for potential pointer arithmetic errors */
      if (SAFE != _is_safe_to_add_pointer ((uintptr_t)DT_platforminfo_property, DT_platforminfo_property_size)) {
        return POINTER_ARITHMETIC_OVERFLOW;
      }

      for (uint32_t current_property_index = 0; current_property_index < number_of_platforminfo_entries; current_property_index++) {
        uint8_t  *current_platforminfo = (uint8_t *)((uintptr_t)DT_platforminfo_property +
                                                     (SINGLE_PLATFORMINFO_SIZE_IN_BYTES * current_property_index));

        if (current_platforminfo[PLATFORMINFO_TYPE] == input_parameters->Input_chip_plat_info_prop->platform_type) {
          /* if correct Platforminfo Type entry found, break and move ahead with further DTB/DTBO selection process.
          Entire DT_platforminfo_selected is equal to 0x0 when handle_entry_data is initialized i.e. _init_handle_entry_data().
          */
          current_entry_data->DT_platforminfo_selected[PLATFORMINFO_TYPE]     = current_platforminfo[PLATFORMINFO_TYPE];
          current_entry_data->DT_platforminfo_selected[PLATFORMINFO_SUB_TYPE] = current_platforminfo[PLATFORMINFO_SUB_TYPE];
          current_entry_data->DT_platforminfo_selected[PLATFORMINFO_MAJOR]    = current_platforminfo[PLATFORMINFO_MAJOR];
          current_entry_data->DT_platforminfo_selected[PLATFORMINFO_MINOR]    = current_platforminfo[PLATFORMINFO_MINOR];

          /* Type of Blob = PLATFORM DTB */
          /* Update the return flag */
          current_entry_data->candidacy_status = VALID_CANDIDATE;
          break;
        }
      }

      if (VALID_CANDIDATE != current_entry_data->candidacy_status) {
        PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" Platforminfo Type mismatch! Moving to next Blob\n", PLATFORMINFO_PROP_NAME);
      }
    }
  }

  EXIT_POINT (_identify_blob_type_and_get_platforminfo) :
    if (DT_platforminfo_property) {
    FREE_WRAPPER (DT_platforminfo_property, DT_platforminfo_property_size);
    DT_platforminfo_property = NULL;
  }

  return SUCCESS;
}

/**
  This function compares whether Root Node (Packaging) Compatible string starts with Input's compatible string.
    - For Input's compatible string, clients provide 3 strings.
      a. DTB:           dtb_compatible_string_starts_with
      b. SOC DTBO:      soc_dtbo_compatible_string_starts_with
      c. Platform DTBO: plat_dtbo_compatible_string_starts_with
    - Based on type of blob, Input string will point to respective string pointer for comparison against Root Node's
      (Packaging) compatible property.
    - If Root Node (Packaging) compatible property is a list, then this function will parse through every string entry
      in the list and compare against Input's string.
  @return
    Returns validity status of the current dtb/dtbo entry based on above validations.
      - If entry is valid, then it returns VALID_CANDIDATE
      - If entry is invalid, then it returns INVALID_CANDIDATE
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_validate_compatible_string (
  handle_entry_data  *current_entry_data,
  handle_input_data  *input_parameters
  )
{
  if (!current_entry_data || !input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Compare #2: Check for Input compatible string exists or not.
  If Input compatible string exists, compare against Root Node (Packaging) "compatible" property
  Else skip checking

  --------------------------------------------------------------------------------------
  | For DT,       | Input compatible string is dtb_compatible_string_starts_with       |
  | For SOC DTBO, | Input compatible string is soc_dtbo_compatible_string_starts_with  |
  | For PLAT DTBO,| Input compatible string is plat_dtbo_compatible_string_starts_with |
  --------------------------------------------------------------------------------------

  If Input compatible string exists, then Compare DT Root Node "compatible" property against:
  a.   For DT, compare with Input dtb_compatible_string_starts_with string
  b.   For SOC DTBO, compare with Input soc_dtbo_compatible_string_starts_with string
  c.   For PLAT DTBO, compare with Input plat_dtbo_compatible_string_starts_with string
  */

  /* Get fdt handle node for current dtb/dtbo entry */
  fdt_node_handle  *node                                = &(current_entry_data->node);
  char             *DT_compatible_property              = NULL;
  char             *Input_compatible_string_starts_with = NULL;
  char             *current_string_ptr                  = NULL;
  uint32_t         DT_compatible_property_size          = 0x0;
  uint32_t         compatible_property_size             = 0x0;
  int              return_status                        = ~FDT_ERR_QC_NOERROR;

  /* By default, mark this entry as Invalid */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  Input_compatible_string_starts_with = input_parameters->Input_chip_plat_info_prop->dtb_compatible_string_starts_with;
  if (current_entry_data->device_tree_type == SOC_DTBO) {
    Input_compatible_string_starts_with = input_parameters->Input_chip_plat_info_prop->soc_dtbo_compatible_string_starts_with;
  } else if (current_entry_data->device_tree_type == PLATFORM_DTBO) {
    Input_compatible_string_starts_with = input_parameters->Input_chip_plat_info_prop->plat_dtbo_compatible_string_starts_with;
  }

  /* If Input_compatible_string_starts_with is NULL, skip further check and return VALID_CANDIDATE */
  if (NULL == Input_compatible_string_starts_with) {
    current_entry_data->candidacy_status = VALID_CANDIDATE;
    goto EXIT_POINT (_validate_compatible_string);
  }

  /* if Input compatible string exists, it should match with packaging's compatible property */
  return_status = fdt_get_prop_size (node, COMPATIBLE_PROP_NAME, &DT_compatible_property_size);
  if ((FDT_ERR_QC_NOERROR != return_status) || !DT_compatible_property_size) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\": Property not found! Moving to next Blob\n", COMPATIBLE_PROP_NAME);
    goto EXIT_POINT (_validate_compatible_string);
  }

  /* If string length for Input_compatible_string_starts_with is 0x0, then directly return VALID_CANDIDATE
     Skip the fetching of compatible property and comparing it with Input_compatible_string_starts_with string
  */
  if (!strlen (Input_compatible_string_starts_with)) {
    current_entry_data->candidacy_status = VALID_CANDIDATE;
    goto EXIT_POINT (_validate_compatible_string);
  }

  DT_compatible_property = (char *)MALLOC_WRAPPER (DT_compatible_property_size);
  if (!DT_compatible_property) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memory allocation failed\n");
    return MALLOC_FAIL;
  }

  /* Read entire "compatible" property from Root Node */
  return_status = fdt_get_string_prop_list (
                                            node,
                                            COMPATIBLE_PROP_NAME,
                                            (void *)DT_compatible_property,
                                            DT_compatible_property_size
                                            );
  if (FDT_ERR_QC_NOERROR != return_status) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unable to fetch \"%s\"! Moving to next Blob\n", COMPATIBLE_PROP_NAME);
    goto EXIT_POINT (_validate_compatible_string);
  }

  current_string_ptr       = DT_compatible_property;
  compatible_property_size = DT_compatible_property_size;

  /* Validate Input compatible string to all string(s) in "compatible" property. If found, break */
  while (compatible_property_size > 0) {
    /* Add 0x1 for Null character */
    int  size = strlen (current_string_ptr) + 0x1;
    /* If valid string found, break from loop */
    if (0x0 == strncmp (
                        current_string_ptr,
                        Input_compatible_string_starts_with,
                        strlen (Input_compatible_string_starts_with)
                        ))
    {
      current_entry_data->candidacy_status = VALID_CANDIDATE;
      break;
    }

    /* Check for pointer overflow before we increment size to current_string_ptr pointer */
    if (SAFE != _is_safe_to_add_pointer ((uintptr_t)current_string_ptr, (size * sizeof (*current_string_ptr)))) {
      return POINTER_ARITHMETIC_OVERFLOW;
    }

    current_string_ptr += size;

    /* check for "wrap around" of unsigned int */
    if (SAFE != _is_safe_to_subtract_uint32 (compatible_property_size, size)) {
      return UINT32_ARITHMETIC_OVERFLOW;
    }

    compatible_property_size -= size;
  }

  current_string_ptr                  = NULL;
  Input_compatible_string_starts_with = NULL;

  /* Check whether string is found */
  if (VALID_CANDIDATE != current_entry_data->candidacy_status) {
    PRINT_LOG_MESSAGE (
                       buffer,
                       BUFFER_SIZE,
                       "\"%s\" mismatch! Did not match to any input \"%s\" string! Moving to next Blob",
                       COMPATIBLE_PROP_NAME,
                       COMPATIBLE_PROP_NAME
                       );
  }

  EXIT_POINT (_validate_compatible_string) :
    if (DT_compatible_property) {
    FREE_WRAPPER ((void *)DT_compatible_property, DT_compatible_property_size);
    DT_compatible_property = NULL;
  }

  return SUCCESS;
}

/**
  This function compares whether Root Node's (Packaging) Compatible string starts with Input's compatible string.
    - First it will need to identify the type of blob the current dtb/dtbo entry (struct handle_entry_data) represents:
      a. DTB
      b. DTBO
        1. SOC DTBO
        2. Platform DTBO
    - For Input's compatible string, clients provide 3 strings.
      a. DTB:           dtb_compatible_string_starts_with
      b. SOC DTBO:      soc_dtbo_compatible_string_starts_with
      c. Platform DTBO: plat_dtbo_compatible_string_starts_with
  @return
    Returns validity status of the current dtb/dtbo entry based on above validations.
      - If entry is valid, then it returns VALID_CANDIDATE
      - If entry is invalid, then it returns INVALID_CANDIDATE
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_is_compatible_string_match (
  handle_entry_data  *current_entry_data,
  handle_input_data  *input_parameters
  )
{
  if (!current_entry_data || !input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* By default, mark this entry as Invalid */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /* Need to identify the type of blob the current dtb/dtbo entry (struct handle_entry_data) is */
  int  return_status = _identify_blob_type_and_get_platforminfo (current_entry_data, input_parameters);

  if ((FDT_ERR_QC_NOERROR != return_status) || (VALID_CANDIDATE != current_entry_data->candidacy_status)) {
    goto EXIT_POINT (_is_compatible_string_match);
  }

  /* Reset this entry as Invalid before Compatible string comparison */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /* Compare Root node (Packaging) compatible property "starts with" for Input's */
  return_status = _validate_compatible_string (current_entry_data, input_parameters);

  EXIT_POINT (_is_compatible_string_match) :
    return return_status;
}

/**
  This function validate whether all the Input properties that are provided from client matches with Root Node's (Packaging)
  properties.
  If a DT property is a list, then this function will parse every value and compare against Input value for that property.
  Every Input property should match against Root Node (Packaging) property value.
  @return
    Returns validity status of the current dtb/dtbo entry based on above validations.
      - If entry is valid, then it returns VALID_CANDIDATE
      - If entry is invalid, then it returns INVALID_CANDIDATE
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_is_property_list_valid (
  handle_entry_data  *current_entry_data,
  handle_input_data  *input_parameters
  )
{
  if (!current_entry_data || !input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Compare #3: Compare Packaging Root Node's additional property list against Input property list
  */

  /* Return VALID_CANDIDATE if there are no Property list provided by Client */
  if ((NULL == input_parameters->Input_prop_list) || (0x0 == input_parameters->prop_num_entries)) {
    current_entry_data->candidacy_status = VALID_CANDIDATE;
    goto EXIT_POINT (_is_property_list_valid);
  }

  /* Get fdt handle node for current dtb/dtbo entry */
  fdt_node_handle  *node            = &(current_entry_data->node);
  property_list    *Input_prop_list = input_parameters->Input_prop_list;

  for (uint32_t input_prop_list_index = 0; input_prop_list_index < input_parameters->prop_num_entries; input_prop_list_index++) {
    uintptr_t  DT_property_list = 0x0;
    uint32_t   DT_property_size = 0x0;

    /* Reset this entry as Invalid before validating every Property List entries */
    current_entry_data->candidacy_status = INVALID_CANDIDATE;

    /* Get size of individual properties from Packaging Root Node */
    int  return_status = fdt_get_prop_size (node, Input_prop_list[input_prop_list_index].prop_name, &DT_property_size);

    if (FDT_ERR_QC_NOERROR != return_status) {
      PRINT_LOG_MESSAGE (
                         buffer,
                         BUFFER_SIZE,
                         "Property \"%s\" not found/error in reading! Moving to next Blob\n",
                         Input_prop_list[input_prop_list_index].prop_name
                         );
      break;
    }

    if (!DT_property_size) {
      PRINT_LOG_MESSAGE (
                         buffer,
                         BUFFER_SIZE,
                         "DT property size for \"%s\" is 0x0! Moving to next Blob\n",
                         Input_prop_list[input_prop_list_index].prop_name
                         );
      break;
    }

    /* Input property size should be valid and DT property size should be a multiple of Input property size */
    if (!Input_prop_list[input_prop_list_index].prop_size_bytes || DT_property_size % Input_prop_list[input_prop_list_index].prop_size_bytes) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "DT property size for \"%s\" is not a multiple of Input property \
                              size! Moving to next Blob\n", Input_prop_list[input_prop_list_index].prop_name);
      break;
    }

    DT_property_list = (uintptr_t)MALLOC_WRAPPER (DT_property_size);
    if (!DT_property_list) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memory allocation failed\n");
      return MALLOC_FAIL;
    }

    /* Get datatype size of individual entries */
    switch (Input_prop_list[input_prop_list_index].prop_size_bytes) {
      case BYTE_SIZE_BYTES:
        /* Case for /bits/ 8 or 1 Byte */
        return_status = fdt_get_uint8_prop_list (
                                                 node,
                                                 Input_prop_list[input_prop_list_index].prop_name,
                                                 (uint8_t *)DT_property_list,
                                                 DT_property_size
                                                 );
        break;
      case WORD_SIZE_BYTES:
        /* Case for /bits/ 16 or 2 Bytes */
        return_status = fdt_get_uint16_prop_list (
                                                  node,
                                                  Input_prop_list[input_prop_list_index].prop_name,
                                                  (uint16_t *)DT_property_list,
                                                  DT_property_size
                                                  );
        break;
      case DWORD_SIZE_BYTES:
        /* Case for /bits/ 32 or 4 Bytes */
        return_status = fdt_get_uint32_prop_list (
                                                  node,
                                                  Input_prop_list[input_prop_list_index].prop_name,
                                                  (uint32_t *)DT_property_list,
                                                  DT_property_size
                                                  );
        break;
      case QWORD_SIZE_BYTES:
        /* Case for /bits/ 64 or 8 Bytes */
        return_status = fdt_get_uint64_prop_list (
                                                  node,
                                                  Input_prop_list[input_prop_list_index].prop_name,
                                                  (uint64_t *)DT_property_list,
                                                  DT_property_size
                                                  );
        break;
      default:
        return_status = ~FDT_ERR_QC_NOERROR;
        PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unsupported size! Moving to next Blob\n");
    }

    if (FDT_ERR_QC_NOERROR != return_status) {
      PRINT_LOG_MESSAGE (
                         buffer,
                         BUFFER_SIZE,
                         "Unable to fetch \"%s\"! Moving to next Blob\n",
                         Input_prop_list[input_prop_list_index].prop_name
                         );
      goto _end_of_prop_list_loop;
    }

    if (SAFE != _is_safe_to_add_pointer ((uintptr_t)DT_property_list, DT_property_size)) {
      return POINTER_ARITHMETIC_OVERFLOW;
    }

    /* Compare every value in the Device Tree property list against Input property */
    for (uint32_t current_property_index = 0;
         current_property_index < (DT_property_size/Input_prop_list[input_prop_list_index].prop_size_bytes);
         current_property_index++
         )
    {
      switch (Input_prop_list[input_prop_list_index].prop_size_bytes) {
        case BYTE_SIZE_BYTES:
          if (*((uint8_t *)DT_property_list+current_property_index) == *((uint8_t *)Input_prop_list[input_prop_list_index].value_buffer)) {
            current_entry_data->candidacy_status = VALID_CANDIDATE;
          }

          break;
        case WORD_SIZE_BYTES:
          if (*((uint16_t *)DT_property_list+current_property_index) == *((uint16_t *)Input_prop_list[input_prop_list_index].value_buffer)) {
            current_entry_data->candidacy_status = VALID_CANDIDATE;
          }

          break;
        case DWORD_SIZE_BYTES:
          if (*((uint32_t *)DT_property_list+current_property_index) == *((uint32_t *)Input_prop_list[input_prop_list_index].value_buffer)) {
            current_entry_data->candidacy_status = VALID_CANDIDATE;
          }

          break;
        case QWORD_SIZE_BYTES:
          if (*((uint64_t *)DT_property_list+current_property_index) == *((uint64_t *)Input_prop_list[input_prop_list_index].value_buffer)) {
            current_entry_data->candidacy_status = VALID_CANDIDATE;
          }

          break;
        default:
          PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unsupported size! Moving to next Blob\n");
          goto _end_of_prop_list_loop;
      }

      /* Check whether property value has been found */
      if (VALID_CANDIDATE == current_entry_data->candidacy_status) {
        break;
      }
    }    /* End of inner loop */

_end_of_prop_list_loop:
    if (DT_property_list) {
      FREE_WRAPPER ((void *)DT_property_list, DT_property_size);
      DT_property_list = 0x0;
    }

    /* Check if Device's (input) property value is found property list (Packaging) values.
       Return from this function even if a Single property does not match/returns an error.
    */
    if (VALID_CANDIDATE != current_entry_data->candidacy_status) {
      PRINT_LOG_MESSAGE (
                         buffer,
                         BUFFER_SIZE,
                         "Property value did not match for \"%s\"! Moving to next Blob\n",
                         Input_prop_list[input_prop_list_index].prop_name
                         );
      goto EXIT_POINT (_is_property_list_valid);
    }
  }  /* End of outer loop */

  EXIT_POINT (_is_property_list_valid) :
    return SUCCESS;
}

/**
  This function validate whether current entry of dtb/dtbo (pointed by struct handle_entry_data) is a valid candidate
  for selecting DTB, SOC DTBO or Platform DTBO. It validates against values provided by struct handle_input_data
  It validates by following steps:
    1. Root node (Packaging) Chip Family should match Input Chip Family
    2. Root node (Packaging) compatible string starts with Input compatible string
       - If Input compatible string exists, compare against Root Node (Packaging) "compatible" property
       - Client passes 3 strings for 3 selecting 3 types of blobs:
         a. BASE DTB
         b. SOC DTBO
         c. Platform DTBO
       - The Input strings can be NULL or optional if client does not want to select any of the above DTBO entry
    3. Root node (Packaging) ALL property value should match with Input property list.

  @return
    Returns validity status of the current dtb/dtbo entry based on above validations.
      - If entry is valid, then it returns VALID_CANDIDATE
      - If entry is invalid, then it returns INVALID_CANDIDATE
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_is_it_dtb_dtbo_candidate (
  handle_entry_data  *current_entry_data,
  handle_input_data  *input_parameters
  )
{
  if (!current_entry_data || !input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Step #1: Packaging Root Node's Chipinfo Family against Device's (input) Chip Family */
  int  return_status = _is_chipinfo_family_match (current_entry_data, input_parameters);

  if ((SUCCESS != return_status) || (VALID_CANDIDATE != current_entry_data->candidacy_status)) {
    goto EXIT_POINT (_is_it_dtb_dtbo_candidate);
  }

  /* Step #2: Check for Input compatible string exists or not.
     If Input compatible string exists, compare against Root Node (Packaging) "compatible" property
  */
  return_status = _is_compatible_string_match (current_entry_data, input_parameters);
  if ((SUCCESS != return_status) || (VALID_CANDIDATE != current_entry_data->candidacy_status)) {
    goto EXIT_POINT (_is_it_dtb_dtbo_candidate);
  }

  /* Step #3: Compare Packaging Root Node's additional property list against Input property list */
  return_status = _is_property_list_valid (current_entry_data, input_parameters);

  EXIT_POINT (_is_it_dtb_dtbo_candidate) :
    return return_status;
}

/**
  This function validate whether fdtheader pointer is pointing to correct DTB/DTBO.
  It validates differently based on 2 condition:
    1. Client provide image size of the entire dtb/dtbo elf image
      - First it validates whether fdtheader is pointing within the boundaries of the elf image.
      - Then it validates whether the fdtheader pointer is pointing to correct DTB/DTBO header (a.k.a valid blob).
      - Third, it validates whether blob size of the current DTB/DTBO entry is within the boundaries of the elf image.
   2. Client does not provide image size of the entire dtb/dtbo elf image
      - It validates whether the fdtheader pointer is pointing to correct DTB/DTBO header (a.k.a valid blob).
      - It validates size of current blob is non zero and when added to fdtheader, the pointer shouldn't overflow.

  @return
    Returns validity status of the current dtb/dtbo entry based on above validations.
      - If entry is valid, then it returns VALID_CANDIDATE
      - If entry is invalid, then it returns INVALID_CANDIDATE
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_is_fdtheader_at_end_of_image (
  uintptr_t          fdtheader,
  handle_data        *handle_ptr,
  handle_input_data  *input_parameters,
  handle_entry_data  *current_entry_data,
  bool               *end_of_dtb_dtbo_file
  )
{
  if (  !fdtheader || !handle_ptr || !handle_ptr->dtbs_image_start_address || !input_parameters || !current_entry_data
     || !end_of_dtb_dtbo_file )
  {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  int  return_status = FAIL;

  current_entry_data->candidacy_status = INVALID_CANDIDATE;
  bool  is_within_range = OUT_OF_RANGE;

  /* Default end of image flag to True */
  *end_of_dtb_dtbo_file = true;
  /* store fdtheader pointer to current blob data structure */
  (current_entry_data->node).blob = (void *)fdtheader;

  /* Based on entire dtbs image size is provided by client, the check for end of image is performed accordingly */

  /* Case#1: DTB/DTBO image size is provided by client */
  if (input_parameters->dtbs_image_size) {
    /* Step #1:
       Check if current fdtheader is within boundaries of entire dtb/dtbo image.
    */
    return_status = _address_within_image_range_and_pointer_overflow_check (
                                                                            (uintptr_t)(current_entry_data->node).blob,
                                                                            (uintptr_t)handle_ptr->dtbs_image_start_address,
                                                                            input_parameters->dtbs_image_size,
                                                                            &is_within_range
                                                                            );
    if (SUCCESS != return_status) {
      return return_status;
    }

    if (IN_RANGE != is_within_range) {
      /* Reached at the end of the appended DTB/DTBOs image file. Return INVALID_CANDIDATE as Candidacy status */
      goto EXIT_POINT (_is_fdtheader_at_end_of_image);
    }

    /* Step #2:
       Check if fdtheader pointer is pointing to correct FDT header.
       Since fdtheader pointer is within the range of the dtb/dtbo image, it should point to correct FDT header.
    */
    return_status = fdt_check_header ((void *)(current_entry_data->node).blob);
    if (return_status) {
      return return_status;
    }

    /* Step #3:
       Check if size of current blob is within boundaries of entire dtb/dtbo image.
       If not within the boundary, then return error.
    */
    return_status = fdt_get_blob_size (&(current_entry_data->node), (uint32_t *)&current_entry_data->device_tree_blob_size);
    if (FDT_ERR_QC_NOERROR != return_status) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Not a Valid Blob! Fetching Blob size failed. Returning error\n");
      return return_status;
    }

    if (0x0 == current_entry_data->device_tree_blob_size) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Not a Valid Blob! Blob size is Null. Returning error\n");
      return INVALID_BLOB_ENTRY;
    }

    is_within_range = OUT_OF_RANGE;
    if (SAFE != _is_safe_to_add_pointer ((uintptr_t)(current_entry_data->node).blob, current_entry_data->device_tree_blob_size)) {
      return POINTER_ARITHMETIC_OVERFLOW;
    }

    /* Check if size of current blob is within boundaries of entire dtb/dtbo image. */
    return_status = _address_within_image_range_and_pointer_overflow_check (
                                                                            (uintptr_t)((uint8_t *)(current_entry_data->node).blob + current_entry_data->device_tree_blob_size) - 1,
                                                                            (uintptr_t)handle_ptr->dtbs_image_start_address,
                                                                            input_parameters->dtbs_image_size,
                                                                            &is_within_range
                                                                            );
    if (SUCCESS != return_status) {
      return return_status;
    }

    if (IN_RANGE != is_within_range) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Not a Valid Blob! Blob size is bigger than overall dtb/dtbo image. Returning error\n");
      return INVALID_BLOB_ENTRY;
    }
  }
  /* Case#2: DTB/DTBO image size is Not provided by client */
  else {
    /* Step #1:
       Validate DTB/DTBO header that fdtheader pointer is pointing to.
       Since size of entire appended DTB/DTBOs image size is unavailable, the only way to know whether fdtheader pointer reached at the end
       of the image is to check FDT header.
    */
    return_status = fdt_check_header ((void *)(current_entry_data->node).blob);
    if (return_status) {
      if ((-FDT_ERR_BADMAGIC) == return_status) {
        /* Reached at the end of the appended DTB/DTBOs image file. Return INVALID_CANDIDATE as Candidacy status */
        goto EXIT_POINT (_is_fdtheader_at_end_of_image);
      } else {
        return return_status;
      }
    }

    /* Step #2:
       Check if size of current blob is non zero and when added to fdtheader, the pointer shouldn't overflow.
    */
    return_status = fdt_get_blob_size (&(current_entry_data->node), (uint32_t *)&current_entry_data->device_tree_blob_size);
    if ((FDT_ERR_QC_NOERROR != return_status) || !current_entry_data->device_tree_blob_size) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Not a Valid Blob! Returning error\n");
      return INVALID_BLOB_ENTRY;
    }

    if (SAFE != _is_safe_to_add_pointer ((uintptr_t)(current_entry_data->node).blob, (current_entry_data->device_tree_blob_size * sizeof (uint8_t)))) {
      return POINTER_ARITHMETIC_OVERFLOW;
    }
  }

  /* fdtheader is within bounds and is pointing to a valid DTB/DTBO entry. Thus marking this entry as Valid */
  current_entry_data->candidacy_status = VALID_CANDIDATE;
  *end_of_dtb_dtbo_file                = false;

  EXIT_POINT (_is_fdtheader_at_end_of_image) :
    return SUCCESS;
}

/**
  This function checks and returns whether pointer_to_check is within the range of [start, start+size].
  It also checks the pointer overflow when range_pointer_upper is calculated.
  @return
    Returns status for range.
      - If pointer is within the range, then it returns IN_RANGE.
      - If pointer is out of range, then it returns OUT_OF_RANGE.
    If addition of pointer overflow, then it returns POINTER_ARITHMETIC_OVERFLOW.
    Returns FAIL if any operations fails, else return SUCCESS.
*/
static int
_address_within_image_range_and_pointer_overflow_check (
  uintptr_t  address,
  uintptr_t  start,
  size_t     size,
  bool       *is_within_range
  )
{
  if (!is_within_range) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  *is_within_range = OUT_OF_RANGE;
  /* Check for pointer overflow before we increment size to start pointer */
  if (SAFE != _is_safe_to_add_pointer (start, size)) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Pointer arithmetic overflow\n");
    return POINTER_ARITHMETIC_OVERFLOW;
  }

  /* Check whether pointer_to_check is within the range */
  if ((start <= address) && (address < (start + size))) {
    *is_within_range = IN_RANGE;
  }

  return SUCCESS;
}

/**
  This function parses the every dtb/dtbo entry in dtb elf image and selects the best base dtb, soc overlay and platform overlay blobs.
  @return
    Returns Address and size for selected:
      1. Base DTB
      2. SOC DTBO
      3. Platform DTBO
    The address and size are returned in struct handle_data pointer.
    If any of the Overlay entries is not selected, then it simply returns NULL and 0x0 for address and size resp.
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_select_dtb_and_dtbos (
  handle_data        *handle_ptr,
  handle_input_data  *input_parameters
  )
{
  PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "DT Selection Algorithm start\n");
  if (!handle_ptr || !handle_ptr->dtbs_image_start_address || !input_parameters) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  int        return_status        = FAIL;
  uintptr_t  fdtheader            = handle_ptr->dtbs_image_start_address;
  bool       end_of_dtb_dtbo_file = false;

  /* Initialize data structures that will cache and store all the Selected entries
     for both DTBOs (SOC DTBO and Platform DTBO)
     Note: Selected Base DTB information is stored in struct handle_data directly.
  */
  handle_selected_dtbo  soc_dtbo_selected;
  handle_selected_dtbo  platform_dtbo_selected;

  return_status = _init_handle_selected_dtb_dtbo (&soc_dtbo_selected);
  if (SUCCESS != return_status) {
    goto EXIT_POINT (_select_dtb_and_dtbos);
  }

  return_status = _init_handle_selected_dtb_dtbo (&platform_dtbo_selected);
  if (SUCCESS != return_status) {
    goto EXIT_POINT (_select_dtb_and_dtbos);
  }

  /* Parse every entry of dtb/dtbo in the blob containing appended DTB/DTBOs file */
  while (true) {
    /* Store information for every DTB/DTBO entries in this structure */
    handle_entry_data  current_entry_data;

    /* Reset handle for every image */
    return_status = _init_handle_entry_data (&current_entry_data);
    if (SUCCESS != return_status) {
      return FAIL;
    }

    /* Validate whether every entry is a valid dtb/dtbo entry */
    return_status = _is_fdtheader_at_end_of_image (fdtheader, handle_ptr, input_parameters, &current_entry_data, &end_of_dtb_dtbo_file);
    if (SUCCESS != return_status) {
      return return_status;
    }

    /* If End of dtb/dtbo elf image is reached, break the loop and return the selected dtb/dtbos to client */
    if (true == end_of_dtb_dtbo_file) {
      break;
    }

    /* Get ROOT node from the current dtbo blob before ROOT node properties are validated for selecting DTB/DTBO */
    return_status = fdt_get_node_handle (&(current_entry_data.node), (current_entry_data.node).blob, ROOT_NODE);
    if (FDT_ERR_QC_NOERROR != return_status) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Unable to fetch Root Node! Returning error\n");
      return GET_ROOT_NODE_FAILED;
    }

    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "== Attempting Blob entry #%d ==\n", blob_index++);

 #ifdef DT_SELECT_LOGGING_ENABLE
    /* current_entry_data.node is already pointing to /board-id/ node */
    return_status = _print_board_id (
                                     fdtheader,
                                     input_parameters->Input_prop_list,
                                     input_parameters->prop_num_entries
                                     );
    if (SUCCESS != return_status) {
      return return_status;
    }

 #endif

    /* If the function returns an error, skip further check and return error.
       Move to next DTB/DTBO blob if current entry is Not Valid.
       If it returns FAIL, then simply return the error.
    */
    return_status = _is_it_dtb_dtbo_candidate (&current_entry_data, input_parameters);
    if (SUCCESS != return_status) {
      return return_status;
    }

    /* Check whether this is a valid entry or not. If not, then skip to next entry */
    if (VALID_CANDIDATE != current_entry_data.candidacy_status) {
      goto _next_dtb_dtbo_entry;
    }

    /* Reset it for further validation */
    current_entry_data.candidacy_status = INVALID_CANDIDATE;

    /* Once the validation of dtb/dtbo entry is done, then further selection needs to be performed differently
       for 3 different types of blobs:
       1. Base DTB
       2. SOC DTBO
       3. Platform DTBO

       Based on type of blob, the best selected candidate (for each type) will be stored in:
       1. Base DTB: in struct handle_data
       2. SOC DTBO: in soc_dtbo_selected
       3. Platform DTBO: in platform_dtbo_selected
    */
    switch (current_entry_data.device_tree_type) {
      /* Case for DTB */
      case BASE_DTB:
        return_status = _validate_and_select_base_dtb (&current_entry_data, handle_ptr);
        if (VALID_CANDIDATE == current_entry_data.candidacy_status) {
          /* Store Current entry's Blob pointer, Rank, Chipinfo version for selected SOC DTBO */
          handle_ptr->base_dtb_ptr  = (uintptr_t)current_entry_data.node.blob;
          handle_ptr->base_dtb_size = current_entry_data.device_tree_blob_size;
          PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Entry selected for Base DTB\n");
        }

        break;

      /* Case for SOC DTBO */
      case SOC_DTBO:
        return_status = _validate_and_select_soc_dtbo (
                                                       &current_entry_data,
                                                       input_parameters,
                                                       &soc_dtbo_selected
                                                       );
        if (VALID_CANDIDATE == current_entry_data.candidacy_status) {
          /* Store Current entry's Blob pointer, Rank, Chipinfo version for selected SOC DTBO */
          handle_ptr->soc_dtbo_ptr                = (uintptr_t)current_entry_data.node.blob;
          handle_ptr->soc_dtbo_size               = current_entry_data.device_tree_blob_size;
          soc_dtbo_selected.rank                  = current_entry_data.rank;
          soc_dtbo_selected.device_tree_blob_size = current_entry_data.device_tree_blob_size;
          soc_dtbo_selected.chipinfo_version_calc = current_entry_data.chipinfo_version_calc;
          PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Potential Entry selected for SOC (chip) DTBO\n");
        }

        break;

      /* Case for SOC DTBO */
      case PLATFORM_DTBO:
        return_status = _validate_and_select_platform_dtbo (
                                                            &current_entry_data,
                                                            input_parameters,
                                                            &platform_dtbo_selected
                                                            );
        if (VALID_CANDIDATE == current_entry_data.candidacy_status) {
          /* Store Blob pointer, Rank, Chipinfo version and Platform version for selected Platform DTBO */
          handle_ptr->plat_dtbo_ptr                        = (uintptr_t)current_entry_data.node.blob;
          handle_ptr->plat_dtbo_size                       = current_entry_data.device_tree_blob_size;
          platform_dtbo_selected.rank                      = current_entry_data.rank;
          platform_dtbo_selected.device_tree_blob_size     = current_entry_data.device_tree_blob_size;
          platform_dtbo_selected.chipinfo_version_calc     = current_entry_data.chipinfo_version_calc;
          platform_dtbo_selected.platforminfo_version_calc = current_entry_data.platforminfo_version_calc;
          PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Potential Entry selected for Platform (board) DTBO\n");
        }

        break;
      default:
        PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Blob Type not supported! ! Moving to next Blob\n");
        goto _next_dtb_dtbo_entry;
    }    /* End of Switch */

_next_dtb_dtbo_entry:
    fdtheader = (uintptr_t)((uint8_t *)fdtheader + current_entry_data.device_tree_blob_size);
  }

  /*  Entries for following should be selected:
      -----------------------------------------
      Category    =   Location where its stored
      -----------------------------------------
      - Base DTB  =   handle_ptr->base_dtb_ptr
      - Plat DTBO =   handle_ptr->plat_dtbo_ptr
      - SOC DTBO  =   handle_ptr->soc_dtbo_ptr
  */

  PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "DT Selection Algorithm end\n");
  EXIT_POINT (_select_dtb_and_dtbos) :
  return SUCCESS;
}

/**
  This function validates the current dtb entry (struct handle_entry_data) and selects best candidate for Base DTB.
  The best candidate is chosen if Root node's (Packaging) Chip ID is 0x0 and Major-Minor version is 1-0.
  If the best candidate was already chosen earlier, then this entry will not be selected for best candidate for
  Base DTB.
  This API gets called only for DTB blob which has Chipinfo Family already matched and hence skipping that check.
  @return
    Returns validity status of the current dtb entry based on above validations.
      - If entry is best candidate so far, then it returns VALID_CANDIDATE.
      - If entry is not best candidate, then it returns INVALID_CANDIDATE.
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_validate_and_select_base_dtb (
  handle_entry_data  *current_entry_data,
  handle_data        *handle_ptr
  )
{
  if (!current_entry_data || !handle_ptr) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Default value for selecting this SOC DTBO entry should be False */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /* Base DTB selected should have Chipinfo ID masked off with 1.0 version.
     Chipinfo Family has already been matched in _is_it_dtb_dtbo_candidate(), thus skipping this check.
     Ignore new entry if an entry for Base DTB is already selected
  */
  if ((handle_ptr->base_dtb_ptr == 0x0) &&
      (current_entry_data->DT_chipinfo_selected[CHIPINFO_ID] == 0x0) &&
      (current_entry_data->DT_chipinfo_selected[CHIPINFO_MAJOR] == 0x1) &&
      (current_entry_data->DT_chipinfo_selected[CHIPINFO_MINOR] == 0x0)
      )
  {
    current_entry_data->candidacy_status = VALID_CANDIDATE;
  }

  return SUCCESS;
}

/**
  This function validates the current dtbo entry (struct handle_entry_data) and selects best candidate for SOC DTBO.
  The slection is done based on
    - Calculating Rank (using Chipinfo ID) and Version (using Major and Minor)
    - Comparing the Rank and Version against best selected candidate for SOC DTBO so far.
  This API gets called only for DTB blob which has Chipinfo Family already matched and hence skipping that check.
  @return
    Returns validity status of the current dtbo entry.
      - If entry is best candidate so far, then it returns VALID_CANDIDATE.
      - If entry is not best candidate, then it returns INVALID_CANDIDATE.
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_validate_and_select_soc_dtbo (
  handle_entry_data     *current_entry_data,
  handle_input_data     *input_parameters,
  handle_selected_dtbo  *soc_dtbo_selected
  )
{
  /* The comparison is perfomed in following steps:
      1. Chipinfo version (Packaging) should not be greater than device's (Input) Chipinfo version.
      2. Rank of already selected Blob entry should not be greater than current rank.
      3. Perform Chipinfo version check against Input Version and against Selected candidate (Best
         SOC DTBO entry till now) version.
  */
  if (!current_entry_data || !input_parameters || !soc_dtbo_selected ||
      !input_parameters->Input_chip_plat_info_prop)
  {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* Default value for selecting this SOC DTBO entry should be False */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /*
    According to  algorithm, the Rank is calculated based on Chipinfo ID.
    If Chipinfo ID is Not masked of, then it should match with Input Chipinfo ID.

    Rank is determined by following condition:
     -----------------------
     [    Chip ID  ] = Rank
     -----------------------
     [     Match   ] =  1
     [  Masked(0x0)] =  0

      Note: Higher the Rank value, higher the priority
  */

  /* According to algorithm: If Chipinfo ID is Not masked off (a.k.a Non zero), then it should match with
     Input's Chipinfo ID. */
  if (current_entry_data->DT_chipinfo_selected[CHIPINFO_ID] &&
      (current_entry_data->DT_chipinfo_selected[CHIPINFO_ID] != input_parameters->Input_chip_plat_info_prop->chip_id))
  {
    goto EXIT_POINT (_validate_and_select_soc_dtbo);
  }

  /* Calculate Rank for the current SOC DTBO based on look-up table */
  if (current_entry_data->DT_chipinfo_selected[CHIPINFO_ID]) {
    current_entry_data->rank = 0x1;
  } else {
    current_entry_data->rank = 0x0;
  }

  /* Calculate Integer form of Chipinfo Versions for comparison against Input Chipinfo Version */
  current_entry_data->chipinfo_version_calc = CALCULATE_CHIP_PLAT_INFO_VERSION (
                                                                                current_entry_data->DT_chipinfo_selected[CHIPINFO_MAJOR],
                                                                                current_entry_data->DT_chipinfo_selected[CHIPINFO_MINOR]
                                                                                );

  /* Version Bound check: The version(s) of the Current entry should not be greater than Input's version(s).

     If Chipinfo version (Packaging) is greater than Input Chipinfo version, then skip further check and
     return Invalid entry.
  */
  if (current_entry_data->chipinfo_version_calc > input_parameters->chipinfo_version_calc) {
    goto EXIT_POINT (_validate_and_select_soc_dtbo);
  }

  /* If Rank of selected entry is higher than Rank of current, skip further check and return Invalid entry. */
  if (soc_dtbo_selected->rank > current_entry_data->rank) {
    goto EXIT_POINT (_validate_and_select_soc_dtbo);
  }
  /* If Rank of selected entry is equal to Rank of current entry, then select based on closest Chipinfo Version */
  else if (soc_dtbo_selected->rank == current_entry_data->rank) {
    /* If current entry's Chipinfo version is higher than selected Chipinfo version, then mark this entry as Valid */
    if (current_entry_data->chipinfo_version_calc > soc_dtbo_selected->chipinfo_version_calc) {
      current_entry_data->candidacy_status = VALID_CANDIDATE;
    } else if (current_entry_data->chipinfo_version_calc <= soc_dtbo_selected->chipinfo_version_calc) {
      /*
          Note: We skip second entry with exact same Versions.
              The first entry (with exact same versions) in the list is picked.
      */
      current_entry_data->candidacy_status = INVALID_CANDIDATE;
    }
  } else {
    /* soc_dtbo_selected->rank < current_entry_data->rank */

    /* If Rank of selected entry is less than Rank of current entry, then mark this exntry as Valid */
    current_entry_data->candidacy_status = VALID_CANDIDATE;
  }

  EXIT_POINT (_validate_and_select_soc_dtbo) :
    return SUCCESS;
}

/**
  This function validates the Root Node (Packaging) oem variant property against Input
    - If Root Node's (Packaging) has OEM variant, then it has to match with Input's OEM variant
  @return
    Returns validity status of the current dtbo entry.
      - If entry is best candidate so far, then it returns VALID_CANDIDATE.
      - If entry is not best candidate, then it returns INVALID_CANDIDATE.
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_validate_oem_variant (
  handle_entry_data  *current_entry_data,
  handle_input_data  *input_parameters
  )
{
  if (!current_entry_data || !input_parameters || !input_parameters->Input_chip_plat_info_prop) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  uint32_t  *DT_oem_var_property     = NULL;
  uint32_t  DT_oem_var_property_size = 0x0;
  /* Get fdt handle node for current dtb/dtbo entry */
  fdt_node_handle  *node = &(current_entry_data->node);

  /* Default value for selecting this Platform DTBO entry should be False */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  int  return_status = fdt_get_prop_size (node, OEM_VAR_PROP_NAME, &DT_oem_var_property_size);

  if ((FDT_ERR_QC_NOERROR != return_status) || !DT_oem_var_property_size) {
    if ((-FDT_ERR_NOTFOUND) == return_status) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\": Property not found!\n", OEM_VAR_PROP_NAME);
      /* If string OEM-VAR property does not exist, then directly return VALID_CANDIDATE */
      current_entry_data->candidacy_status = VALID_CANDIDATE;
    } else {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Error in fetching Property \"%s\"! Moving to next Blob\n", OEM_VAR_PROP_NAME);
      goto EXIT_POINT (_validate_oem_variant);
    }
  } else {
    /* DT property size should be a multiple of Input property size */
    if (DT_oem_var_property_size % SINGLE_OEM_VARIANT_ELEMENT_SIZE_BYTES) {
      PRINT_LOG_MESSAGE (
                         buffer,
                         BUFFER_SIZE,
                         "DT property size for \"%s\" is not a multiple of Input property size! Moving to next Blob\n",
                         OEM_VAR_PROP_NAME
                         );
      goto EXIT_POINT (_validate_oem_variant);
    }

    DT_oem_var_property = (uint32_t *)MALLOC_WRAPPER (DT_oem_var_property_size);
    if (!DT_oem_var_property) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memory allocation failed\n");
      return MALLOC_FAIL;
    }

    return_status = fdt_get_uint32_prop_list (node, OEM_VAR_PROP_NAME, DT_oem_var_property, DT_oem_var_property_size);
    if (FDT_ERR_QC_NOERROR != return_status) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Error in fetching Property \"%s\"! Moving to next Blob", OEM_VAR_PROP_NAME);
      goto EXIT_POINT (_validate_oem_variant);
    } else {
      for (uint32_t current_property_index = 0;
           current_property_index < (DT_oem_var_property_size / SINGLE_OEM_VARIANT_ELEMENT_SIZE_BYTES);
           current_property_index++)
      {
        if (DT_oem_var_property[current_property_index] == input_parameters->Input_chip_plat_info_prop->oem_var) {
          /* Mark this entry as Valid if Root Node (Packaging) OEM variant matches with Input's OEM variant */
          current_entry_data->candidacy_status = VALID_CANDIDATE;
          break;
        }
      }
    }

    if (VALID_CANDIDATE != current_entry_data->candidacy_status) {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Property value did not match for \"%s\"!\n", OEM_VAR_PROP_NAME);
    }
  }

  EXIT_POINT (_validate_oem_variant) :
    if (DT_oem_var_property) {
    FREE_WRAPPER (DT_oem_var_property, DT_oem_var_property_size);
    DT_oem_var_property = NULL;
  }

  return SUCCESS;
}

/**
  This function validates the current dtbo entry (struct handle_entry_data) and selects best candidate for Platform DTBO.
  The selection is done based on:
    - If Root Node's (Packaging) has OEM variant, then it has to match with Input's OEM variant
    - Root Node's Platforminfo Type should match Input's Platforminfo type
    - Calculating Rank (using Platforminfo Subtype and Chipinfo ID) and Version (using Chipinfo's and Platforminfo's Major and Minor version)
    - Comparing the Rank and Version against best selected candidate for Platform DTBO so far.
  This API gets called only for DTB blob which has Chipinfo Family already matched and hence skipping that check.
  @return
    Returns validity status of the current dtbo entry.
      - If entry is best candidate so far, then it returns VALID_CANDIDATE.
      - If entry is not best candidate, then it returns INVALID_CANDIDATE.
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_validate_and_select_platform_dtbo (
  handle_entry_data     *current_entry_data,
  handle_input_data     *input_parameters,
  handle_selected_dtbo  *platform_dtbo_selected
  )
{
  if (!current_entry_data || !input_parameters || !platform_dtbo_selected ||
      !input_parameters->Input_chip_plat_info_prop)
  {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  /* The comparison is perfomed in following steps:
      1. DT Platform Type (Packaging) must match with Input Platform Type.
         else skip further check and return false.
      2. If DT "oem-var" property exists (Packaging), it should match to Input oem-var (in Input_chip_plat_info_prop);
         else skip further check and return false.
      3. Calculate Rank for the current Platform DTBO blob entry.
      4. Calculate Chipinfo and Platforminfo versions for both DT (Packaging) and Input as well.
      5. Call _platform_dtbo_version_check() to perform version check in following precedence:
          a. Platforminfo Major-Minor Version
          b. Chipinfo     Major-Minor Version
         Perform version check against Input versions and against Selected candidate (Best Platform DTBO entry so far) version.
  */

  /* Default value for selecting this Platform DTBO entry should be False */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /* Platform Type must match for Platform DTBO */
  if (current_entry_data->DT_platforminfo_selected[PLATFORMINFO_TYPE] != input_parameters->Input_chip_plat_info_prop->platform_type) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" Platforminfo Type mismatch! Moving to next Blob\n", PLATFORMINFO_PROP_NAME);
    goto EXIT_POINT (_validate_and_select_platform_dtbo);
  }

  /* If DT "oem-var" property exists, it should match to Input oem-var (in Input_chip_plat_info_prop) */
  int  return_status = _validate_oem_variant (current_entry_data, input_parameters);

  if (SUCCESS != return_status) {
    return FAIL;
  }

  if (VALID_CANDIDATE != current_entry_data->candidacy_status) {
    goto EXIT_POINT (_validate_and_select_platform_dtbo);
  }

  /* Resetting candidacy_status to INVALID_CANDIDATE */
  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /* According to  algorithm, the Rank is calculated based on Chipinfo ID and Platform Sub Type look-up table
      ----------------------------------------------------------
      Case#  | [    Platforminfo Subtype, Chipinfo ID ] = Rank
      ----------------------------------------------------------
      Case#1 : [              Match     ,    Match    ] =  3
      Case#2 : [              Match     , Masked(0x0) ] =  2
      Case#3 : [           Masked(0x0)  ,    Match    ] =  1
      Case#4 : [           Masked(0x0)  , Masked(0x0) ] =  0

      Note: Higher the Rank value, higher the priority
  */

  /* If DT Platforminfo Subtype (Packaging) is Not masked of (a.k.a non zero), then it should match Input's Platforminfo Subtype */
  if (current_entry_data->DT_platforminfo_selected[PLATFORMINFO_SUB_TYPE] &&
      (current_entry_data->DT_platforminfo_selected[PLATFORMINFO_SUB_TYPE] != input_parameters->Input_chip_plat_info_prop->platform_subtype)
      )
  {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" Platforminfo Sub Type mismatch! Moving to next Blob\n", PLATFORMINFO_PROP_NAME);
    goto EXIT_POINT (_validate_and_select_platform_dtbo);
  }

  /* If DT Chipinfo ID (Packaging) is Not masked of (a.k.a non zero), then it should match Input's Chipinfo ID */
  if (current_entry_data->DT_chipinfo_selected[CHIPINFO_ID] &&
      (current_entry_data->DT_chipinfo_selected[CHIPINFO_ID] != input_parameters->Input_chip_plat_info_prop->chip_id)
      )
  {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "\"%s\" Platforminfo Sub Type mismatch! Moving to next Blob\n", PLATFORMINFO_PROP_NAME);
    goto EXIT_POINT (_validate_and_select_platform_dtbo);
  }

  /* Here DT's (Packaging) Platforminfo Subtype and DT's (Packaging) Chipinfo ID is either Masked off(0x0) or Matched to Input's Platforminfo Subtype and
     Input's Chipinfo ID respectively.
     Note: If value for Platforminfo Subtype and/or Chipinfo ID is non-zero, then it has already been verified against its corresponding Input values.

     Rank can be calculated the current Platform DTBO blob entry based on above look-up table.
  */

  /* Case#1: Both Platforminfo Subtype and Chipinfo ID Matches */
  if (current_entry_data->DT_platforminfo_selected[PLATFORMINFO_SUB_TYPE] && current_entry_data->DT_chipinfo_selected[CHIPINFO_ID]) {
    current_entry_data->rank = 0x3;
  }
  /* Case#2: Platforminfo Subtype Matches and Chipinfo ID is Masked off(0x0) */
  else if (current_entry_data->DT_platforminfo_selected[PLATFORMINFO_SUB_TYPE] && (0x0 == current_entry_data->DT_chipinfo_selected[CHIPINFO_ID])) {
    current_entry_data->rank = 0x2;
  }
  /* Case#3: Platforminfo Subtype Masked off(0x0) and Chipinfo ID Matches */
  else if ((0x0 == current_entry_data->DT_platforminfo_selected[PLATFORMINFO_SUB_TYPE]) && current_entry_data->DT_chipinfo_selected[CHIPINFO_ID]) {
    current_entry_data->rank = 0x1;
  }
  /* Case#4: Both Platforminfo Subtype and Chipinfo ID are Masked off(0x0) */
  else {
    current_entry_data->rank = 0x0;
  }

  /* Calculate Integer form of Chipinfo Versions for comparison against Input Chipinfo Version */
  current_entry_data->chipinfo_version_calc = CALCULATE_CHIP_PLAT_INFO_VERSION (
                                                                                current_entry_data->DT_chipinfo_selected[CHIPINFO_MAJOR],
                                                                                current_entry_data->DT_chipinfo_selected[CHIPINFO_MINOR]
                                                                                );
  current_entry_data->platforminfo_version_calc = CALCULATE_CHIP_PLAT_INFO_VERSION (
                                                                                    current_entry_data->DT_platforminfo_selected[PLATFORMINFO_MAJOR],
                                                                                    current_entry_data->DT_platforminfo_selected[PLATFORMINFO_MINOR]
                                                                                    );

  /* Call _platform_dtbo_version_check() to perform version check in following precedence:
       a. Platforminfo Major-Minor Version
       b. Chipinfo     Major-Minor Version
       Perform version check against Input versions and against Selected candidate (Best Platform DTBO entry so far) version.
     If a valid entry is found, then this funtion will return candidacy_status as VALID_CANDIDATE
  */
  if (SUCCESS != _platform_dtbo_version_check (current_entry_data, input_parameters, platform_dtbo_selected)) {
    return FAIL;
  }

  EXIT_POINT (_validate_and_select_platform_dtbo) :
    return SUCCESS;
}

/**
  This function validates Version number (Major and Minor) betwee
  @return
    Returns validity status of the current dtbo entry.
      - If entry is best candidate so far, then it returns VALID_CANDIDATE.
      - If entry is not best candidate, then it returns INVALID_CANDIDATE.
    Returns FAIL if any operations fails, else return SUCCESS
*/
static int
_platform_dtbo_version_check (
  handle_entry_data     *current_entry_data,
  handle_input_data     *input_parameters,
  handle_selected_dtbo  *platform_dtbo_selected
  )
{
  /* The comparison is perfomed in following steps:
      1. Both Chipinfo and Platforminfo version (Packaging) should not be greater than
          device's (Input) Chipinfo and Platforminfo version respectively
      2. Rank of already selected Blob entry should not be greater than current rank.
      3. Perform version check in following precedence:
          a. Platforminfo Major-Minor Version
          b. Chipinfo     Major-Minor Version
  */
  if (!current_entry_data || !input_parameters || !platform_dtbo_selected) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  current_entry_data->candidacy_status = INVALID_CANDIDATE;

  /* Version Bound check: The version(s) of the Current entry should not be greater than Input's version(s).

     If Platforminfo version or Chipinfo version (Packaging) is greater than Input corresponding version,
      then skip further check and return false.
  */
  if ((current_entry_data->platforminfo_version_calc > input_parameters->platforminfo_version_calc) ||
      (current_entry_data->chipinfo_version_calc > input_parameters->chipinfo_version_calc)
      )
  {
    current_entry_data->candidacy_status = INVALID_CANDIDATE;
    goto EXIT_POINT (_platform_dtbo_version_check);
  }

  /* If Rank of selected entry is higher than Rank of current, skip further check and return false. */
  if (platform_dtbo_selected->rank > current_entry_data->rank) {
    current_entry_data->candidacy_status = INVALID_CANDIDATE;
    goto EXIT_POINT (_platform_dtbo_version_check);
  } else if (platform_dtbo_selected->rank == current_entry_data->rank) {
    /* If Rank of selected entry is EQUAL to Rank of current entry, then perform tie breaker in following precedence:
       I. Platforminfo version:
          - If Platforminfo version of Current is greater than Selected version, then mark this entry as Valid.
          - If Platforminfo version of Current is less than Selected version, then mark this entry as Invalid.
          - If Platforminfo version of Current is equal to Selected version, then check Chipinfo version.
            i. Chipinfo version:
               - If Chipinfo version of Current is greater than Selected version, then mark this entry as Valid.
               - If Chipinfo version of Current is equal to Selected version, then mark this entry as Invalid. We will
                 skip the entry with Exact same Versions (for both Platforminfo version and Chipinfo version)
               - If Chipinfo version of Current is less than Selected version, then mark this entry as Invalid.
    */
    if (current_entry_data->platforminfo_version_calc > platform_dtbo_selected->platforminfo_version_calc) {
      /* Return Valid if Platform version is greater than Platform version of selected entry blob. */
      current_entry_data->candidacy_status = VALID_CANDIDATE;
    }
    /* Perform Chipinfo version check if Platforminfo version matches */
    else if (current_entry_data->platforminfo_version_calc == platform_dtbo_selected->platforminfo_version_calc) {
      if (current_entry_data->chipinfo_version_calc > platform_dtbo_selected->chipinfo_version_calc) {
        current_entry_data->candidacy_status = VALID_CANDIDATE;
      } else if (current_entry_data->chipinfo_version_calc <= platform_dtbo_selected->chipinfo_version_calc) {
        /* Note: We skip second entry with exact same Versions.
        The first entry (with exact same versions) in the list is picked.
        */
        current_entry_data->candidacy_status = INVALID_CANDIDATE;
      }
    } else {
      /* current_entry_data->platforminfo_version_calc < platform_dtbo_selected->platforminfo_version_calc */

      /* If Platforminfo version of Current is less than Selected version, then mark this entry as Invalid. */
      current_entry_data->candidacy_status = INVALID_CANDIDATE;
    }
  } else {
    /* platform_dtbo_selected->rank < current_entry_data->rank */

    /* If Rank of Selected entry is less than current entry, then mark this exntry as Valid */
    current_entry_data->candidacy_status = VALID_CANDIDATE;
  }

  EXIT_POINT (_platform_dtbo_version_check) :
    return SUCCESS;
}

/**
  Function parses the raw dtb elf and selects the best base dtb, soc overlay and platform overlay blobs.
    @param[out] handle              Allocate & return handle data to keep track of instance and its DTB/DTBO data
    @param[in]  dtbs_image_start_address Address pointing to start of raw dtb
    @param[in]  dtbs_image_size     Size for entire dtb/dtbo elf.
                                    This argument can be set to 0x0.
    @param[in]  Input_chip_plat_info_prop Collection of Chipinfo, Platforminfo & compatible string parameters that
                                    will be required to find DTB and DTBOs
    @param[in]  Input_prop_list        Collection of user defined parameters that will be compared to find DTB and DTBOs
    @param[in]  prop_num_entries    Number of properties in Input_prop_list
      @param[out] base_dtb_size       Return size of base Device Tree Blob
    @param[out] soc_dtbo_size       Return size of SOC Device Tree Overlay
    @param[out] plat_dtbo_size      Return size of Platform Device Tree Overlay
  @return
    Returns -1 if any operations fails, else return 0 on success
*/
int
dt_select_open (
  uintptr_t                *handle,
  uintptr_t                dtbs_image_start_address,
  size_t                   dtbs_image_size,
  chip_plat_info_property  *Input_chip_plat_info_prop,
  property_list            *Input_prop_list,
  unsigned int             prop_num_entries,
  size_t                   *base_dtb_size,
  size_t                   *soc_dtbo_size,
  size_t                   *plat_dtbo_size
  )
{
  int  return_status = 0;

  if ( !dtbs_image_start_address ||
       !Input_chip_plat_info_prop ||
       (Input_prop_list && !prop_num_entries) ||
       (!Input_prop_list && prop_num_entries) ||
       !base_dtb_size ||
       !soc_dtbo_size ||
       !plat_dtbo_size
       )
  {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  handle_data  *handle_ptr = (handle_data *)MALLOC_WRAPPER (sizeof (handle_data));

  if (!handle_ptr) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memory allocation failed\n");
    return MALLOC_FAIL;
  }

  return_status = _init_handle_data (handle_ptr, dtbs_image_start_address);
  if (SUCCESS != return_status) {
    goto EXIT_POINT (dt_select_open);
  }

  /* Structure for handling data for client's Input data.
     This struct holds all the input parameters that Client provides when open() is called
  */
  handle_input_data  input_parameters;

  return_status = _init_handle_input_data (
                                           dtbs_image_size,
                                           &input_parameters,
                                           Input_chip_plat_info_prop,
                                           Input_prop_list,
                                           prop_num_entries
                                           );
  if (SUCCESS != return_status) {
    goto EXIT_POINT (dt_select_open);
  }

  return_status = _select_dtb_and_dtbos (
                                         handle_ptr,
                                         &input_parameters
                                         );
  if (SUCCESS != return_status) {
    goto EXIT_POINT (dt_select_open);
  }

  return_status = _print_selected_dtb_dtbos (handle_ptr, &input_parameters);
  if (SUCCESS != return_status) {
    goto EXIT_POINT (dt_select_open);
  }

  *base_dtb_size  = handle_ptr->base_dtb_size;
  *soc_dtbo_size  = handle_ptr->soc_dtbo_size;
  *plat_dtbo_size = handle_ptr->plat_dtbo_size;
  *handle         = (uintptr_t)handle_ptr;

  EXIT_POINT (dt_select_open) :
    if ((SUCCESS != return_status) && (NULL != handle_ptr)) {
    FREE_WRAPPER ((void *)handle_ptr, sizeof (handle_data));
    handle_ptr = NULL;
  }

  return return_status;
}

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
int
dt_select_read_blob (
  uintptr_t       handle,
  enum blob_type  read_blob_type,
  uintptr_t       *blob_ptr,
  size_t          blob_size
  )
{
  handle_data  *handle_ptr   = (handle_data *)handle;
  int          return_status = SUCCESS;

  if (!handle_ptr || !blob_ptr || (SUCCESS != _handle_check (handle_ptr))) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return_status = INVALID_PARAMETER;
    goto exit;
  }

  uintptr_t        address = 0x0;
  size_t           size    = 0x0;
  fdt_node_handle  node;

  _init_fdt_node_handle (&node);

  switch (read_blob_type) {
    case BASE_DTB:
      address = handle_ptr->base_dtb_ptr;
      size    = handle_ptr->base_dtb_size;
      break;

    case SOC_DTBO:
      address = handle_ptr->soc_dtbo_ptr;
      size    = handle_ptr->soc_dtbo_size;
      break;

    case PLATFORM_DTBO:
      address = handle_ptr->plat_dtbo_ptr;
      size    = handle_ptr->plat_dtbo_size;
      break;

    default:
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Reading blob error. Blob Type not supported\n");
      return_status = READ_BLOB_TYPE_MISMATCH;
      goto exit;
  }

  if (size && (blob_size == size)) {
    node.blob     = (const void *)address;
    return_status = fdt_get_blob_size (&node, (uint32_t *)&size);
    if (return_status) {
      goto exit;
    }

    if (size == blob_size) {
      /* Point to base blob if Null address is passed */
      if (0x0 == (*blob_ptr)) {
        *blob_ptr = address;
      } else {
        if (MEMCPY_WRAPPER ((void *)*blob_ptr, blob_size, (void *)address, size) != size) {
          PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Memcpy failed\n");
          return_status = MEMCOPY_FAIL;
        }
      }
    } else {
      PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Reading blob error. Size mismatch\n");
      return_status = READ_BLOB_SIZE_MISMATCH;
    }
  } else {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Reading blob error. Size mismatch\n");
    return_status = READ_BLOB_SIZE_MISMATCH;
  }

exit:
  return return_status;
}

/**
  Function to free handle data
    @param[in]  handle              Instance of Handle data
  @return
    Returns -1 if any operations fails, else return 0 on success
*/
int
dt_select_close (
  uintptr_t  handle
  )
{
  if (!handle || (SUCCESS != _handle_check ((handle_data *)handle))) {
    PRINT_LOG_MESSAGE (buffer, BUFFER_SIZE, "Wrong parameter passed to %s\n", __func__);
    return INVALID_PARAMETER;
  }

  FREE_WRAPPER ((void *)handle, sizeof (handle_data));
  return SUCCESS;
}
