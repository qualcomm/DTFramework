// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

#if defined (TARGET_XBL)
#ifndef BUILD_X86
#include <boot_config_context.h>
#include "boot_logger_if.h"
#include "boot_memory_mgr_if.h"
boot_memory_mgr_if_type *mem_if = NULL;
#endif
#include <string.h>
#include "dt_select_env.h"
#include <stdarg.h>
#elif defined (TARGET_UEFI)
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "dt_select_env.h"
#endif

#if defined (TARGET_XBL)
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
  DEBUG ((DEBUG_WARN, buffer_ptr));
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

#elif defined (TARGET_UEFI)
static inline void
_replace_percent_s_with_percent_a (
  const char  *src,
  char        *dest,
  size_t      size
  );

/* ============================================================================
**  Function : malloc
** ============================================================================
*/
void *
MALLOC_WRAPPER (
  size_t  size
  )
{
  /* Call AllocatePages if size is greater than 4KB */
  if (size >= SIZE_4KB) {
    UINTN  num_of_pages = (size / SIZE_4KB);
    if ((size % SIZE_4KB) != 0x0) {
      num_of_pages++;
    }

    return AllocatePages (num_of_pages);
  }

  return AllocatePool (size);
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
  if (size >= SIZE_4KB) {
    UINTN  num_of_pages = (size / SIZE_4KB);
    if ((size % SIZE_4KB) != 0x0) {
      num_of_pages++;
    }

    FreePages (buffer_ptr, num_of_pages);
  }

  FreePool (buffer_ptr);
}

void
LOG_MESSAGE_WRAPPER (
  char  *buffer_ptr
  )
{
  DEBUG ((DEBUG_WARN, buffer_ptr));
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
  CopyMem (destination_ptr, source_ptr, source_size);
  return source_size;
}

size_t
SNPRINTF_WRAPPER (
  char        *buffer,
  size_t      size,
  const char  *str,
  ...
  )
{
  char  *str_a = (char *)MALLOC_WRAPPER (size);

  if (NULL == str_a) {
    return 0;
  }

  VA_LIST  args;

  VA_START (args, str);
  _replace_percent_s_with_percent_a ((const char *)str, str_a, size);
  size_t  return_size = AsciiVSPrint (buffer, size, str_a, args);

  VA_END (args);
  FREE_WRAPPER (str_a, size);

  // Check for buffer overflow
  if (return_size >= size) {
    // Handle buffer overflow error
    // You can either truncate the string or return an error code
    // For this example, we'll truncate the string
    buffer[size - 1] = '\0'; // null-terminate the string
  }

  return (return_size < size) ? return_size : size - 1;
}

static void
_replace_percent_s_with_percent_a (
  const char  *src,
  char        *dest,
  size_t      size
  )
{
  int  index = 0x0;

  // Check if the destination buffer is large enough
  size_t  src_len = AsciiStrLen (src);

  if (src_len >= size) {
    // Handle error: destination buffer is too small
    // You can either truncate the string or return an error code
    // For this example, we'll truncate the string
    src_len = size - 1; // leave space for the null terminator
  }

  for (index = 0; index < src_len; index++) {
    if ('\0' == src[index]) {
      break;
    }

    dest[index] = src[index];
    if ((src[index] == '%') && (index + 1 < src_len) && (src[index + 1] == 's')) {
      dest[index+1] = 'a';
      index++;
    }
  }

  // Ensure the destination string is null-terminated
  dest[src_len] = '\0';
}
#endif