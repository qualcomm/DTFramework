// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear


#ifndef DT_SELECT_INTERNAL_H
#define DT_SELECT_INTERNAL_H

#include "Environment/dt_select_env.h"
#include "libfdt.h"
#include "DTBExtnLib.h"
#include "dt_select.h"

#define DT_SELECT_LIBRARY_ID  0x445453454c454354

#define SUCCESS                      0
#define FAIL                         -1
#define MALLOC_FAIL                  -2
#define INVALID_PARAMETER            -3
#define OVERFLOWING_BLOB             -4
#define INVALID_BLOB_ENTRY           -5
#define GET_ROOT_NODE_FAILED         -6
#define FDT_HEADER_CHECK_FAIL        -7
#define POINTER_ARITHMETIC_OVERFLOW  -8
#define UINT32_ARITHMETIC_OVERFLOW   -9
#define READ_BLOB_TYPE_MISMATCH      -10
#define MEMCOPY_FAIL                 -11
#define READ_BLOB_SIZE_MISMATCH      -12
#define INVALID_RETURN_PTR           -13

#define BUFFER_SIZE  200

/* Chipinfo = /bits/ 16 <<Family> <ID> <Major> <MINOR>>*/
#define CHIPINFO_FAMILY                     0
#define CHIPINFO_ID                         1
#define CHIPINFO_MAJOR                      2
#define CHIPINFO_MINOR                      3
#define MAX_CHIPINFO_SIZE                   4
#define SINGLE_CHIPINFO_ELEMENT_SIZE_BYTES  sizeof(uint16_t)

/* board-info = /bits/ 8 <<Type> <Subtype> <Major> <MINOR>>*/
#define PLATFORMINFO_TYPE                       0
#define PLATFORMINFO_SUB_TYPE                   1
#define PLATFORMINFO_MAJOR                      2
#define PLATFORMINFO_MINOR                      3
#define MAX_PLATFORMINFO_SIZE                   4
#define SINGLE_PLATFORMINFO_ELEMENT_SIZE_BYTES  sizeof(uint8_t)

/* oem-var is an array of 32-bit integers */
#define SINGLE_OEM_VARIANT_ELEMENT_SIZE_BYTES  sizeof(uint32_t)

#define SINGLE_CHIPINFO_SIZE_IN_BYTES      (MAX_CHIPINFO_SIZE * SINGLE_CHIPINFO_ELEMENT_SIZE_BYTES)
#define SINGLE_PLATFORMINFO_SIZE_IN_BYTES  (MAX_PLATFORMINFO_SIZE * SINGLE_PLATFORMINFO_ELEMENT_SIZE_BYTES)

#define MAJOR_INDEX  0
#define MINOR_INDEX  1

#define VALID_CANDIDATE    true
#define INVALID_CANDIDATE  false

#define SAFE    true
#define UNSAFE  false

#define IN_RANGE      true
#define OUT_OF_RANGE  false

#define BYTE_SIZE_TAG   "/bits/ 8"
#define WORD_SIZE_TAG   "/bits/ 16"
#define DWORD_SIZE_TAG  ""      /* By Default "/bits/ 32" can be left blank */
#define QWORD_SIZE_TAG  "/bits/ 64"

/* Properties names for chip-info, board-info, is_overlay, compatible & oem-var */
#define ROOT_NODE               "/board-id/"
#define ROOT_NODE_NAME          "board-id"
#define IS_OVERLAY_PROP_NAME    "is-overlay"
#define CHIPINFO_PROP_NAME      "chip-info"
#define PLATFORMINFO_PROP_NAME  "board-info"
#define COMPATIBLE_PROP_NAME    "proc-name"
#define OEM_VAR_PROP_NAME       "oem-var"

#define BYTE_SIZE_BYTES   1
#define WORD_SIZE_BYTES   2
#define DWORD_SIZE_BYTES  4
#define QWORD_SIZE_BYTES  8

#define BITS_IN_BYTE  (CHAR_BIT)
#define CALCULATE_CHIP_PLAT_INFO_VERSION(a, b)  ((a) << (sizeof((a)) * BITS_IN_BYTE) | (b))

/* DEFAULT_LOG_MESSAGE is not dependent on Flag DT_SELECT_LOGGING_ENABLE
   PRINT_LOG_MESSAGE is dependent on Flag DT_SELECT_LOGGING_ENABLE.
*/
#define DEFAULT_LOG_MESSAGE(buffer, size, str, ...)  do{\
                    SNPRINTF_WRAPPER(buffer, size, str, ##__VA_ARGS__); \
                    LOG_MESSAGE_WRAPPER (buffer); \
                  }while(0)

#ifdef DT_SELECT_LOGGING_ENABLE
#define PRINT_LOG_MESSAGE(buffer, size, str, ...) \
                    DEFAULT_LOG_MESSAGE(buffer, size, str, ##__VA_ARGS__)
#else
#define PRINT_LOG_MESSAGE(buffer, size, str, ...)
#endif

#define EXIT_POINT(func_name)  _##func_name##_exit

/* Marking datatype for Major and Minor version in Integer format to be 4 Byte value.
   Currently Chipinfo Versions and Platforminfo Versions are of 2 Bytes and 1 Byte resp.
   So 4 Byte datatype is sufficient for storing Integer format for both Versions(Chipinfo
   and Platforminfo)
*/
typedef uint32_t version_datatype;

/* Structure for handle data.
   This structure stores variables for pointer and size for all selected DT blobs, i.e.:
   - BASE DTB
   - SOC DTBO
   - PLATFORM DTBO
   This handle is allocated and freed for every open() and close() respectively.
*/
typedef struct {
  unsigned long long    data_guard;
  uintptr_t        dtbs_image_start_address;
  uintptr_t        base_dtb_ptr;
  size_t           base_dtb_size;
  uintptr_t        soc_dtbo_ptr;
  size_t           soc_dtbo_size;
  uintptr_t        plat_dtbo_ptr;
  size_t           plat_dtbo_size;
} handle_data;

/* Structure for handling data for client's Input data.
   This struct holds all the input parameters that Client provides when open() is called
*/
typedef struct {
  size_t                     dtbs_image_size;
  chip_plat_info_property    *Input_chip_plat_info_prop;
  property_list              *Input_prop_list;
  unsigned int               prop_num_entries;
  version_datatype           chipinfo_version_calc;
  version_datatype           platforminfo_version_calc;
} handle_input_data;

/* Structure for handling data for every dtb/dtbo entry in the image.
   This struct holds the relevant/required data for every entry's Root Node; that
   this algorithm will need to determine for  selecting best candidates for DTB and DTBO(s).
*/
typedef struct {
  fdt_node_handle    node;
  size_t             device_tree_blob_size;
  enum blob_type     device_tree_type;
  uint16_t           DT_chipinfo_selected[MAX_CHIPINFO_SIZE];
  uint8_t            DT_platforminfo_selected[MAX_PLATFORMINFO_SIZE];
  int8_t             rank;
  bool               candidacy_status;

  /* The variable ***_version_calc will store Versions (for chipinfo and platforminfo) in Integer format.
     The Versions are comprised of 2 values: Major and Minor.

     The version are stored here by shifting the Major version by size of respective datatype (16 bits
     for Chipinfo and 8 bits for Platforminfo Major versions) and then ORing with Minor version.
     (See CALCULATE_CHIP_PLAT_INFO_VERSION Macro above for reference)

     For example:
     1. if Chipinfo version is 3.2, then chipinfo_version_calc = 0x030002.
     2. if Platforminfo version is 3.2, then platforminfo_version_calc = 0x0302.

     The size for both variables ***_version_calc is 4 Bytes.
  */
  version_datatype    chipinfo_version_calc;
  version_datatype    platforminfo_version_calc;
} handle_entry_data;

/* Structure for handling data for best selected DTBO candidates.
   When DT Select algorithm goes across all the dtb/dtbo entries, this struct holds all relevant data
   for the best selected SOC and Platform DTBO candidate(s).
*/
typedef struct {
  int8_t              rank;
  size_t              device_tree_blob_size;
  version_datatype    chipinfo_version_calc;
  version_datatype    platforminfo_version_calc;
} handle_selected_dtbo;

/* DT Select environment wrapper function declaration */

/* ============================================================================
**  Function : MALLOC_WRAPPER
** ============================================================================
*/
void *
MALLOC_WRAPPER (
  size_t  size
  );

/* ============================================================================
**  Function : FREE_WRAPPER
** ============================================================================
*/
void
FREE_WRAPPER (
  void    *buffer_ptr,
  size_t  size
  );

/* ============================================================================
**  Function : LOG_MESSAGE_WRAPPER
** ============================================================================
*/
void
LOG_MESSAGE_WRAPPER (
  char  *buffer_ptr
  );

/* ============================================================================
**  Function : MEMCPY_WRAPPER
** ============================================================================
*/
size_t
MEMCPY_WRAPPER (
  void        *destination_ptr,
  size_t      destination_size,
  const void  *source_ptr,
  size_t      source_size
  );

/* ============================================================================
**  Function : SNPRINTF_WRAPPER
** ============================================================================
*/
size_t
SNPRINTF_WRAPPER (
  char        *buffer,
  size_t      size,
  const char  *str,
  ...
  );

#endif /* DT_SELECT_INTERNAL_H */
