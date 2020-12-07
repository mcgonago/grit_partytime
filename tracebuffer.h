/*****************************************************************************
 * File:     @(#) $Id: //depot/roc/swbranches/include/tracebuffer.h $
 * Contents: This file contains the global data, typedefs, enums, and function
 *           prototypes for the tracebuffer utility
 *
 * Created:  19-Sep-05
 *
 * Remarks:  none
 *
 * Copyright (c) 2005 by Reef Point Systems, Inc.
 * All Rights Reserved.
 *
 * 8 New England Executive Park
 * Burlington, MA 01803
 * 781-505-8300
 *****************************************************************************/

#ifndef __TRACEBUFFER_H__
#define __TRACEBUFFER_H__

#if 0
#include "xstatbuffer.h"
#include "rcli.h"
#endif

#include "zcli.h"

#include "pthread.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_ALL              (0xfffffffful)
#define MAX_DISPLAY_INFO_LENGTH  (100)

#undef UNIT_TEST_WRAP_AROUND

#if 0
typedef struct TimeSpec_s
{
    uint32_t tv_sec;
    uint32_t tv_usec;
} TimeSpec_t;
#endif

int TraceBufferTimeGreaterEqual(TimeSpec_t *t1, TimeSpec_t *t2);
void TraceBufferTimeSubtract(TimeSpec_t* start, TimeSpec_t* end, TimeSpec_t *rslt);

int traceprintf(char *file, const char *function, int line, const char *format, ...);
#define TRACE_PRINTF(format, args...) \
      traceprintf(__FILE__, __FUNCTION__, __LINE__, format, ##args)

    
#ifdef UNIT_TEST_WRAP_AROUND
#define COLUMNS          4
#define TRACE_BUFF_SIZE  1024
#define HIGH_WATER_MARK  32
#else
#define COLUMNS          4

// IO CARDS, IOX's, FNS's need more memory for MMS pools (so give them less for tracing)
#if defined(IOCARD) || defined(IOX) || defined(FNS)
#define TRACE_BUFF_SIZE  (0x1000)
#else
#define TRACE_BUFF_SIZE  (0x100000)    // 1M
//#define TRACE_BUFF_SIZE  (0x10000000)  // 256M
#endif

#define HIGH_WATER_MARK  (256)
#endif /* TEST_WRAP_AROUND */

#ifdef BOOTBLOCK
#define TBPrintf(traceID, file, function, line, format, p1, p2, p3, p4, p5, p6)
#define TBDisplay(traceID, startIndex, endIndex, count, printFp)
#define TBReset()
#define TBDisplayStop(x)
#define TBDisplayStart(x)
#define TBStart(traceID, a)
#define TBStop(traceID, a)
#define TBFind(a,b)
#define TBHighWaterMarkSet(a)
#define TBMaxStringLengthSet(a)
#define TBFilterSet(a)
#define TBFilterClear(a)
#define TBAddrGet(traceID)
#define TBNumGet(traceID)
#define TBMaxDisplayInfoLengthGet()
#define TBHexDump(a, b, c, d, e)
#define TBStack(file, function, line)
#define TBLastGet(traceID)
#else

#define PLATFORM_NATIVE_T uint64_t

extern int TBPrintf(int traceID,
                    char *file,
                    const char *function,
                    int line,
                    char *format, PLATFORM_NATIVE_T p1, PLATFORM_NATIVE_T p2, PLATFORM_NATIVE_T p3, PLATFORM_NATIVE_T p4, PLATFORM_NATIVE_T p5, PLATFORM_NATIVE_T p6);

extern int TBPrintfMicrocode(int traceID, char *buffer);
extern void TBDisplay(int traceID, uint32_t startIndex, uint32_t endIndex, uint32_t count, PRINT_FP printFp);
extern void TBDisplayStop(int traceID);
extern void TBDisplayStart(int traceID);
extern void TBReset(void);
extern void TBStart(int traceID, PRINT_FP printFp);
extern void TBStop(int traceID, PRINT_FP printFp);
extern void TBWrapEnable(int traceID, PRINT_FP printFp);
extern void TBWrapDisable(int traceID, PRINT_FP printFp);
extern void TBStop(int traceID, PRINT_FP printFp);
extern void TBFind(char *filterString, PRINT_FP printFp);
extern void TBHighWaterMarkSet(uint32_t highWater);
extern void TBMaxDisplayInfoLengthSet(uint32_t maxStringLength);
extern uint32_t TBMaxDisplayInfoLengthGet(void);
extern void TBFilterSet(char *filterString);
extern void TBFilterClear(uint32_t filterNumber);
extern char *TBAddrGet(int traceID);
extern uint32_t TBNumGet(int traceID);
extern void TBHexDump(char *file, const char *function, int line, unsigned int *start, uint32_t numBytes);
extern void TBStack(char *file, const char *function, int line);
extern uint32_t TBLastGet(int traceID);
extern void TraceUDelay(unsigned int us);
#endif

#define MAX_TRACE_BUFFERS           (3)
#define MAX_TRACE_CHANNELS          (3)

#define TB_ID_WDB                   (0)
#define TB_ID_WDB_INT               (1)

#define TRACE_RUNNING               (1)
#define TRACE_STOPPED               (2)

#define TRACE_WRAP_ENABLED          (1)
#define TRACE_WRAP_DISABLED         (2)

#define TRACE_REPEAT_ENABLED        (1)
#define TRACE_REPEAT_DISABLED       (2)

#define TB_STACK() \
      TBStack(__FILE__, __FUNCTION__, __LINE__)

#define TB_PRINTF_0(traceID, format) \
      TBPrintf(traceID, __FILE__, __FUNCTION__, __LINE__, format, 0, 0, 0, 0, 0, 0);

#define TB_PRINTF_1(traceID, format, p1) \
      TBPrintf(traceID, __FILE__, __FUNCTION__, __LINE__, format, p1, 0, 0, 0, 0, 0);

#define TB_PRINTF_2(traceID, format, p1, p2) \
      TBPrintf(traceID, __FILE__, __FUNCTION__, __LINE__, format, p1, p2, 0, 0, 0, 0);

#define TB_PRINTF_3(traceID, format, p1, p2, p3) \
      TBPrintf(traceID, __FILE__, __FUNCTION__, __LINE__, format, p1, p2, p3, 0, 0, 0);

#define TB_PRINTF_4(traceID, format, p1, p2, p3, p4) \
      TBPrintf(traceID, __FILE__, __FUNCTION__, __LINE__, format, p1, p2, p3, p4, 0, 0);

#define TB_PRINTF_5(traceID, format, p1, p2, p3, p4, p5) \
      TBPrintf(traceID, __FILE__, __FUNCTION__, __LINE__, format, p1, p2, p3, p4, p5, 0);

#define TB_PRINTF_6(traceID, format, p1, p2, p3, p4, p5, p6) \
      TBPrintf(traceID, __FILE__, __FUNCTION__, __LINE__, format, p1, p2, p3, p4, p5, p6);

#define TB_HEXDUMP_INT(start, numBytes) \
      TBHexDump(__FILE__, __FUNCTION__, __LINE__, start, numBytes)

#define TB_ERROR(errNum) \
      TBError(__FILE__, __FUNCTION__, __LINE__, errNum)

#ifdef __cplusplus
}
#endif

#endif /* __TRACEBUFFER_H__ */
