// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear
----------------------------------------------------------------------------------------------------------------------------------

Please ensure to update dt_select_env.c and dt_select_env.h based on the specific environments where these files will be utilized.

dt_select_env.h
---------------
This file contains all the essential header files related to the environment that are required for compiling this library.

Note: If the environment supports standard datatype, please include stdint.h and stddef.h in the file.


dt_select_env.c
---------------
This file defines all the essential wrapper functions that is environment specific. 

This file defines the wrapper function for:
  1. Malloc (MALLOC_WRAPPER)
  2. Free (FREE_WRAPPER)
  3. Log message (LOG_MESSAGE_WRAPPER)
  4. Memcpy (MEMCPY_WRAPPER)