/*****************************************************************************
 * File:     usrprintf.h
 * Contents: This file contains the global data, typedefs, enums, and function
 *           prototypes for functions contained in the fastpath test suite
 *           printf utility library usrprintf.c
 *
 * Created:  15-February-06
 *
 * Remarks:  none
 *
 * Copyright (c) 2006 by Reef Point Systems, Inc.
 * All Rights Reserved.
 *
 * 8 New England Executive Park
 * Burlington, MA 01803
 * 781-505-8300
 *****************************************************************************/

#ifndef __USRPRINTF_H__
#define __USRPRINTF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdarg.h"

#define COLUMN_FORMATTING_DISABLE (0)

#define ENABLE_USR_SPRINTF

#ifndef ENABLE_USR_SPRINTF
#define USR_SPRINTF(buffer,format, args...)
#define USR_SPRINTF_DISABLED(buffer,format, args...)
#define USR_SPRINTF_CONVERTED(buffer,format, args...)
#define USR_SPRINTF_LOG(buffer,format, args...)
#define USR_SPRINTF_ENABLED(buffer,format, args...)
#define USR_SPRINTF_ERROR(buffer,format, args...)
#define USR_SPRINTF_SUCCESS(buffer,format, args...)
#define USR_SPRINTF_OK(buffer,format, args...)
#define USR_SPRINTF_FAILURE(buffer,format, args...)
#define USR_SPRINTF_WARNING(buffer,format, args...)
#define USR_SPRINTF_ENTERED(buffer,format, args...)
#define USR_SPRINTF_LEAVING(buffer,format, args...)
#define USR_SPRINTF_DONE(buffer,format, args...)
#define USR_SPRINTF_STATE(buffer,format, args...)
#define USR_SPRINTF_INFO(info, format, args...)
#define USR_HIT_ENTER_TO_CONTINUE()
#define USR_UART_POLLED_MODE_INIT()
#else

int usrSPrintf(char *buffer, const char *file, const char *function, int line, const char *status, const char *format, ...);

#define USR_SPRINTF(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, COLUMN_FORMATTING_DISABLE, format, ##args)

#define USR_SPRINTF_LOG(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "LOG", format, ##args)

#define USR_SPRINTF_DISABLED(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "DISABLED", format, ##args)

#define USR_SPRINTF_CONVERTED(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "OK (c)", format, ##args)

#define USR_SPRINTF_ENABLED(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "ENABLED", format, ##args)

#define USR_SPRINTF_ERROR(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "ERROR", format, ##args)

#define USR_SPRINTF_SUCCESS(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "SUCCESS", format, ##args)

#define USR_SPRINTF_OK(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "OK", format, ##args)

#define USR_SPRINTF_FAILURE(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "FAILURE", format, ##args)

#define USR_SPRINTF_WARNING(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "WARNING", format, ##args)

#define USR_SPRINTF_ENTERED(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "ENTERED", format, ##args)

#define USR_SPRINTF_DONE(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "DONE", format, ##args)

#define USR_SPRINTF_STATE(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "STATE", format, ##args)

#define USR_SPRINTF_LEAVING(buffer,format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, "LEAVING", format, ##args)

#define USR_SPRINTF_INFO(info, format, args...) \
      usrSPrintf(buffer,__FILE__, __FUNCTION__, __LINE__, info, format, ##args)

#endif /* !ENABLE_USR_SPRINTF */

#ifdef __cplusplus
}
#endif

#endif /* __USRPRINTF_H__ */
