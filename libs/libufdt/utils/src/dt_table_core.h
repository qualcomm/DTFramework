/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DT_TABLE_CORE_H
#define DT_TABLE_CORE_H

#include <stdint.h>

/*
 * For the image layout, refer README.md for the detail
 */

#define DT_TABLE_MAGIC 0xd7b7ab1e
#define DT_TABLE_DEFAULT_PAGE_SIZE 2048
#define DT_TABLE_DEFAULT_VERSION 0

struct dt_table_header {
  uint32_t magic;       /* DT_TABLE_MAGIC */
  uint32_t total_size;  /* includes dt_table_header + all dt_table_entry
                           and all dtb/dtbo */
  uint32_t header_size; /* sizeof(dt_table_header) */

  uint32_t dt_entry_size;     /* sizeof(dt_table_entry) */
  uint32_t dt_entry_count;    /* number of dt_table_entry */
  uint32_t dt_entries_offset; /* offset to the first dt_table_entry
                                 from head of dt_table_header.
                                 The value will be equal to header_size if
                                 no padding is appended */

  uint32_t page_size; /* flash page size we assume */
  uint32_t version;   /* DTBO image version, the current version is 0.
                         The version will be incremented when the dt_table_header
                         struct is updated. */
};

enum dt_compression_info {
  NO_COMPRESSION,
  ZLIB_COMPRESSION,
  GZIP_COMPRESSION,
  LZ4_COMPRESSION
};

struct dt_table_entry {
  uint32_t dt_size;
  uint32_t dt_offset; /* offset from head of dt_table_header */

  uint32_t id;        /* optional, must be zero if unused */
  uint32_t rev;       /* optional, must be zero if unused */
  uint32_t custom[4]; /* optional, must be zero if unused */
};

struct dt_table_entry_v1 {
  uint32_t dt_size;
  uint32_t dt_offset; /* offset from head of dt_table_header */

  uint32_t id;  /* optional, must be zero if unused */
  uint32_t rev; /* optional, must be zero if unused */
  uint32_t
      flags; /* For version 1 of dt_table_header, the 4 least significant bits
                of 'flags' will be used indicate the compression
                format of the DT entry as per the enum 'dt_compression_info' */
  uint32_t custom[3]; /* optional, must be zero if unused */
};

#endif
