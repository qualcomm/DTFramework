// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause Clear

/** @file DTBExtnLib_env.h
  Environment header file
**/


#ifndef DTBEXTNLIB_ENV_H_
#define DTBEXTNLIB_ENV_H_

/*=========================================================================
     Instrumentation
==========================================================================*/

#ifdef INSTRUMENTATION
  #include <stdio.h>
#define SNPRINTF(a, b, c, ...)  AsciiSPrint(a, b, c, ##__VA_ARGS__)
#define PUTS(a)                 puts(a)
#else
#define SNPRINTF(a, b, c, ...)
#define PUTS(a)
#endif

/*=========================================================================
     Supported Architectures
==========================================================================*/

// #define PORT_ARMv8
// #define PORT_Q6

/*========================================================================*/
#if defined (TARGET_UEFI)
#define dtb_log(...)  DEBUG ((EFI_D_WARN, ##__VA_ARGS__))
#elif defined (TARGET_XBL)
#define dtb_log(...) \
{\
     snprintf(dtb_log_buffer, dtb_log_buffer_size, ##__VA_ARGS__);\
     boot_log_message(dtb_log_buffer);\
}
#elif defined (PORT_Q6)
#define dtb_log(...) \
{\
     snprintf(dtb_log_buffer, dtb_log_buffer_size, ##__VA_ARGS__);\
     ULogFront_RealTimeString(dtbHandle, dtb_log_buffer);\
}
#else
#define dtb_log(...)
#endif

#ifdef PORT_ARMv8

  #ifdef INSTRUMENTATION

    #ifndef BUILD_X86
      #include "boot_logger_if.h"
    #else
      #include "junkdrawer.h"
    #endif

#define puts  boot_log_message

  #endif

#endif

/*========================================================================*/
#ifdef PORT_Q6

  #include "libfdt_env.h"

  #ifdef INSTRUMENTATION
    #ifdef BUILD_X86
      #include "junkdrawer.h"
    #endif
  #endif

#endif

#endif /* DTBEXTNLIB_ENV_H_ */
