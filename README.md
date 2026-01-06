# DTFwk - Device Tree Framework

Extended Device Tree Blob (DTB) library and DT Selection library for Qualcomm platforms, built on top of FdtLib (edk2).

This project provides enhanced DTB manipulation capabilities and device tree selection functionality for Qualcomm platforms, supporting device tree creation, parsing, modification, property operations, node management, overlay merging, and intelligent DTB/DTBO selection. It runs on Qualcomm® Snapdragon™ processor platforms.

## Project Structure

```
DTFwk/
├── DTBExtnLib/                    # Extended DTB library
│   ├── inc/                       # Public header files
│   │   ├── create-dtb-apis.h      # DTB creation API header
│   │   ├── DTBExtnLib_env.h       # Environment configuration header
│   │   └── DTBInternals.h         # Internal definitions header
│   └── src/                       # Source files
│       ├── DTBExtnLib_blob.c      # Blob management functions
│       ├── DTBExtnLib_create-dtb.c # DTB creation functions
│       ├── DTBExtnLib_driver.c    # Driver-related functions
│       ├── DTBExtnLib_node.c      # Node operation functions
│       ├── DTBExtnLib_overlay.c   # Overlay merge functions
│       └── DTBExtnLib_prop.c      # Property operation functions
├── DTSelectLib/                   # DT Selection library
│   ├── inc/                       # Internal header files
│   │   ├── dt_select_env.h        # Environment configuration
│   │   ├── dt_select_internal.h   # Internal definitions
│   │   └── README.txt             # Library documentation
│   └── src/                       # Source files
│       ├── dt_select.c            # DT selection algorithm
│       ├── dt_select_env.c        # Environment wrapper functions
│       └── get_dt.c               # Wrapper APIs for DT selection
├── inc/                           # Public API headers
│   ├── DTBExtnLib.h               # DTB extension library API
│   ├── dt_select.h                # DT selection library API
│   └── get_dt.h                   # DT selection wrapper API
├── tool/                          # Utility tools
│   └── parse_dtb_log_script.py    # DTB log parsing script
├── CODE-OF-CONDUCT.md             # Code of Conduct
├── CONTRIBUTING.md                # Contribution guidelines
├── LICENSE.txt                    # BSD-3-Clause License
├── README.md                      # This file
└── SECURITY.md                    # Security policy
```

## Branches

**main**: Primary development branch. Contributors should develop submissions based on this branch and submit pull requests to this branch.

## Components

### DTBExtnLib - Extended DTB Library

Enhanced DTB manipulation library providing comprehensive device tree operations.

#### Core Functionality Modules

##### 1. Blob Management (DTBExtnLib_blob.c)
- `fdt_check_for_valid_blob()` - Validate DTB blob integrity
- `fdt_set_blob_handle()` - Set blob handle
- `fdt_get_blob_handle()` - Get blob handle
- `fdt_get_blob_size()` - Get blob size
- `fdt_init_root_handle_for_driver()` - Initialize root handle for driver
- `fdt_init_root_handle_for_driver_by_id()` - Initialize root handle for driver by ID

##### 2. DTB Creation (DTBExtnLib_create-dtb.c)
- `fdt_add_tree()` - Create a new empty device tree
- `fdt_add_node()` - Create a new node
- `fdt_add_prop_list_u8()` - Add u8 type property list
- `fdt_add_prop_list_u16()` - Add u16 type property list
- `fdt_add_prop_list_u32()` - Add u32 type property list
- `fdt_add_prop_list_u64()` - Add u64 type property list
- `fdt_add_prop_list_string()` - Add string type property list

##### 3. Driver Functions (DTBExtnLib_driver.c)
- `fdt_get_next_node_handle_for_compatible()` - Get next node handle by compatible property
- `fdt_get_name_index()` - Get name index
- `fdt_get_reg()` - Get reg property

##### 4. Node Operations (DTBExtnLib_node.c)
- `fdt_get_node_handle()` - Get node handle
- `fdt_get_phandle_node()` - Get node by phandle
- `fdt_get_parent_node()` - Get parent node
- `fdt_node_cmp()` - Compare nodes
- `fdt_node_copy()` - Copy node
- `fdt_get_prop_values_size_of_node()` - Get node property values size
- `fdt_get_prop_values_of_node()` - Get node property values
- `fdt_get_count_of_subnodes()` - Get subnode count
- `fdt_get_subnode_names()` - Get subnode names
- `CreatDtbHashTable()` - Create DTB hash table for fast lookup

##### 5. Overlay Merge (DTBExtnLib_overlay.c)
- `fdt_merge_overlay()` - Merge primary DTB and overlay DTB

##### 6. Property Operations (DTBExtnLib_prop.c)
- `fdt_get_prop_size()` - Get property size
- `fdt_get_uint8_prop()` / `fdt_get_uint8_prop_list()` - Get u8 properties
- `fdt_get_uint16_prop()` / `fdt_get_uint16_prop_list()` - Get u16 properties
- `fdt_get_uint32_prop()` / `fdt_get_uint32_prop_list()` - Get u32 properties
- `fdt_get_uint64_prop()` / `fdt_get_uint64_prop_list()` - Get u64 properties
- `fdt_get_boolean_prop()` / `fdt_get_bool_prop()` - Get boolean properties
- `fdt_get_string_prop_list()` - Get string property list

### DTSelectLib - DT Selection Library

Intelligent device tree selection library that automatically selects the best matching DTB and DTBOs based on platform information.

#### Core Functionality

##### DT Selection Algorithm (dt_select.c)
- `dt_select_open()` - Parse DTB/DTBO image and select best matching blobs
- `dt_select_read_blob()` - Read selected DTB/DTBO blob
- `dt_select_close()` - Close DT selection handle

##### Wrapper APIs (get_dt.c)
- `get_dt()` - High-level API to get final DTB with overlays applied
- `get_dt_free()` - Free DTB memory allocated by get_dt()

#### Selection Criteria

The DT selection algorithm selects the best matching DTB and DTBOs based on:
- **Chip Information**: Chip family, ID, major/minor version
- **Platform Information**: Platform type, subtype, major/minor version
- **OEM Variant**: OEM-specific variant identifier
- **Compatible Strings**: Device tree compatible property matching
- **Custom Properties**: User-defined property matching

#### Selection Process

1. **Base DTB Selection**: Selects base DTB with matching chip family and version 1.0
2. **SOC DTBO Selection**: Selects SOC overlay based on chip ID and version ranking
3. **Platform DTBO Selection**: Selects platform overlay based on platform type, subtype, chip ID, and version ranking

## Supported Platforms

This library supports the following target platforms and architectures:
- **TARGET_UEFI** - UEFI environment
- **TARGET_XBL** - XBL (eXtensible Boot Loader) environment
- **PORT_ARMv8** - ARMv8 architecture
- **PORT_Q6** - Qualcomm Hexagon DSP

## Requirements

- Qualcomm platform SDK (select appropriate version based on target platform)
- Supported build environments:
  - UEFI development environment (for UEFI targets)
  - XBL development environment (for XBL targets)
  - Hexagon SDK (for Q6 targets)
- Device Tree Compiler (dtc) - for DTB file processing

## Integration Guide

**IMPORTANT**: Only include header files (.h) in your code, never include source files (.c).

### For DTBExtnLib

```c
#include "DTBExtnLib.h"          // For DTB manipulation APIs
#include "create-dtb-apis.h"     // For DTB creation APIs
```

Add `DTBExtnLib/inc/` directory to your compiler's include path and compile all `.c` files in `DTBExtnLib/src/` as part of your build.

### For DTSelectLib

```c
#include "dt_select.h"           // For DT selection APIs
#include "get_dt.h"              // For high-level wrapper APIs
```

Add `inc/` directory to your compiler's include path and compile all `.c` files in `DTSelectLib/src/` as part of your build.

## Usage Examples

### DTBExtnLib Examples

#### Creating a New Device Tree

```c
#include "create-dtb-apis.h"

fdt_node_handle node;
int offset;

// Create a new device tree with 4096 bytes
fdt_add_tree(&node, 4096);

// Add a new node
fdt_add_node(&node, "/soc/device@0", &offset);

// Add properties
uint32_t reg_values[] = {0x1000, 0x100};
fdt_add_prop_list_u32(&node, "reg", reg_values, 2);

char *compatible = "vendor,device";
fdt_add_prop_list_string(&node, "compatible", compatible, strlen(compatible) + 1);
```

#### Reading Device Tree Properties

```c
#include "DTBExtnLib.h"

fdt_node_handle node;
uint32_t value;
int ret;

// Get node handle
ret = fdt_get_node_handle(&node, "/soc/device@0");

// Read u32 property
ret = fdt_get_uint32_prop(&node, "reg", &value);

// Read string property
char compatible[64];
ret = fdt_get_string_prop_list(&node, "compatible", compatible, sizeof(compatible));
```

#### Merging Overlay

```c
#include "DTBExtnLib.h"

void *primary_blob;    // Primary DTB
void *overlay_blob;    // Overlay DTB
void *merge_blob;      // Merged DTB
size_t pb_size, ob_size, mb_size;

// Merge overlay into primary DTB
int ret = fdt_merge_overlay(primary_blob, pb_size, 
                            overlay_blob, ob_size,
                            merge_blob, mb_size);
```

### DTSelectLib Examples

#### Using DT Selection API

```c
#include "dt_select.h"

uintptr_t handle;
uintptr_t dtbs_image_addr = /* address of appended DTB/DTBO image */;
size_t dtbs_image_size = /* size of image */;
size_t base_dtb_size, soc_dtbo_size, plat_dtbo_size;

// Setup chip and platform information
chip_plat_info_property chip_plat_info = {
    .chip_family = 0x1234,
    .chip_id = 0x5678,
    .chip_maj_version = 1,
    .chip_min_version = 0,
    .platform_type = 0x01,
    .platform_subtype = 0x02,
    .platform_maj_version = 1,
    .platform_min_version = 0,
    .oem_var = 0,
    .dtb_compatible_string_starts_with = "qcom,",
    .soc_dtbo_compatible_string_starts_with = "qcom,soc-",
    .plat_dtbo_compatible_string_starts_with = "qcom,board-"
};

// Open and select DTB/DTBOs
int ret = dt_select_open(&handle, dtbs_image_addr, dtbs_image_size,
                        &chip_plat_info, NULL, 0,
                        &base_dtb_size, &soc_dtbo_size, &plat_dtbo_size);

// Read selected base DTB
uintptr_t base_dtb_ptr = 0;
ret = dt_select_read_blob(handle, BASE_DTB, &base_dtb_ptr, base_dtb_size);

// Close handle
dt_select_close(handle);
```

#### Using High-Level Wrapper API

```c
#include "get_dt.h"

uintptr_t dtbs_image_addr = /* address of appended DTB/DTBO image */;
size_t dtbs_image_size = /* size of image */;
uintptr_t final_dtb_addr;
size_t final_dtb_size;

// Setup chip and platform information
chip_plat_info_property chip_plat_info = {
    .chip_family = 0x1234,
    .chip_id = 0x5678,
    .chip_maj_version = 1,
    .chip_min_version = 0,
    .platform_type = 0x01,
    .platform_subtype = 0x02,
    .platform_maj_version = 1,
    .platform_min_version = 0,
    .oem_var = 0,
    .dtb_compatible_string_starts_with = "qcom,",
    .soc_dtbo_compatible_string_starts_with = "qcom,soc-",
    .plat_dtbo_compatible_string_starts_with = "qcom,board-"
};

// Get final DTB with all overlays applied
int ret = get_dt(dtbs_image_addr, dtbs_image_size,
                &chip_plat_info, NULL, 0,
                &final_dtb_addr, &final_dtb_size, POST);

// Use the final DTB...

// Free memory when done
get_dt_free(final_dtb_addr, final_dtb_size);
```

## Features

### DTBExtnLib Features
- **High-Performance Node Lookup**: Uses hash tables for fast node lookup
- **Type-Safe Property Operations**: Provides typed property read/write APIs
- **Overlay Support**: Supports DTB overlay merge operations
- **Multi-Platform Support**: Supports UEFI, XBL, ARMv8, and Hexagon DSP platforms
- **Debug Support**: Configurable logging and debug functionality
- **Memory Management**: Optimized memory allocation and caching mechanisms

### DTSelectLib Features
- **Intelligent Selection**: Automatically selects best matching DTB and DTBOs
- **Ranking Algorithm**: Uses sophisticated ranking based on chip/platform versions
- **Flexible Matching**: Supports chip family, ID, platform type/subtype matching
- **OEM Variant Support**: Handles OEM-specific device tree variants
- **Compatible String Matching**: Supports compatible property prefix matching
- **Custom Property Matching**: Allows user-defined property matching criteria
- **Automatic Overlay Application**: Applies SOC and platform overlays automatically

## Development

### Build Configuration

Define appropriate macros based on target platform during compilation:

#### DTBExtnLib Build Macros

```bash
# UEFI platform
-DTARGET_UEFI

# XBL platform
-DTARGET_XBL

# ARMv8 architecture
-DPORT_ARMv8

# Hexagon DSP
-DPORT_Q6

# Enable debug output
-DINSTRUMENTATION

# Enable hash table for fast node lookup (recommended for large device trees)
-DENABLE_HASH_FOR_DTB

# Enable double hash to reduce hash collisions (optional, requires ENABLE_HASH_FOR_DTB)
# -DENABLE_DOUBLE_HASH_FOR_DTB

# Enable DTB profiling for performance analysis (optional)
# -DENABLE_DTB_PROFILING
```

#### DTSelectLib Build Macros

```bash
# Enable DT selection logging
-DDT_SELECT_LOGGING_ENABLE

# Build for x86 (for testing/simulation)
-DBUILD_X86
```

#### Hash Table Optimization (ENABLE_HASH_FOR_DTB)

When `ENABLE_HASH_FOR_DTB` is defined, the library automatically creates a hash table to significantly improve node lookup performance:

- **Performance**: Dramatically reduces node search time, especially beneficial for large device trees
- **Algorithm**: Uses DJB2 hash algorithm for fast hash computation
- **Memory**: Requires additional memory (~40 bytes per node) to store hash table entries
- **Automatic Creation**: Hash table is automatically built when calling `fdt_set_blob_handle()`
- **Phandle Cache**: Also creates a phandle-to-node-offset cache for faster phandle lookups

**Optional enhancements**:
- `ENABLE_DOUBLE_HASH_FOR_DTB`: Adds FNV-64 as a second hash function to reduce collision probability (requires `ENABLE_HASH_FOR_DTB`)
- `ENABLE_DTB_PROFILING`: Enables performance profiling to analyze hash table efficiency and node access patterns

**Example**:
```c
#include "DTBExtnLib.h"

void *dtb_blob;
size_t blob_size;
int blob_id = 0;

// Set blob handle - hash table is automatically created if ENABLE_HASH_FOR_DTB is defined
int ret = fdt_set_blob_handle(dtb_blob, blob_size, blob_id);
if (ret == FDT_ERR_QC_NOERROR) {
    // Blob handle set successfully
    // Hash table has been created automatically
    // Subsequent node lookups will be much faster
}
```

**Note**: Hash table creation is automatic and happens once per DTB when setting the blob handle. For merged DTBs, the hash table will be automatically disabled to avoid conflicts.

### Contributing

We welcome contributions to this project! Please follow these steps:

1. Read our [Code of Conduct](CODE-OF-CONDUCT.md) and [License](LICENSE.txt)
2. Fork and clone the repository
3. Create a new branch based on `main`
4. Make your changes and ensure they follow the coding standards
5. Commit your changes using the [DCO](https://developercertificate.org/) with `-s` or `--signoff` option
6. Push to your fork and submit a pull request

For detailed contribution guidelines, please see [CONTRIBUTING.md](CONTRIBUTING.md).

### Coding Standards

- Follow existing code style
- All public APIs must have complete documentation comments
- Use provided error checking macros (PTR_CHECK, BLOBID_CHECK, etc.)
- Ensure code compiles without warnings before submitting

### Security Analysis

All pull requests from external contributors are automatically scanned using [Semgrep](https://github.com/semgrep/semgrep) to detect insecure coding patterns and potential security flaws. Contributors are expected to resolve any flagged issues before the PR can be merged.

## Error Handling

### DTBExtnLib Error Codes

The library uses standard FDT error codes and extends them with the following Qualcomm-specific error codes:
- `FDT_ERR_QC_NULLPTR` - Null pointer error
- `FDT_ERR_QC_BLOBID` - Invalid blob ID
- `FDT_ERR_QC_FDTLIB_ERROR` - FDT library error

### DTSelectLib Error Codes

- `SUCCESS` (0) - Operation successful
- `FAIL` (-1) - General failure
- `MALLOC_FAIL` (-2) - Memory allocation failed
- `INVALID_PARAMETER` (-3) - Invalid parameter
- `GET_DT_ERR_NONE` (0) - No error
- `GET_DT_ERR_INVALID_PARAMETER` (1) - Invalid parameter
- `GET_DT_ERR_HANDLE_RETURN_ERROR` (2) - Handle return error
- `GET_DT_ERR_INVALID_BASE_DTB_SIZE` (3) - Invalid base DTB size
- `GET_DT_ERR_MALLOC_FAIL` (4) - Memory allocation failed
- `GET_DT_ERR_SIZE_OVERFLOW` (5) - Size overflow
- `GET_DT_ERR_INVALID_BASE_DTB_PTR` (6) - Invalid base DTB pointer

## Tools

### parse_dtb_log_script.py

Python script for parsing and analyzing DTB Enable hash table and enable ENABLE_DTB_PROFILING logs.

## Getting in Contact

For questions or suggestions, please contact us through:

* [Report an Issue on GitHub](../../issues)
* [Open a Discussion on GitHub](../../discussions)

## Security

For reporting security vulnerabilities, please see [SECURITY.md](SECURITY.md). You can also contact our [Product Security team](mailto:product-security@qualcomm.com).

## License

DTFwk is licensed under the [BSD-3-Clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.

## Acknowledgments

This project is built as an extension of EDK2's FdtLib. We thank all contributors who have helped make this project better.

### Related Projects

- **Device Tree Compiler (dtc)**: [https://github.com/dgibson/dtc](https://github.com/dgibson/dtc) - Official device tree compiler and libfdt

---

Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
