// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/*#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>*/
#ifndef BUILD_X86
#include <boot_config_context.h>
#include "boot_logger_if.h"
#include "boot_memory_mgr_if.h"
boot_memory_mgr_if_type *mem_if = NULL;
#endif
#include <string.h>
#include "dt_select_env.h"
#include <stdarg.h>

/* ============================================================================
**  Function : malloc
** ============================================================================
*/
void *
MALLOC_WRAPPER (
  size_t  size
  )
{
  #ifndef BUILD_X86
  void *ptr = NULL;
  if (mem_if == NULL) {
    boot_config_context_get_interface(CONFIG_CONTEXT_INTERFACE_MEMORY_MGR, (void **)&mem_if);
  }
  mem_if->malloc(size, &ptr);
  return ptr;
  #else
  return malloc(size);
  #endif
}

/* ============================================================================
**  Function : free
** ============================================================================
*/
void
FREE_WRAPPER (
  void    *buffer_ptr,
  size_t  size
  )
{
  #ifndef BUILD_X86
  mem_if->free(buffer_ptr);
  #else
  free(buffer_ptr);
  #endif
}

void
LOG_MESSAGE_WRAPPER (
  char  *buffer_ptr
  )
{
  #if 0
  DEBUG ((EFI_D_WARN, buffer_ptr));
  #else
  boot_log_message(buffer_ptr);
  #endif
}

size_t
MEMCPY_WRAPPER (
  void        *destination_ptr,
  size_t      destination_size,
  const void  *source_ptr,
  size_t      source_size
  )
{
  /* Check whether size for destination buffer size is not less than source size.
     - If it is less, then truncate the copy to destination buffer size.
     - Else copy entire source size worth of memory.
  */
  source_size = source_size > destination_size ? destination_size : source_size;
  memcpy(destination_ptr, source_ptr, source_size);
  return source_size;
}

size_t SNPRINTF_WRAPPER (char *buffer, size_t size, const char *str, ...)
{
  va_list args;
  va_start(args, str);
  size = (size_t)vsnprintf(buffer, size, str, args);
  va_end(args);
  return size;
}
