// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

/** @file DTBExtnLib_node.c
  FdtLib (edk2) Extended APIs
**/

/*===========================================================================
                        INCLUDE FILES
===========================================================================*/
#include "libfdt.h"
#include "DTBExtnLib.h"
#include "DTBInternals.h"
#include "DTBExtnLib_env.h"
#include "DTBDefs.h"

#if defined (TARGET_UEFI)
  #include <Library/PrintLib.h>
  #include <Library/QcomBaseLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/DebugLib.h>
#elif defined (TARGET_XBL)
  #include <stdint.h>
  #include <stdio.h>
  #include <string.h>
  #include <boot_logger.h>
  #include "boot_config_context.h"
  #include "boot_timer_hw_if.h"
#elif defined (PORT_Q6)
  #include "stdio.h"
  #include "ULogFront.h"
  #include "qurt_sclk.h"
#endif

/*=========================================================================
     Default Defines
==========================================================================*/
#define hash_table_level  10

#define dtb_log_buffer_size  256

#define MAX_PHANDLE_CACHE_SIZE  1000

#define MEMORY_ALLOC_ALIGN_MASK  7

/*if enable ENABLE_DTB_PROFILING, we can analyse dtb hash algorithm performance
when call fdt_get_node_handle it can print dtb search time , node used count,
and node total access time, default disable*/
#if defined (TARGET_UEFI) || defined (TARGET_XBL) || defined (PORT_Q6)
// #define ENABLE_DTB_PROFILING
#endif

/*only define ENABLE_HASH_FOR_DTB before ENABLE_DOUBLE_HASH_FOR_DTB have can define,
 it is just increasing the security of the search, because single-layer hashes may collide,
 default only uesd single hash and disable ENABLE_DOUBLE_HASH_FOR_DTB*/
#ifdef ENABLE_HASH_FOR_DTB
// #define ENABLE_DOUBLE_HASH_FOR_DTB
#endif

#ifdef PORT_Q6
#define DTB_MSG_NAME  "dtb_log"
#define DTB_MSG_SIZE  (200*1024)
#endif

typedef struct hash_node {
  fdt_node_handle     node;
  uint64_t            node_name_hash_value;
  struct hash_node    *next;
 #ifdef ENABLE_DOUBLE_HASH_FOR_DTB
  uint64_t            node_name_hash_value_2;
 #endif
 #ifdef ENABLE_DTB_PROFILING
  uint64_t            node_access;
 #endif
} hash_node;

typedef struct blob_list_node {
  const void               *blob;
  hash_node                *hash_node;
  struct blob_list_node    *next;
} blob_list_node;

typedef struct phandle_to_node_offset_cache {
  const void                             *blob;
  struct phandle_to_node_offset_cache    *next;
  uint32_t                               *array;
  uint32_t                               size;
} phandle_to_node_cache;

/*=========================================================================
     Local Static Variables
==========================================================================*/
#ifdef ENABLE_HASH_FOR_DTB
static blob_list_node         *phead_hash_node_table[hash_table_level] = { 0 };
static phandle_to_node_cache  *phandle_cache_root = NULL;
static char                   parent_path[256] = { 0 }, basename[256] = { 0 };
static int                    total_node = 0;
#endif

#ifdef ENABLE_DTB_PROFILING
static int  total_access_node      = 0;
static int  total_access_node_time = 0;
#endif

#if defined (ENABLE_DTB_PROFILING) && defined (ENABLE_HASH_FOR_DTB)
static int  total_used_node = 0;
#endif

#if defined (TARGET_UEFI)
#elif defined (TARGET_XBL)
static boot_timer_hw_if_type    *timer_if = NULL;
static boot_memory_mgr_if_type  *mem_if   = NULL;
  #if defined (ENABLE_HASH_FOR_DTB) || defined (ENABLE_DTB_PROFILING)
static char  dtb_log_buffer[dtb_log_buffer_size] = { 0 };
  #endif
#elif defined (PORT_Q6)
static ULogHandle  dtbHandle                           = NULL;
static char        dtb_log_buffer[dtb_log_buffer_size] = { 0 };
#endif

/*=========================================================================
     Globals
==========================================================================*/

/*=========================================================================
      Public APIs
==========================================================================*/

/**
 * __dtb_get_time_us - return current time
 *
 * returns: current time us vuale
 *
 */
uint64_t
__dtb_get_time_us (
  void
  )
{
 #if defined (TARGET_UEFI)
  return GetTimerCountus ();
 #elif defined (TARGET_XBL)
  return timer_if->get_apss_qtimer_counter_us ();
 #elif defined (PORT_Q6)
  return qurt_timer_timetick_to_us (qurt_sysclock_get_hw_ticks ());
 #else
  return 0;
 #endif
}

void
__init_timer_mem_api (
  void
  )
{
  static int  init_flag = 0;

  if (1 == init_flag) {
    return;
  }

 #if defined (TARGET_XBL)
  if ((timer_if == NULL) && (mem_if == NULL)) {
    boot_config_context_get_interface (CONFIG_CONTEXT_INTERFACE_MEMORY_MGR, (void **)&mem_if);
    boot_config_context_get_interface (CONFIG_CONTEXT_INTERFACE_TIMER_HW, (void **)&timer_if);
    init_flag = 1;
  }

 #elif defined (PORT_Q6)
  ULogFront_RealTimeInit (&dtbHandle, DTB_MSG_NAME, DTB_MSG_SIZE, ULOG_MEMORY_LOCAL, ULOG_LOCK_OS);
  init_flag = 1;
 #endif
}

#ifdef TARGET_UEFI

/**
  Allocates and zeros a buffer.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
static VOID *EFIAPI
AllocateZeroPoolNoFree (
  IN UINTN  AllocationSize
  )
{
  VOID  *Memory;

  Memory = AllocatePages (EFI_SIZE_TO_PAGES (AllocationSize));
  if (Memory != NULL) {
    ZeroMem (Memory, AllocationSize);
  }

  return Memory;
}

#endif

/**
 * __dtb_malloc - return alloc memory status
 * @dwsize: alloc memory size
 * @ppmem: store memory pointer address
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 */
uint32_t
__dtb_malloc (
  uint32_t  dwSize,
  void      **ppMem
  )
{
  if (NULL == ppMem) {
    return -1;
  }

 #if defined (TARGET_UEFI)
  static uint8_t  *FreeBufferPtr = NULL, *EndPtr = NULL;
  if (dwSize == 0) {
    *ppMem = NULL;
  }

  if (dwSize >= EFI_PAGE_SIZE) {
    *ppMem =  AllocateZeroPoolNoFree (dwSize);
    return FDT_ERR_QC_NOERROR;
  }

  if (FreeBufferPtr == NULL) {
    if ((FreeBufferPtr = AllocateZeroPoolNoFree (EFI_PAGE_SIZE)) == NULL) {
      DEBUG ((EFI_D_WARN, "MemoryAlloc failed\n"));
      *ppMem = NULL;
    }

    EndPtr = FreeBufferPtr + EFI_PAGE_SIZE;
  }

  // 8 byte align
  dwSize = (dwSize + MEMORY_ALLOC_ALIGN_MASK) & (~MEMORY_ALLOC_ALIGN_MASK);
  if (FreeBufferPtr + dwSize > EndPtr) {
    if ((FreeBufferPtr = AllocateZeroPoolNoFree (EFI_PAGE_SIZE)) == NULL) {
      DEBUG ((EFI_D_WARN, "MemoryAlloc failed\n"));
      ASSERT (FreeBufferPtr != NULL);
      *ppMem = NULL;
    }

    EndPtr = FreeBufferPtr + EFI_PAGE_SIZE;
  }

  *ppMem         = FreeBufferPtr;
  FreeBufferPtr += dwSize;
  return FDT_ERR_QC_NOERROR;

 #elif defined (TARGET_XBL)
  mem_if->malloc (dwSize, ppMem);
 #elif defined (PORT_Q6)
  *ppMem = malloc (dwSize);
 #endif

  if (NULL == *ppMem) {
    return -1;
  } else {
    return FDT_ERR_QC_NOERROR;
  }
}

/**
* __dtb_free - return alloc memory status
* @ppmem: store memory pointer address
*
* returns: FDT_ERR_QC_NOERROR (success) else error indicator
*
*/
uint32_t
__dtb_free (
  void  *pmem
  )
{
  if (NULL == pmem) {
    return -1;
  }

 #if defined (TARGET_UEFI)
  // in uefi target no free memory
 #elif defined (TARGET_XBL)
  mem_if->free (pmem);
 #elif defined (PORT_Q6)
  free (pmem);
 #endif

  return FDT_ERR_QC_NOERROR;
}

/**
* get_node_full_name - get node full path names
* @blob: node blob
* @offset: node offset
* @name: store node name buffer
*
*/
void
get_node_full_name (
  const void  *blob,
  int         offset,
  char        *name
  )
{
  const char  *pname[10];
  int         len[10] = { 0 };
  int         i = 0, total_len = 0;
  int         parent_offset = offset;

  pname[i] = fdt_get_name (blob, offset, &len[i]);
  // if parent_offset == 0, node is "/" else node is sub node
  while (parent_offset != 0 && i < 9) {
    i++;
    parent_offset = fdt_parent_offset (blob, parent_offset);
    pname[i]      = fdt_get_name (blob, parent_offset, &len[i]);
  }

  while (i > 0) {
    memset (&name[total_len++], '/', 1);
    i--;
    memcpy (&name[total_len], pname[i], len[i]);
    total_len = total_len + len[i];
  }
}

#ifdef ENABLE_HASH_FOR_DTB

/**
 * Hash2 - calculate an hash value
 * @str: hash string address
 *
 * returns: return a 64 unsigned value
 *
 */
  #ifdef ENABLE_DOUBLE_HASH_FOR_DTB
uint64_t
Hash2 (
  const char  *str
  )
{
  // fnv-64
  #define FNV_PRIME_64     0x109e3799bb211641ULL
  #define OFFSET_BASIS_64  0xcbf29ce484222325ULL
  uint64_t       hash_value = OFFSET_BASIS_64;
  const uint8_t  *data      = (const uint8_t *)str;
  while (*data) {
    hash_value ^= *data;
    hash_value *= FNV_PRIME_64;
    data++;
  }

  return hash_value;
}

  #endif

/**
 * path_depth- calculate a path level depth
 * @path: dtb node path name
 *
 * returns: return dtb node depth
 *
 */
static int
path_depth (
  const char  *path
  )
{
  int         depth = 0;
  const char  *p;

  for (p = path; *p; p++) {
    if (*p == '/') {
      depth++;
    }
  }

  return depth;
}

/**
 * strr_chr - Match the last occurrence position of a specific character in a string
 * @str: string pointer
 * @c: match character
 *
 * returns: If match character is in string, return its last occurrence, otherwise return null
 *
 */
static char *
strr_chr (
  const char  *str,
  int         c
  )
{
  if (NULL == str) {
    return NULL;
  }

  char        char_to_find = (char)c;
  const char  *end         = str + strlen (str);

  while (end != str) {
    if (*(end - 1) == char_to_find) {
      return (char *)(end - 1);
    }

    end--;
  }

  return NULL;
}

/**
 * Hash2 - calculate an hash value
 * @str: hash string address
 *
 * returns: return a 64 unsigned value
 *
 */
static uint64_t
Hash (
  const char  *str
  )
{
  // DJB2
  uint64_t  hash = 5381;
  int       c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

/**
 * find_node - from dtb hash table search a node
 * @blob: dtb node bolb address
 * @name: dtb node name
 *
 * returns: if node in dtb hash table, return hash_node address otherwise return null
 *
 */
static hash_node *
find_node (
  const char  *name,
  const char  *blob
  )
{
  int             level;
  blob_list_node  *blob_node = NULL;
  hash_node       *node      = NULL;

 #ifdef  ENABLE_HASH_FOR_DTB
  uint64_t  node_hash_value = 0;
  node_hash_value = Hash (name);
 #endif

 #ifdef  ENABLE_DOUBLE_HASH_FOR_DTB
  uint64_t  node_hash_value_2 = 0;
 #endif

  level = path_depth (name);

  blob_node = phead_hash_node_table[level];
  while (blob_node) {
    if (blob_node->blob == blob) {
      node = blob_node->hash_node;
      while (node) {
        if (node->node_name_hash_value == node_hash_value) {
 #ifdef ENABLE_DOUBLE_HASH_FOR_DTB
          node_hash_value_2 = Hash2 (name);
          if ( node->node_name_hash_value_2 == node_hash_value_2) {
            return node;
          }

 #else
          return node;
 #endif
        }

        node = node->next;
      }

      return NULL;
    }

    blob_node = blob_node->next;
  }

  return NULL;
}

void
fix_node_name (
  char  *name
  )
{
  int  len = strlen (name);

  if (name[len - 1] == '/') {
    name[len - 1] = '\0';
  }
}

/**
 * dirname - get a path directory name and base name
 * @path: path name
 * @basename: store base name pointer
 *
 * returns: point directory name address
 *
 */
static char *
dirname (
  char  *path,
  char  *basename
  )
{
  char  *last_slash = strr_chr (path, '/');

  // example ".soc"
  if (last_slash == NULL) {
    path[0] = '.';
    path[1] = '\0';
    return path;
    // example "/soc"
  } else if (last_slash == path) {
    if (*(path+1) != '\0') {
      memcpy (basename, last_slash+1, strlen (last_slash+1)+1);
    }

    *(path + 1) = '\0';
    return path;
    // example "//soc"
  } else if ((last_slash == path + 1) && (*path == '/')) {
    memcpy (basename, last_slash+1, strlen (last_slash+1)+1);
    *path       = '/';
    *(path + 1) = '\0';
    return path;
    // example "/soc/memory/"
  } else if ((path + strlen (path) - 1) == last_slash) {
    *last_slash = '\0';
    return dirname (path, basename);
    // example "/soc/memory"
  } else {
    *last_slash = '\0';
    memcpy (basename, last_slash+1, strlen (last_slash+1)+1);
    return path;
  }
}

/**
 * add_hash_node - add a hash_nade to the phead_hash_node_table
 * insert a hash_node to list, that is from head node insert
 * @p_hash_node: pointer to the node to be added
 * @level: node lenel depth
 *
 *
 */
static void
add_hash_node (
  hash_node  *p_hash_node,
  int        level
  )
{
  blob_list_node  *blob_node = phead_hash_node_table[level];

  while (blob_node) {
    if (p_hash_node->node.blob == blob_node->blob) {
      p_hash_node->next    = blob_node->hash_node;
      blob_node->hash_node = p_hash_node;
      return;
    }

    blob_node = blob_node->next;
  }

  dtb_log ("insert hash_node fail\n");
}

/**
 * alloc_phandle_to_node_cache_space - alloc phandle to node offfset cache space
 * @blob: blob address
 * returns: 0 (success) else error indicator
 *
 */
int
alloc_phandle_to_node_cache_space (
  const void  *blob
  )
{
  int                    phandle_cache_is_exist = 0;
  int                    max_phandle            = 0;
  uint32_t               ret_value              = 0;
  phandle_to_node_cache  *current_phandle_cache = NULL;
  phandle_to_node_cache  *phandle_cache         = phandle_cache_root;

  while (NULL != phandle_cache) {
    if (blob == phandle_cache->blob) {
      phandle_cache_is_exist = 1;
    }

    phandle_cache = phandle_cache->next;
  }

  // if phandle to node offset cache already exist, only return success else alloc memory space
  if (phandle_cache_is_exist) {
    dtb_log ("blob [%p] phandle cache has been created!\n");
    return 0;
  } else {
    // get current blob maximum phandle value and limit range
    max_phandle = fdt_get_max_phandle (blob);
    if (max_phandle > MAX_PHANDLE_CACHE_SIZE) {
      max_phandle = MAX_PHANDLE_CACHE_SIZE;
    }

    // if max_pandle == 0 that current blob does not need to creat phandle cache
    if (max_phandle != 0) {
      ret_value = __dtb_malloc (sizeof (phandle_to_node_cache), (void *)&current_phandle_cache);
      if (ret_value) {
        return -1;
      }

      memset (current_phandle_cache, 0, sizeof (phandle_to_node_cache));

      // because phandle value is form 1 start, so array space is max_phandle + 1
      ret_value = __dtb_malloc ((sizeof (uint32_t) * (max_phandle + 1)), (void *)&(current_phandle_cache->array));
      if (ret_value) {
        // Free previously allocated memory
        __dtb_free (current_phandle_cache);
        current_phandle_cache = NULL;
        return -1;
      }

      memset (current_phandle_cache->array, 0, sizeof (uint32_t) * (max_phandle + 1));
      current_phandle_cache->size = max_phandle + 1;
      current_phandle_cache->blob = blob;

      // insert list head and update root point
      current_phandle_cache->next = phandle_cache_root;
      phandle_cache_root          = current_phandle_cache;
    }

    return 0;
  }
}

/**
 * free_phandle_to_node_cache - free phandle cache list
 *
 */
void
free_phandle_to_node_cache (
  void
  )
{
  phandle_to_node_cache  *current_phandle_cache = phandle_cache_root;
  phandle_to_node_cache  *next_phandle_cache    = NULL;

  while (current_phandle_cache) {
    next_phandle_cache = current_phandle_cache->next;
    if ((current_phandle_cache->blob != NULL) && (current_phandle_cache->size > 0)) {
      __dtb_free (current_phandle_cache->array);
      current_phandle_cache->array = NULL;
      current_phandle_cache->blob  = NULL;
      current_phandle_cache->size  = 0;
    }

    __dtb_free (current_phandle_cache);
    current_phandle_cache = next_phandle_cache;
  }

  phandle_cache_root = NULL;
}

/**
 * free_hash_nodes - free hash_node list
 * @p: hash_node list head pointer
 *
 */
static void
free_hash_nodes (
  hash_node  *p
  )
{
  hash_node  *p_next = NULL;

  if (p->next) {
    p_next = p->next;
    free_hash_nodes (p_next);
  }

  // p->next = NULL;
  __dtb_free (p);
}

/**
 * free_hash_tables - free phead_hash_node_table and set it null
 *
 *
 */
void
free_hash_tables (
  void
  )
{
  blob_list_node  *blob_node          = NULL;
  blob_list_node  *previous_blob_node = NULL;
  hash_node       *p                  = NULL;
  int             i                   = 0;

  for (i = 0; i < hash_table_level; i++) {
    blob_node = phead_hash_node_table[i];
    while (blob_node) {
      p = blob_node->hash_node;
      if (p) {
        free_hash_nodes (p);
      }

      previous_blob_node = blob_node;
      blob_node          = blob_node->next;
      memset (previous_blob_node, 0, sizeof (blob_list_node));
    }

    phead_hash_node_table[i] = NULL;
  }
}

/**
 * is_merge_dtb - Determine whether the current device tree has been created
 *@blob: pointer to the device tree blob
 *returns: 1: device tree is merge device else return 0
 */
static int
is_merge_dtb (
  const void  *blob
  )
{
  blob_list_node  *blob_node = NULL;

  for (int i = 0; i < hash_table_level; i++) {
    blob_node = phead_hash_node_table[i];
    while (blob_node) {
      if (blob_node->blob == blob) {
        return 1;
      }

      blob_node = blob_node->next;
    }
  }

  return 0;
}

#endif

/**
 * fdt_get_node_handle - return node_handle
 * @node: pointer to fdt_node_handle object
 * @blob: pointer to the device tree blob
 * @name: pointer to node name (string)
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   int fdt_path_offset(const void *fdt, const char *path);
 */
int
fdt_get_node_handle (
  fdt_node_handle  *node,
  const void       *blob,
  char             *name
  )
{
  int  offset;           /* FdtLib: return offset value */
  int  ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

 #ifdef ENABLE_DTB_PROFILING
  uint64_t  bts, ets;
  __init_timer_mem_api ();
  bts = ets = __dtb_get_time_us ();
 #endif

  PTR_CHECK (node);
  PTR_CHECK (name);

  /* Include the fdt_get_blob_handle API feature here to make that API optional or obsolete*/
  if (NULL == blob) {
    ret_value = fdt_get_blob_handle (&blob, DEFAULT_BLOB_ID);
    if (ret_value != FDT_ERR_QC_NOERROR) {
      return ret_value;
    }
  }

 #ifdef ENABLE_HASH_FOR_DTB
  int        subnode_offset = -1;
  hash_node  *p_node        = NULL;

  // path_depth (name);
  // if (phead_hash_node_table != NULL )
  {
    p_node = find_node (name, blob);
    if (p_node !=  NULL) {
      node->blob   = p_node->node.blob;
      node->offset = p_node->node.offset;
 #ifdef ENABLE_DTB_PROFILING
      ets                    = __dtb_get_time_us ();
      total_access_node      = total_access_node + 1;
      total_access_node_time = total_access_node_time + (ets-bts);
      if (p_node->node_access == 0) {
        p_node->node_access = 1;
        total_used_node++;
      }

 #if defined (TARGET_UEFI)
      dtb_log ("lookup in dtb hash table: current_node[%a]..used_node[%d]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%016x]\n", name, total_used_node, total_access_node, total_access_node_time, blob, p_node->node.offset);
 #else
      dtb_log ("lookup in dtb hash table: current_node[%s]..used_node[%d]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%016x]\n", name, total_used_node, total_access_node, total_access_node_time, blob, p_node->node.offset);
 #endif
 #endif
      return ret_value;
    } else {
      memcpy (parent_path, name, strlen (name)+1);
      dirname (parent_path, basename);
      p_node = find_node (parent_path, blob);
      if (p_node != NULL) {
        subnode_offset = fdt_subnode_offset (p_node->node.blob, p_node->node.offset, basename);
        if (subnode_offset > 0) {
 #ifdef ENABLE_DTB_PROFILING
          ets                    = __dtb_get_time_us ();
          total_access_node      = total_access_node + 1;
          total_access_node_time = total_access_node_time + (ets-bts);
          if (p_node->node_access == 0) {
            p_node->node_access = 1;
            total_used_node++;
          }

 #if defined (TARGET_UEFI)
          dtb_log ("lookup in parent hash table: current_node[%a]..used_node[%d]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%016x]\n", name, total_used_node, total_access_node, total_access_node_time, blob, subnode_offset);
 #else
          dtb_log ("lookup in parent hash table: current_node[%s]..used_node[%d]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%016x]\n", name, total_used_node, total_access_node, total_access_node_time, blob, subnode_offset);
 #endif
 #endif
        }
      }
    }
  }

  if (subnode_offset >= 0) {
    // offset= p_node->node.offset + subnode_offset;
    node->blob = p_node->node.blob;
    // node->offset = offset;
    node->offset = subnode_offset;
    // find_by_subnode_offset = subnode_offset;
  } else
 #endif
  {
    node->blob = blob;

    offset = fdt_path_offset (node->blob, name);
    if (offset >= 0) {
      node->offset = offset;
 #ifdef ENABLE_DTB_PROFILING
      ets                    = __dtb_get_time_us ();
      total_access_node      = total_access_node + 1;
      total_access_node_time = total_access_node_time + (ets-bts);
 #if defined (TARGET_UEFI)
      dtb_log ("lookup in dtb blob: current_node[%a]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%016x]\n", name, total_access_node, total_access_node_time, blob, node->offset);
 #else
      dtb_log ("lookup in dtb blob: current_node[%s]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%016x]\n", name, total_access_node, total_access_node_time, blob, node->offset);
 #endif
 #endif
    } else {
      if (dtb_config.verbose) {
        SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "BAD offset[%d]", offset);
        PUTS (ibuffer);
      }

      ret_value = offset;
    }
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%a][%d]", __func__, ret_value, name, node->offset);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_get_phandle_node - return phandle fdt_node_handle
 * @node: pointer to fdt_node_handle object
 * @phandle: phandle reference in blob
 * @pnode: pointer to fdt_node_handle for new phandle node
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   int fdt_node_offset_by_phandle(const void *fdt, uint32_t phandle);
 */
int
fdt_get_phandle_node (
  fdt_node_handle  *node,
  uint32_t         phandle,
  fdt_node_handle  *pnode
  )
{
  int  offset    = 0;                                           /* FdtLib: return offset value */
  int  ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

 #ifdef ENABLE_DTB_PROFILING
  uint64_t  bts, ets;
  char      name[128] = { 0 };
  __init_timer_mem_api ();
  bts = ets = __dtb_get_time_us ();
 #endif

  PTR_CHECK (node);
  PTR_CHECK (pnode);

 #ifdef ENABLE_HASH_FOR_DTB
  phandle_to_node_cache  *phandle_cache = phandle_cache_root;
  while (phandle_cache) {
    if ((phandle_cache->blob == node->blob) && (phandle_cache->size > phandle)) {
      pnode->offset = offset = phandle_cache->array[phandle];
      pnode->blob   = node->blob;
      break;
    }

    phandle_cache = phandle_cache->next;
  }

  if (offset <= 0)
 #endif
  {
    offset = fdt_node_offset_by_phandle (node->blob, phandle);
    if (offset < 0) {
      if (dtb_config.verbose) {
        SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "BAD phandle[%d]", offset);
        PUTS (ibuffer);
      }

      ret_value = offset;
    } else {
      pnode->offset = offset;
      pnode->blob   = node->blob;
    }

    if (dtb_config.trace) {
      SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%s][%d].[%d]", __func__, ret_value, phandle);
      PUTS (ibuffer);
    }
  }

 #ifdef ENABLE_DTB_PROFILING
  ets = __dtb_get_time_us ();
  memset (&name, 0, 128);
  get_node_full_name (node->blob, offset, name);
  total_access_node      = total_access_node + 1;
  total_access_node_time = total_access_node_time + (ets-bts);
 #if defined (TARGET_UEFI)
  dtb_log ("lookup in dtb blob form phandle:...current_node[%a]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%d]\n", name, total_access_node, total_access_node_time, node->blob, node->offset);
 #else
  dtb_log ("lookup in dtb blob form phandle:...current_node[%s]..access_node[%d]..total_access_time[%d]..blob address[%p]..node offset[%d]\n", name, total_access_node, total_access_node_time, node->blob, node->offset);
 #endif
 #endif

  return ret_value;
}

/**
 * fdt_get_parent_node - return parent node fdt_node_handle
 * @node: pointer to fdt_node_handle object
 * @pnode: pointer to fdt_node_handle for new parent node
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   int fdt_parent_offset(const void *fdt, int nodeoffset);
 */
int
fdt_get_parent_node (
  fdt_node_handle  *node,
  fdt_node_handle  *pnode
  )
{
  int  offset;                                                  /* FdtLib: return offset value */
  int  ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (pnode);

  offset = fdt_parent_offset (node->blob, node->offset);
  if (offset < 0) {
    if (dtb_config.verbose) {
      SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "BAD parent[%d]", offset);
      PUTS (ibuffer);
    }

    pnode->blob   = NULL;
    pnode->offset = offset;
    ret_value     = offset;
  } else {
    pnode->offset = offset;
    pnode->blob   = node->blob;
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%d]", __func__, ret_value, offset);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_node_cmp - compare 2 fdt_node_handle_objects
 * @node_a: pointer to fdt_node_handle object A
 * @node_b: pointer to fdt_node_handle object B
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 */
int
fdt_node_cmp (
  fdt_node_handle  *node_a,
  fdt_node_handle  *node_b
  )
{
  int  ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node_a);
  PTR_CHECK (node_b);

  if (!((node_a->blob == node_b->blob) && (node_a->offset == node_b->offset))) {
    ret_value = -FDT_ERR_QC_NODE_DIFFERENT;

    if (dtb_config.verbose) {
      SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "nodes are different[%p].[%p]", node_a, node_b);
      PUTS (ibuffer);
    }
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%p].[%p]", __func__, ret_value, node_a, node_b);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_node_copy - copy fdt_node_handle_object
 * @dst: pointer to destination fdt_node_handle object
 * @src: pointer to source fdt_node_handle object
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 */
int
fdt_node_copy (
  fdt_node_handle  *dst,
  fdt_node_handle  *src
  )
{
  int  ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (dst);
  PTR_CHECK (src);

  dst->blob   = src->blob;
  dst->offset = src->offset;

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%p].[%p]", __func__, ret_value, dst, src);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_get_prop_values_size_of_node - return required buffer size to hold all properties of node
 * @node: pointer to fdt_node_handle object
 * @size: pointer to uint32 to hold buffer size (in bytes)
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_property_offset(int, const void *, int)
 *   const void *fdt_getprop_by_offset(const void *fdt, int offset, const char **namep, int *lenp);
 */
int
fdt_get_prop_values_size_of_node (
  fdt_node_handle  *node,
  uint32_t         *size
  )
{
  int                        property;          /* FdtLib: property iterator */
  int                        len;               /* FdtLib: return errors & size */
  const char                 *pname;            /* FdtLib: return name */
  const struct fdt_property  *fdtp;             /* FdtLib: return raw prop data */
  int                        ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (size);

  *size = 0;

  fdt_for_each_property_offset (property, node->blob, node->offset) {
    fdtp = fdt_getprop_by_offset (node->blob, property, &pname, &len);
    if ((NULL == fdtp) || (len < 0)) {
      if (dtb_config.verbose) {
        SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "Err[%d].Prop[%a]", len, pname);
        PUTS (ibuffer);
      }

      LEN_ERROR_CHECK (len);
      break;
    } else {
      if (dtb_config.trace) {
        SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a] has size [%d]", pname, len);
        PUTS (ibuffer);
      }

      *size += len;
    }
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%d]", __func__, ret_value, (fdt32_t)*size);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_get_prop_values_of_node - return packed buffer of all properties of node
 * @node: pointer to fdt_node_handle object
 * @format: required for endianess, [b|B] = string/byte,
 *                                  [h|H] = half-word (16),
 *                                  [w|W] = word (32),
 *                                  [d|D] = double-word (64)
 *                                                                      [i|I] = ignore the property
 * @packed_prop_values: buffer pointer to hold packed node properties
 * @size: buffer size (in bytes)
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_property_offset(int, const void *, int)
 *   const void *fdt_getprop_by_offset(const void *fdt, int offset, const char **namep, int *lenp);
 *   const void *fdt_getprop(const void *fdt, int nodeoffset, const char *name, int *lenp);
 *   static inline uint32_t fdt32_to_cpu(fdt32_t x)
 *   static inline uint64_t fdt64_to_cpu(fdt64_t x)
 * NOTE: Must refer header info availble in DTBExtnLib.h for this API
 */
int
fdt_get_prop_values_of_node (
  fdt_node_handle  *node,
  char             *format,
  void             *packed_prop_values,
  uint32_t         size
  )
{
  int         property;                                         /* FdtLib: property iterator */
  int         len;                                              /* FdtLib: return errors & size */
  const char  *pname;                                           /* FdtLib: return name */
  int         ret_value = FDT_ERR_QC_NOERROR;

  uint32_t    i = 0, j;                                 /* indexes & loop counters */
  uint32_t    count = 0;                                /* running count of copied data */
  char        *dwalk;                                   /* walking pointer in destination buffer */
  const void  *swalk;                                   /* source buffer (set for each property) */
  uint8_t     *d8, *s8;                                 /* used for uint8_t  copies */
  uint16_t    *d16, *s16;                               /* used for uint16_t copies */
  uint32_t    *d32, *s32;                               /* used for uint32_t copies */
  uint64_t    *d64, *s64;                               /* used for uint64_t copies */

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (format);
  PTR_CHECK (packed_prop_values);
  dwalk = packed_prop_values;                           /* init destination buffer ptr */

  fdt_for_each_property_offset (property, node->blob, node->offset) {
    /*Ignore the propert*/
    if (('i' == format[i]) || ('I' == format[i])) {
      /*Increase the format[ ] index count by 1 and skip the extraction of that property.*/
      i++;
      continue;
    } else {
      /* step 1: This API return pointer to DATA directly */
      swalk = fdt_getprop_by_offset (node->blob, property, &pname, &len);
      if ((NULL == swalk) || (len <= 0)) {
        if (dtb_config.verbose) {
          SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "Err[%d].Index[%d]", len, i);
          PUTS (ibuffer);
        }

        LEN_ERROR_CHECK (len);
        goto exit;
      } else if (0 == len) {
        /* property has empty value so nothing to copy */
        /* normally this isn't an error, but this API has to error at this point */
        /* client is expecting a value in a packed struct and there is no value */
        if (dtb_config.verbose) {
          ret_value = -FDT_ERR_QC_NILVALUE;
          SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "Property[%a] has no value in DTB[%d]", pname, ret_value);
          PUTS (ibuffer);
          goto exit;
        }
      } else {
        /* property has value to copy to destination */
        if (count >= size) {
          /* count of data items copied so far has filled buffer */
          ret_value = -FDT_ERR_QC_TRUNCATED;
          goto exit;
        }

        if (len > (size-count)) {
          /* size of new data to copy will overflow buffer */
          ret_value = -FDT_ERR_QC_OVERFLOW;
          goto exit;
        }

        switch (format[i]) {
          case 'b':
          case 'B':
            /* byte format for this property (8-bit) */
            d8 = (uint8_t *)dwalk;
            s8 = (uint8_t *)swalk;
            for (j = 0; j < len; j++) {
              d8[j] = s8[j];
            }

            break;

          case 'h':
          case 'H':
            /* half-word format for this property (16-bit) */
            d16 = (uint16_t *)dwalk;
            s16 = (uint16_t *)swalk;
            for (j = 0; j < len/2; j++) {
              d16[j] = fdt16_to_cpu (s16[j]);
            }

            break;

          case 'w':
          case 'W':
            /* word format for this property (32-bit) */
            d32 = (uint32_t *)dwalk;
            s32 = (uint32_t *)swalk;
            for (j = 0; j < len/4; j++) {
              d32[j] = fdt32_to_cpu (s32[j]);
            }

            break;

          case 'd':
          case 'D':
            /* double-word format for this property (64-bit) */
            d64 = (uint64_t *)dwalk;
            s64 = (uint64_t *)swalk;
            for (j = 0; j < len/8; j++) {
              d64[j] = fdt64_to_cpu (s64[j]);
            }

            break;

          default:
            if (dtb_config.verbose) {
              SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%d] BAD format [%c]", i, format[i]);
              PUTS (ibuffer);
            }

            ret_value = -FDT_ERR_QC_BADFORMAT;
            goto exit;
        }

        count += len;                                           /* update running total of copied data */
        dwalk += len;                                           /* update destination walking pointer */

        /* new destination address should be checked for alignment (2/4/8 byte)
           based on next item to extract (see format string) */
      }

      i++;                              /* increment format[] index */
    }
  }

exit:
  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%a]", __func__, ret_value, format);
    PUTS (ibuffer);
  }

  if (ret_value && (size > 0)) {
    SetMem (packed_prop_values, 0, size);
  }

  return ret_value;
}

/**
 * fdt_get_prop_names_size_of_node - return required buffer size to hold all property names in node
 * @node: pointer to fdt_node_handle object
 * @size: pointer to uint32 to hold buffer size (in bytes)
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_property_offset(int, const void *, int)
 *   const void *fdt_getprop_by_offset(const void *fdt, int offset, const char **namep, int *lenp);
 */
int
fdt_get_prop_names_size_of_node (
  fdt_node_handle  *node,
  uint32_t         *size
  )
{
  int                        property;          /* FdtLib: property iterator */
  int                        len;               /* FdtLib: return errors & size */
  const char                 *pname;            /* FdtLib: return name */
  const struct fdt_property  *fdtp;             /* FdtLib: return raw prop data */
  int                        ret_value = FDT_ERR_QC_NOERROR;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (size);

  *size = 0;

  fdt_for_each_property_offset (property, node->blob, node->offset) {
    fdtp = fdt_getprop_by_offset (node->blob, property, &pname, &len);
    if ((NULL == fdtp) || (len < 0)) {
      if (dtb_config.verbose) {
        SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "Err[%d].Prop[%a]", len, pname);
        PUTS (ibuffer);
      }

      LEN_ERROR_CHECK (len);
      break;
    } else {
      *size += AsciiStrLen (pname)+1;
    }
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%d]", __func__, ret_value, (fdt32_t)*size);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_get_prop_names_of_node - return buffer of all property names in node
 * @node: pointer to fdt_node_handle object
 * @prop_names: buffer pointer to hold all property names
 * @size: pointer to uint32 to hold buffer size (in bytes)
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_property_offset(int, const void *, int)
 *   const void *fdt_getprop_by_offset(const void *fdt, int offset, const char **namep, int *lenp);
 */
int
fdt_get_prop_names_of_node (
  fdt_node_handle  *node,
  char             *prop_names,
  uint32_t         size
  )
{
  int                        property;          /* FdtLib: property iterator */
  int                        len;               /* FdtLib: return errors & size */
  const char                 *pname;            /* FdtLib: return name */
  const struct fdt_property  *fdtp;             /* FdtLib: return raw prop data */
  int                        ret_value = FDT_ERR_QC_NOERROR;

  int       tlen, running_count = 0;
  char      *walk = prop_names;
  uint16_t  i;                          /* index into names[] KW doesn't like 32-bits */

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (prop_names);

  fdt_for_each_property_offset (property, node->blob, node->offset) {
    fdtp = fdt_getprop_by_offset (node->blob, property, &pname, &len);
    if ((NULL == fdtp) || (len < 0)) {
      if (dtb_config.verbose) {
        SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "Err[%d].Prop[%a]", len, pname);
        PUTS (ibuffer);
      }

      LEN_ERROR_CHECK (len);
      break;
    } else {
      len = AsciiStrLen (pname);
      if (len < INT_MAX) {
        tlen = len + 1;                         /* include '\0' on end */
      } else {
        ret_value = -FDT_ERR_QC_OVERFLOW;
        break;
      }

      if (tlen <= (size-running_count)) {
        for (i = 0; i < tlen; i++) {
          walk[i] = pname[i];
        }

        walk          += tlen;
        running_count += tlen;
      } else {
        ret_value = FDT_ERR_QC_TRUNCATED;
        break;
      }
    }
  }
  if ((FDT_ERR_QC_NOERROR == ret_value) && (size > 0)) {
    prop_names[size-1] = '\0';                          /* ensure buffer is null-terminated */
  }

  if (dtb_config.trace) {
    SNPRINTF (ibuffer, DTB_LINE_BUF_SIZE, "[%a][%d].[%d]", __func__, ret_value, (fdt32_t)size);
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_get_count_of_subnodes - return count of all subnodes of node
 * @node: pointer to fdt_node_handle object
 * @count: pointer to uint32 to hold count of subnodes
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_subnode(int, const void *, int)
 */
int
fdt_get_count_of_subnodes (
  fdt_node_handle  *node,
  uint32_t         *count
  )
{
  int  subnode;                                 /* FdtLib: subnode iterator */

  uint32_t  i = 0;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (count);

  fdt_for_each_subnode (subnode, node->blob, node->offset) {
    i++;
  }
  *count = i;

  if (dtb_config.trace) {
    SNPRINTF (
              ibuffer,
              DTB_LINE_BUF_SIZE,
              "[%a][%d].[%d].[%d]",
              __func__,
              0,
              node->offset,
              (fdt32_t)*count
              );
    PUTS (ibuffer);
  }

  return FDT_ERR_QC_NOERROR;
}

/**
 * fdt_get_cache_of_subnodes - return cache of all subnodes of node
 * @node: pointer to fdt_node_handle object
 * @cache: pointer to fdt_node_handle array of size @count
 * @count: uint32 to hold count of subnodes
 *
 * Performance Impact: This API is added specifically to address a
 *                     known performance issue.  When a node has sub-nodes
 *                     this API returns an array of fdt_node_handles
 *                     with the DTB offset for each sub-node, so no string
 *                     lookup of the sub-node is required.
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_subnode(int, const void *, int)
 */
int
fdt_get_cache_of_subnodes (
  fdt_node_handle  *node,
  fdt_node_handle  *cache,
  uint32_t         count
  )
{
  int  subnode;                                 /* FdtLib: subnode iterator */

  uint32_t  i = 0;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

 #ifdef ENABLE_DTB_PROFILING
  uint64_t  bts, ets;
  char      name[128] = { 0 };
  __init_timer_mem_api ();
  bts = ets = __dtb_get_time_us ();
 #endif

  PTR_CHECK (node);
  PTR_CHECK (cache);

  fdt_for_each_subnode (subnode, node->blob, node->offset) {
    cache[i].blob   = node->blob;
    cache[i].offset = subnode;
    i++;
    if (i >= count) {
      break;
    }
  }

  if (dtb_config.trace) {
    SNPRINTF (
              ibuffer,
              DTB_LINE_BUF_SIZE,
              "[%s][%d].[%d].[%d]",
              __func__,
              0,
              node->offset,
              (fdt32_t)count
              );
    PUTS (ibuffer);
  }

 #ifdef ENABLE_DTB_PROFILING
  ets = __dtb_get_time_us ();
  dtb_log ("fdt_get_cache_of_subnodes cost time[%d]", (ets-bts));
  for (int i = 0; i < count; i++) {
    total_access_node = total_access_node + 1;
    memset (&name, 0, 128);
    get_node_full_name (node->blob, cache[i].offset, name);
 #if defined (TARGET_UEFI)
    dtb_log ("lookup in dtb blob form parent: current_node[%a]..access_node[%d]..blob address[%p]\n", name, total_access_node, node->blob);
 #else
    dtb_log ("lookup in dtb blob form parent: current_node[%s]..access_node[%d]..blob address[%p]\n", name, total_access_node, node->blob);
 #endif
  }

 #endif

  return FDT_ERR_QC_NOERROR;
}

/**
 * fdt_get_size_of_subnode_names - return size of subnode names
 * @node: pointer to fdt_node_handle object
 * @names: pointer to uint32_t array to hold sizes
 * @count: uint32 to hold count of array items
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_subnode(int, const void *, int)
 *   const char *fdt_get_name(const void *fdt, int nodeoffset, int *lenp);
 */
int
fdt_get_size_of_subnode_names (
  fdt_node_handle  *node,
  uint32_t         *names,
  uint32_t         count
  )
{
  int  subnode;                                 /* FdtLib: subnode iterator */
  int  len;                                     /* FdtLib: return errors & size */
  int  ret_value = FDT_ERR_QC_NOERROR;

  uint16_t  i = 0;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (names);

  if (count == 0) {
    ret_value = -FDT_ERR_QC_NILVALUE;
  } else {
    fdt_for_each_subnode (subnode, node->blob, node->offset) {
      (void)fdt_get_name (node->blob, subnode, &len);
      if (len >= 0) {
        names[i] = len + 1;                             /* name + '\0' */
      } else {
        LEN_ERROR_CHECK (len);
        break;
      }

      if (i++ >= count) {
        ret_value = FDT_ERR_QC_TRUNCATED;
        break;
      }
    }
  }

  if (dtb_config.trace) {
    SNPRINTF (
              ibuffer,
              DTB_LINE_BUF_SIZE,
              "[%a][%d].[%d].[%d]",
              __func__,
              ret_value,
              node->offset,
              (fdt32_t)count
              );
    PUTS (ibuffer);
  }

  return ret_value;
}

/**
 * fdt_get_subnode_names - return buffer of all subnode names
 * @node: pointer to fdt_node_handle object
 * @prop_names: buffer pointer to hold all subnode names
 * @size: pointer to uint32 to hold buffer size (in bytes)
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 * FdtLib calls:
 *   macro: fdt_for_each_subnode(int, const void *, int)
 *   const char *fdt_get_name(const void *fdt, int nodeoffset, int *lenp);
 */
int
fdt_get_subnode_names (
  fdt_node_handle  *node,
  void             *names,
  uint32_t         size
  )
{
  int         subnode;                          /* FdtLib: subnode iterator */
  int         len;                              /* FdtLib: return errors & size */
  const char  *name;                            /* FdtLib: return raw prop data */
  int         ret_value = FDT_ERR_QC_NOERROR;

  uint16_t  i;
  int       tlen;
  int       running_count = 0;
  char      *walk         = (char *)names;

 #ifdef INSTRUMENTATION
  char  ibuffer[DTB_LINE_BUF_SIZE];
 #endif

  PTR_CHECK (node);
  PTR_CHECK (names);

  running_count = 0;
  fdt_for_each_subnode (subnode, node->blob, node->offset) {
    name = fdt_get_name (node->blob, subnode, &len);
    if ((NULL == name) || (len < 0)) {
      LEN_ERROR_CHECK (len);
      break;
    }

    if (len < INT_MAX) {
      tlen = len + 1;                   /* include '\0' on end */
    } else {
      ret_value = -FDT_ERR_QC_OVERFLOW;
      break;
    }

    if (tlen <= (size-running_count)) {
      for (i = 0; i < tlen; i++) {
        walk[i] = name[i];
      }

      running_count += tlen;
      walk          += tlen;
    } else {
      ret_value = -FDT_ERR_QC_BUF2SMALL;
      break;
    }
  }

  if (dtb_config.trace) {
    SNPRINTF (
              ibuffer,
              DTB_LINE_BUF_SIZE,
              "[%a][%d].[%d]",
              __func__,
              ret_value,
              (fdt32_t)size
              );
    PUTS (ibuffer);
  }

  return ret_value;
}

#ifdef ENABLE_HASH_FOR_DTB

/**
 * add_hash - add a hash_node to the dtb hash table
 * @node_offset: node offset in dtb blob
 * @blob: dtb blob address
 * @full_name: current node full name
 * @node_depth: current node depth
 * returns:  current node hash_node pointer(success) else null
 *
 */
static hash_node *
add_hash (
  int         node_offset,
  const void  *blob,
  char        *full_name,
  int         node_depth
  )
{
  hash_node  *sub_node = NULL;
  int        status    = 0;

  if ((node_offset < 0) || (blob == NULL) || (full_name == NULL) || (node_depth < 0)) {
    dtb_log ("add_hash node parameter error\n");
    return NULL;
  }

  status = __dtb_malloc (sizeof (hash_node), (void *)&sub_node);
  if (status) {
    dtb_log ("when add dtb hash_node no memory\n");
    return NULL;
  }

  memset (sub_node, 0, sizeof (hash_node));
  sub_node->node.blob   = blob;
  sub_node->node.offset = node_offset;
 #ifdef ENABLE_HASH_FOR_DTB
  sub_node->node_name_hash_value = Hash (full_name);
 #endif
 #ifdef ENABLE_DOUBLE_HASH_FOR_DTB
  sub_node->node_name_hash_value_2 = Hash2 (full_name);
 #endif
  sub_node->next = NULL;
  add_hash_node (sub_node, node_depth);
  return sub_node;
}

/**
 * init_root - init dtb tree root node and add dtb hash table
 * @blob: dtb root node name
 * @root_name: dtb root node name
 *
 * returns: root node hash_node pointer(success) else null
 *
 */
static hash_node *
init_root (
  const void  *blob,
  const char  *root_name
  )
{
  hash_node       *root_node = NULL;
  blob_list_node  *blob_node = NULL;
  int             offset, status;
  int             i = 0;

  offset = fdt_path_offset (blob, root_name);

  // init blob_node struct and insert hash table array
  while (i < hash_table_level) {
    status = __dtb_malloc (sizeof (blob_list_node), (void *)&blob_node);
    if (status) {
      dtb_log ("dtb alloc memory fail\n");
      return NULL;
    }

    memset (blob_node, 0, sizeof (blob_list_node));
    blob_node->blob          = blob;
    blob_node->next          = phead_hash_node_table[i];
    phead_hash_node_table[i] = blob_node;
    i++;
  }

  if ((offset >= 0) && (*root_name == '/')) {
    status = __dtb_malloc (sizeof (hash_node), (void *)&root_node);
    if (status) {
      dtb_log ("dtb alloc memory fail\n");
      return NULL;
    }

    root_node->node.blob   = blob;
    root_node->node.offset = offset;
 #ifdef ENABLE_HASH_FOR_DTB
    root_node->node_name_hash_value = Hash (root_name);
 #endif
 #ifdef ENABLE_DOUBLE_HASH_FOR_DTB
    root_node->node_name_hash_value_2 = Hash2 (root_name);
 #endif

    root_node->next = NULL;
    add_hash_node (root_node, 1);
    return root_node;
  } else {
    dtb_log ("init root fail\n");
    return NULL;
  }
}

/**
 * Recursive_find_subnode - add all child nodes of a node to the dtb hash table
 * @blob: current dtb blob
 * @node: current hash_node pointer
 * @name: current hash_node name
 * @depth: current hash_node depth
 *
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 */
static int
Recursive_find_subnode (
  const void  *blob,
  hash_node   *node,
  char        *name,
  int         depth
  )
{
  // Stores combined name
  const char  *subname;
  // Contains offset of current node to subnode
  int        sub_node_offset;
  hash_node  *root_node = NULL;
  int        ret_value;
  int        len;

  // handle init, current root not in hashtable
  if (NULL == node) {
    if (*name != '/') {
      dtb_log ("name parameter invaild\n");
      return FDT_ERR_QC_INPUT_ARG_ERR;
    } else {
      root_node = init_root (blob, name);
      total_node++;
      if (NULL != root_node) {
        // root node level depth is 0
        return Recursive_find_subnode (blob, root_node, name, 0);
      } else {
        return FDT_ERR_QC_MEMALLOC;
      }
    }
  } else {
    fdt_for_each_subnode (sub_node_offset, blob, node->node.offset) {
      char       fullname[256] = { 0 };
      int        phandle_value = 0;
      hash_node  *sub_node     = NULL;

      total_node++;

      // get current node phandle value and fill phandle array only phandle_value > 0
      phandle_value = fdt_get_phandle (blob, sub_node_offset);
      if ((phandle_value > 0) && (phandle_value < phandle_cache_root->size)) {
        phandle_cache_root->array[phandle_value] = sub_node_offset;
      }

      // Gets isolated subnode name
      subname = fdt_get_name (blob, sub_node_offset, &len);
      // Adds current node name to subnode and stores in dbuf for navigation
 #if defined (TARGET_UEFI)
      if (AsciiStrCmp (name, "/") != 0) {
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
      if (strcmp (name, "/") != 0) {
 #endif
        // If current node is not root node, add "/" between current node name and subnode name
 #if defined (TARGET_UEFI)
        AsciiSPrint (fullname, 256, "%a/%a", name, subname);
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
        snprintf (fullname, 256, "%s/%s", name, subname);
 #endif
      } else {
 #if defined (TARGET_UEFI)
        AsciiSPrint (fullname, 256, "%a%a", name, subname);
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
        snprintf (fullname, 256, "%s%s", name, subname);
 #endif
      }

      sub_node = add_hash (sub_node_offset, blob, fullname, (depth + 1));

      if (NULL != sub_node) {
        ret_value = Recursive_find_subnode (blob, sub_node, fullname, depth + 1);
        if (ret_value != FDT_ERR_QC_NOERROR) {
          return ret_value;
        }
      } else {
 #if defined (TARGET_UEFI)
        dtb_log ("node add hash table fail:node name[%a]\n", fullname);
 #else
        dtb_log ("node add hash table fail:node name[%s]\n", fullname);
 #endif
        return FDT_ERR_QC_MEMALLOC;
      }
    }
    return FDT_ERR_QC_NOERROR;
  }
}

  #ifdef ENABLE_DTB_PROFILING

/**
 * print all dtb nodes -
 * @blob: current dtb blob
 * @offset: current node offset
 * @name: current hash_node name
 *
 *
 */
static void
print_all_dtb_nodes (
  const void  *blob,
  int         offset,
  char        *name
  )
{
  // Stores combined name
  const char  *subname;
  // Contains offset of current node to subnode
  int  sub_node_offset;
  int  len;

  #if defined (TARGET_UEFI)
      if (AsciiStrCmp (name, "/") != 0) {
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
      if (strcmp (name, "/") != 0) {
 #endif
 #if defined (TARGET_UEFI)
    dtb_log ("found dtb node:[%a]\n", name);
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
    dtb_log ("found dtb node:[%s]\n", name);
 #endif
  }

  fdt_for_each_subnode (sub_node_offset, blob, offset) {
    char  fullname[256] = { 0 };

    subname = fdt_get_name (blob, sub_node_offset, &len);
    // Adds current node name to subnode and stores in dbuf for navigation
 #if defined (TARGET_UEFI)
      if (AsciiStrCmp (name, "/") != 0) {
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
      if (strcmp (name, "/") != 0) {
 #endif
      // If current node is not root node, add "/" between current node name and subnode name
 #if defined (TARGET_UEFI)
      AsciiSPrint (fullname, 256, "%a/%a", name, subname);
      dtb_log ("found dtb node:[%a]..hash value[%016x]..node offset[%016x]\n", fullname, Hash (fullname), sub_node_offset);
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
      snprintf (fullname, 256, "%s/%s", name, subname);
      dtb_log ("found dtb node:[%s]..hash value[%016x]..node offset[%016x]\n", fullname, Hash (fullname), sub_node_offset);
 #endif
    } else {
 #if defined (TARGET_UEFI)
      AsciiSPrint (fullname, 256, "%a%a", name, subname);
      dtb_log ("found dtb node:[%a]..hash value[%016x]..node offset[%016x]\n", fullname, Hash (fullname), sub_node_offset);
 #elif defined (TARGET_XBL) || defined (PORT_Q6)
      snprintf (fullname, 256, "%s%s", name, subname);
      dtb_log ("found dtb node:[%s]..hash value[%016x]..node offset[%016x]\n", fullname, Hash (fullname), sub_node_offset);
 #endif
    }

    print_all_dtb_nodes (blob, sub_node_offset, fullname);
  }
}

  #endif

#endif

/**
 * CreatDtbHashTable - creat a dtb hash table
 * @blob_id: current dtb blob id
 * @blob: current dtb blob address
 * note:it need extra memory store dtb hash_node ,each hash_node need 40 byte memory space
 * total need memory is (total node * 40)
 * returns: FDT_ERR_QC_NOERROR (success) else error indicator
 *
 */
int
CreatDtbHashTable (
  int         blob_id,
  const void  *blob
  )
{
 #ifdef ENABLE_HASH_FOR_DTB
  char      *root_name = "/";
  uint64_t  bts        = 0;
  uint64_t  ets        = 0;
  int       ret_value;
  __init_timer_mem_api ();

 #ifdef ENABLE_DTB_PROFILING
  total_used_node   = 0;
  total_access_node = 0;
 #endif
  dtb_log ("start creat dtb hash table..blob address[%p]\n", blob);
  total_node = 0;
  bts        = __dtb_get_time_us ();

  // free already exist hash table , A device tree can only be created once
  if (is_merge_dtb (blob)) {
    free_hash_tables ();
    free_phandle_to_node_cache ();
    dtb_log ("current dtb hash table already exist, will disable dtb hash table feture\n");
    return FDT_ERR_QC_NOERROR;
  }

  // alloc memory space for phandle to node offset cache
  ret_value = alloc_phandle_to_node_cache_space (blob);
  if (ret_value) {
    dtb_log ("build phandle cache fail\n");
    return -1;
  }

  // build hash table and phandle to node offset cache
  ret_value = Recursive_find_subnode (blob, NULL, root_name, 0);
  ets       = __dtb_get_time_us ();

 #ifdef ENABLE_DTB_PROFILING
  total_access_node_time = (ets-bts);
  print_all_dtb_nodes (blob, 0, "/");
 #endif

  dtb_log ("end create dtb hash table..creat hash table cost time:[%llu]..total node:[%d]\n", (ets-bts), total_node);
  return ret_value;
 #else
  return FDT_ERR_QC_NOERROR;
 #endif
}
