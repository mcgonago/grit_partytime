/*****************************************************************************
 * File:     @(#) $Id: //depot/roc/swbranches/tracebuffer/tracebuffer.c #6 $
 * Contents: This file contains the IQ trace buffer printf functions
 *
 * Created:  17-May-06
 *
 * Remarks:
 *
 * To build and run unit test:
 *
 * % gcc -ggdb -O0 -c link.c -o link.o
 * % gcc -ggdb -O0 -DZCOUNT_SIMULATION=1 -c timeline.c -o timeline.o
 * % gcc -ggdb -O0 -c linklist.c -o linklist.o
 * % gcc -ggdb -O0 -o unittest -I../include -DUNIT_TEST=1 -DLinux tracebuffer.c link.o linklist.o timeline.o -lrt
 * % ./unittest
 *
 * Copyright (c) 2006 by Reef Point Systems
 * All Rights Reserved.
 *
 * 8 New England Executive Park
 * Burlington, MA 01803
 * 781-505-8300
 *
 *****************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <getopt.h>
#include "pthread.h"
#include "string.h"
#include "fcntl.h"

#include "link.h"
#include "linklist.h"

#if 0
#include "xstatcommands.h"
#include "uint64commas.h"
#include "xstatcustom.h"
#include "rcli_server.h"
#include "rcli.h"
#include "countercustom.h"
#include "xstatcolor.h"
#endif

#include "zcli.h"
#include "zcolor.h"
#include "tracebuffer.h"
#include "timeline.h"
#include "zcli_server.h"
#include "usrsprintf.h"

#if 0 /* defined(__CYGWIN__) */
#define PLATFORM_NATIVE_T uint32_t
#else
#define PLATFORM_NATIVE_T uint64_t
#endif

#ifdef UNIT_TEST
#define ENDIAN_32(x)       \
   ((((x) & 0x000000ff) << 24) | \
    (((x) & 0x0000ff00) <<  8) | \
    (((x) & 0x00ff0000) >>  8) | \
    (((x) & 0xff000000) >> 24))
#else
#define ENDIAN_32(x) (x)
#endif

//#define DEBUG_LEVEL_1 (1)

#define NSEC_IN_SEC                 (1000000000)
#define NSEC_IN_SEC_DIV2            (500000000)
#define NSEC_IN_USEC                (1000)

#define USEC_IN_SEC                 (1000000)
#define USEC_IN_SEC_DIV2            (500000)
#define USEC_IN_NSEC                (1000)

#define NO_FILLER                   (0xFF)
#define NUM_DIGITS_INT_64           (19)
#define INDEX_STRING_MAX_LENGTH     (30)
#define SOME_NUMBER_OF_FORMAT_BYTES (30)
#define GET_PASSED_NULL_TERMINATOR  (1)
#define MAX_STRING_SIZE             (512)

#if 0
int channel2buffer[MAX_TRACE_CHANNELS][MAX_TRACE_BUFFERS] =
{
    {{0,0},
     {0,0},
     {0,0}},
    {{0,0},
     {0,0},
     {0,0}},
    {{0,0},
     {0,0},
     {0,0}}
}

typedef struct TraceChannel_s
{
    char fancyBuffer[MAX_STRING_SIZE];
    char previousString[MAX_STRING_SIZE];
    char inputString[MAX_STRING_SIZE];
    char columnString[MAX_STRING_SIZE];
    char fileCopy[MAX_STRING_SIZE];
    char functionCopy[MAX_STRING_SIZE];
    char taskCopy[MAX_STRING_SIZE];
    char wrappedMessage[MAX_STRING_SIZE];
    char timeStampString[MAX_STRING_SIZE];
} TraceChannel_t;

TraceChannel_t traceChannel[MAX_TRACE_BUFFERS];
#else
static PLATFORM_NATIVE_T busyBufferCounter;
static PLATFORM_NATIVE_T busyBuffer;

// +++owen - DEFAULT TO RUNNING!!!
//static PLATFORM_NATIVE_T traceState = TRACE_STOPPED;
static PLATFORM_NATIVE_T traceState = TRACE_RUNNING;

static PLATFORM_NATIVE_T traceWrap = TRACE_WRAP_ENABLED;
static PLATFORM_NATIVE_T traceRepeat = TRACE_REPEAT_ENABLED;
#endif
 
#define MAX_BUFF_SIZE (1024)
static char traceString[MAX_BUFF_SIZE];
static char taskString[MAX_BUFF_SIZE];

#if 0
static TimeSpec_t previousTime;
static PLATFORM_NATIVE_T previousTick;
static uint32_t firstTime = 1;
#endif

/* +++owen - QUICK AND DIRTY !!! USE a global flag for now !!! */
FILE *logfile = NULL;
uint32_t runInBackground = 0;

typedef struct TraceBuffer_s
{
    char traceBuffer[TRACE_BUFF_SIZE + 10];
    char *tbCurrent;
    char *tbStart;
    PLATFORM_NATIVE_T tbEnd;
    PLATFORM_NATIVE_T wrappedCounter;
    PLATFORM_NATIVE_T repeatCounter;
    PLATFORM_NATIVE_T lastCounter;
    PLATFORM_NATIVE_T busyChannelCounter;
    PLATFORM_NATIVE_T busyChannel;
    PLATFORM_NATIVE_T formatTooLong;
    PLATFORM_NATIVE_T displayStop;

    char fancyBuffer[MAX_STRING_SIZE];
    char previousString[MAX_STRING_SIZE];
    char inputString[MAX_STRING_SIZE];
    char columnString[MAX_STRING_SIZE];
    char fileCopy[MAX_STRING_SIZE];
    char functionCopy[MAX_STRING_SIZE];
    char taskCopy[MAX_STRING_SIZE];
    char wrappedMessage[MAX_STRING_SIZE];
    char timeStampString[MAX_STRING_SIZE];
} TraceBuffer_t; 

TraceBuffer_t traceBuffer[MAX_TRACE_BUFFERS];

ColumnState_t columnState[MAX_COLUMNS];

int tbInitialized = 0;

void cmdHelp( CLI_PARSE_INFO *info);

#define COLUMN_FORMATTING_DISABLE (0)

#define MAX_NUMBER_OF_ARGS (6)

#define CHARS_PER_ASCII_BYTE    3
#define ASCII_BYTES_PER_LONGVAL 4
#define CHARS_PER_ASCII_LONGVAL (CHARS_PER_ASCII_BYTE * ASCII_BYTES_PER_LONGVAL)

#define debug_print(mod, format, args...)   log_tb_printf_columns(__FILE__, __FUNCTION__, __LINE__, "DEBUG", "   DEBUG: " format, ##args)
#define debug_print2(mod, format, args...)  log_tb_printf_columns(__FILE__, __FUNCTION__, __LINE__, "DEBUG", "   DEBUG: " format, ##args)

static char hexDumpString[MAX_STRING_SIZE];
static uint8_t asciistr[CHARS_PER_ASCII_LONGVAL * 4 + 1];  /* 4 words/line + null terminator*/
static char tempStr[20];

static struct {
    uint32_t mask;
    uint32_t shift;
} masks[4] = 
{{0xff000000,24},
 {0x00ff0000,16},
 {0x0000ff00, 8},
 {0x000000ff, 0}};

static uint32_t TBChannelInsert(TraceBuffer_t *tb, char *insertBuff);
static bool TBBufferInsert(TraceBuffer_t *tb, char *insertBuff);
static void update(char *buff, TimeSpec_t *wallTime, TimeSpec_t *relativeTime);
static void convert(char *buff, TimeSpec_t *time1, TimeSpec_t *time2);


//extern void dumpSaveStack(stackInfo_t *stack);
extern uint32_t dumpGetSp(void);
extern void SysTaskIdSelfToName(char *taskString);
extern void cmd_party( CLI_PARSE_INFO *info);

// +++owen
static int intLockLocal (void);
static int intUnlockLocal (int oldSR);

static char newCommandLine[160];
Arguments_t arguments;

#define THREAD_NAME_SIZE (32)

#ifdef MC2
#undef printf
#endif

#ifdef Linux
void SysTaskIdSelfToName(char *taskString)
{
#ifdef MC2
    pthread_t tid = pthread_self();

   int rc;
   int tidx = 0;
   
   rc = pthread_getname_np(tid, taskString, THREAD_NAME_SIZE);

   if (rc != 0)
   {
       sprintf("%s", taskString, "NADA");
   }
#else
    strcpy(taskString, "tLinux");
#endif
}
#endif

static void
SysNewLineStrip(char *buff)
{
    char *ptr = buff;
    uint32_t i;
    uint32_t len = strlen(buff);

    for (i = 0; i < len; i++)
    {
        if ((*ptr == '\n') || (*ptr == '\r'))
        {
            *ptr = ' ';
        }
        ptr++;
    }
}

static int
SysIsDigit(char c)
{
    if ((c >= '0') && (c <= '9'))
    {
        return 1;
    }

    return 0;
}

/*>------------------------------------------------------------------------------
 *
 * (char *)s = SysAtoi(int *val, char *s);
 *	
 *  DESCRIPTION:
 *  This function finds and converts the ascii digit found at the
 *  beginning of the input string. The character following the converted
 *  ascii number is returned;
 *
 *  ARGS:
 *  val          - returned converted number
 *  s            - pointer to start of string.
 *
 *  RETURNS:
 *  s            - points to character following converted ascii number
 *  
 *  REMARKS:
 *  <NONE>
 * 
 *----------------------------------------------------------------------------<*/
static char *
SysAtoi(int *val, char *s)
{
    int j, k;
    char *tmp;
    
    char *endPtr = s;
    int n = 0;

    if (!SysIsDigit(s[0]))
    {
        /* Nothing to convert */
        return endPtr;
    }
    
    while (*s == ' ')
    {
        /* Skip white space */
        s++;
    }

    if (*s == '-')
    {
        /* skip sign for now */
        s++;
    }

    /* Find location of first non digit is */
    for (j = 0; SysIsDigit(s[j]); j++)
    {
        endPtr++;
    }

    /* Point to rightmost (least significant) part of number */
    tmp = endPtr - 1;

    /* Convert decimal number, take care of 0th place first */
    n = (*tmp - '0');
    tmp--;

    /* Then successive adds of next (X) 10s places */
    for (k = 1; k < j; k++, tmp--)
    {
        n += (10 * k) * (*tmp - '0');
    }

    /* return converted integer */
    *val = n;
    
    /* Return pointer to right after number that was just converted */
    return endPtr;
}

int TraceColumnOn(int columnNum)
{
    if ((columnNum > MAX_COLUMNS) || (columnNum <= 0))
    {
       return -1;
    }

    return(columnState[columnNum - 1].on);
}


/*>------------------------------------------------------------------------------
 *
 * (char *)s = FillerGet(int *filler, int *fillerCount, char *s);
 *	
 *  DESCRIPTION:
 *  This function parses the input string looking for the optional
 *  fill and filler counts from %[filler][count]{format}
 *
 *  ARGS:
 *  filler       - returned filler value
 *  fillerCount  - returned filler count
 *  s            - pointer to start of format string
 *
 *  RETURNS:
 *  s            - pointer to format qualifier.
 *  
 *  REMARKS:
 *  <NONE>
 * 
 *----------------------------------------------------------------------------<*/
static char *
FillerGet(unsigned char *filler, int *fillerCount, char *s)
{
    if (!SysIsDigit(s[0]))
    {
        /* No filler or count present */
        return(s);
    }

    if (*s == '0')
    {
        /* zero filler */
        *filler = *s;

        /* Get past filler */
        s++;
    }

    /* Convert filler count (if one actually exists) */
    s = SysAtoi(fillerCount, s);

    if ((*fillerCount > 0) && (*filler == NO_FILLER))
    {
        /* No filler found, but count, so, make the filler a ' ' */
        *filler = ' ';
    }
    
    return s;
}

static int
SysMemcpy(char *a, char *b, unsigned long size)
{
    while (size) {
	*a=*b;
	++a;
	++b;
	--size;
    }
    return(0);
}

static int
SysStrcpy(char *a, char *b)
{
    int len = 0;

    if (b == 0)
    {
        return 0;
    }

    while (*b != '\0') {
	*a=*b;
	++a;
	++b;
	len++;
    }

    *a = '\0';
    len++;

    return(len);
}

static PLATFORM_NATIVE_T
TBSprintf(char *buf, char *format, PLATFORM_NATIVE_T p1, PLATFORM_NATIVE_T p2, PLATFORM_NATIVE_T p3, PLATFORM_NATIVE_T p4, PLATFORM_NATIVE_T p5, PLATFORM_NATIVE_T p6)
{
    unsigned long temp;
    char *string;
    PLATFORM_NATIVE_T arguments[MAX_NUMBER_OF_ARGS];

    int argIdx = 0;
    unsigned long count = NUM_DIGITS_INT_64;
    unsigned char filler = NO_FILLER;
    int fillerCount = -1;
    int done = 0;
    char *current = buf;
    
    arguments[0] = p1;
    arguments[1] = p2;
    arguments[2] = p3;
    arguments[3] = p4;
    arguments[4] = p5;
    arguments[5] = p6;
    
    while (*format)
    {
        if (*format != '%')
        {
            *current = *format;
            ++current;
        }
        else if (argIdx >= MAX_NUMBER_OF_ARGS)
        {
            /* Too many format specifiers found in format string */
            break;
        }
        else
        {
            done = 0;
            filler = NO_FILLER;
            fillerCount = -1;

            /* Bump pointer past '%' */
            ++format;

            /* For now, ignore any/all negate flags */
            if (*format == '-')
            {
                /* Bump pointer past negate flag */
                ++format;
            }
            
            /* Strip optional filler and count from %[filler][count]{specifier} */
            format = FillerGet(&filler, &fillerCount, (char *)format);

	    while (!done)
            {
                switch(*format)
                {
                    case 's' :

                        string = (char *)arguments[argIdx++];
		    
                        while (*string)
                        {
                            *current = *string;
                            ++current;
                            ++string;
                        }
                        done = 1;
                        break;

		    case 'l' :
		    case 'u' :
		    case 'L' :
		    case 'U' :
                        /* Ignoring above qualifiers - always assuming 4-byte long words */
                        ++format;
                        /* We are not done */
                        break;

		    case 'c' :

                        temp = arguments[argIdx++];

                        tempStr[0] = (char)(temp & 0xFF);

                        SysMemcpy(current, &tempStr[0], 1);

                        current += 1;
                        done = 1;
                        break;
                        
		    case 'd' :
                        count = NUM_DIGITS_INT_64;
                        temp = arguments[argIdx++];

                        do
                        {
                            tempStr[count] = temp % 10 + '0';
                            temp = temp / 10;
                            --count;
                        } while (temp);

                        if (filler != NO_FILLER)
                        {
                            while ((NUM_DIGITS_INT_64 - count) < fillerCount)
                            {
                                tempStr[count] = filler;
                                --count;
                            }
                        }

                        SysMemcpy(current, &tempStr[count+1], NUM_DIGITS_INT_64 - count);
                        current += NUM_DIGITS_INT_64 - count;
                        done = 1;
                        break;
			    
		    case 'x' :
		    case 'X' :
                        count = NUM_DIGITS_INT_64;
                        temp = arguments[argIdx++];
		    
                        do {
                            tempStr[count] = (temp & 0xf);
                            if (tempStr[count] >= 0xa)
                            {
                                tempStr[count] += ('a' - 0xa);   
                            } else
                            {
                                tempStr[count] += '0';   
                            }
				    
                            temp = temp >> 4;
                            --count;
                        } while (temp);
				
                        if (filler != NO_FILLER)
                        {
                            while ((NUM_DIGITS_INT_64 - count) < fillerCount)
                            {
                                tempStr[count] = filler;
                                --count;
                            }
                        }
				
                        SysMemcpy(current, &tempStr[count+1], NUM_DIGITS_INT_64 - count);
                        current += NUM_DIGITS_INT_64 - count;
                        done = 1;
                        break;
				
		    default:
                        done = 1;
                        break;
		}
            }            
        }            
        ++format;
    }

    *current = 0;
    
    return((int)(current - buf));
}

static bool FileCheck (const char *path)
{
    struct stat sb;

    return (stat (path, &sb) == 0);
}


#ifdef MC2

// +++owen - FOR NOW - DUE to EARLY - to investigate later - SBC execution of printf....

// typedef int (*PRINT_FP)(const char *format, ...);

static int printf_disable(const char *format, ...)
{
    return(0);
}
#endif

void TBInit(PRINT_FP print_fp)
{
    int i;
    TraceBuffer_t *tb;
    FILE *fp ;
    char cmdLine[CMD_BUF_LEN];
#ifdef MC2
    PRINT_FP printFp = &printf_disable;
#else    
    PRINT_FP printFp = print_fp;
#endif

    if (tbInitialized == 1)
    {
        return;
    }
    
    tbInitialized = 1;

    if (printFp == NULL)
    {
       printFp = printf;
    }
    
    memset(traceBuffer, 0, sizeof(traceBuffer));

    for (i = 0; i < MAX_TRACE_BUFFERS; i++)
    {
        tb = &traceBuffer[i];
        tb->tbCurrent = tb->traceBuffer;
        tb->tbStart  = tb->traceBuffer;
        //tb->traceState = TRACE_RUNNING;
        tb->tbEnd = (PLATFORM_NATIVE_T)tb->traceBuffer + TRACE_BUFF_SIZE;
    }

    busyBufferCounter = 0;
    busyBuffer = 0;


    for (i = 0; i < MAX_COLUMNS; i++)
    {
        columnState[i].on = 1;
    }

#if 0
    // +++owen - hardcode for now
    columnState[0].on = 0;
    columnState[1].on = 0;
    columnState[2].on = 0;
    columnState[3].on = 0;
#else
    if ((FileCheck("/boot/tbinit")) || (FileCheck("tbinit")) || (FileCheck("~/tbinit")))
    {
        /* +++owen - hack - force pre TBPrint commands - in particular 'column' commands - to run first */

        if (FileCheck("/boot/tbinit"))
        {
            TBPrintf(TB_ID_WDB, __FILE__, __FUNCTION__, __LINE__, "INFO: Found /boot/tbinit", 0, 0, 0, 0, 0, 0);
            fp = fopen("/boot/tbinit", "r");
        }
        else if (FileCheck("~/tbinit"))
        {
            TBPrintf(TB_ID_WDB, __FILE__, __FUNCTION__, __LINE__, "INFO: Found ~/tbinit", 0, 0, 0, 0, 0, 0);
            fp = fopen("~/tbinit", "r");
        }
        else
        {
            TBPrintf(TB_ID_WDB, __FILE__, __FUNCTION__, __LINE__, "INFO: Found tbinit (locally)", 0, 0, 0, 0, 0, 0);
            fp = fopen("tbinit", "r");
        }

        if (!fp)
        {
            return;
        }

        while (1)
        {
            if (!fgets(cmdLine, CMD_BUF_LEN, fp))
            {
                /* Done reading file */
                break;
            }

            TBPrintf(TB_ID_WDB, __FILE__, __FUNCTION__, __LINE__, "%s", (PLATFORM_NATIVE_T)cmdLine, 0, 0, 0, 0, 0);

            /* Skip comment lines */
            if (strstr(cmdLine, "#") != 0)
            {
                continue;
            }


            /* Get rid of linefeed <lf> */
            cmdLine[strlen(cmdLine) - 1] = '\0';
                                                 
            /* Parse command and execute */            
            cliEntry(cmdLine, printFp);
        }

        fclose(fp);
    }
#endif
    
}

#ifndef Linux

static PLATFORM_NATIVE_T
GetTick(void)
{
    uint32_t low,high,high2;
    PLATFORM_NATIVE_T ticks;
    int lock;
    
    lock = intLockLocal();
    
    high = asmGetTBU();
    low = asmGetTime();
    high2 = asmGetTBU();

    /* if we rolled over between reads just read low value again */
    if (high != high2)
    {
        low = asmGetTime();
        high = high2;
    }
    lock = intUnlockLocal(lock);

    ticks = ((PLATFORM_NATIVE_T)high << 32) | low;

    return(ticks);
}

static int
GetTime(TimeSpec_t *tp)
{
  /*
   * Decrementer frequency for PPC7447 is the SYS_CLK_FREQ 166666666 (1/6 of a Gig)
   * divided by (4), which is 41,666,666.6, right around 41MHz, 1/24 of a Gig
   *
   * (1/41666666)*4294967295 = 103.07921508, which means we wrap every 103 seconds.
   *
   * usTick = (41666666 + 500000) / 1000000 = 42.1
   *
   * To convert from frequence to a microsecond tick, we add 1/2 of what we
   * are looking for divide to round up (thus, the + 500000 / 1000000)
   */
#if defined(PPC7447)
    uint32_t usTick = (DECREMENTER_FREQ + USEC_IN_SEC_DIV2) / USEC_IN_SEC;
#else
  uint32_t usTick = (SYS_CLK_FREQ + USEC_IN_SEC_DIV2) / USEC_IN_SEC;
#endif
  uint32_t tmp;
  PLATFORM_NATIVE_T currentTick;
  PLATFORM_NATIVE_T tickBump;

aaax
  if (firstTime == 1)
  {
      tp->tv_sec = 0;
      tp->tv_usec = 0;
      return 0;
  }
   
  currentTick = GetTick();

  if (currentTick >= previousTick)
  {
      tickBump = (currentTick - previousTick) / usTick;
  }
  else
  {
      /* wrapped... */
      tickBump = ((0xFFFFFFFF - previousTick) + currentTick) / usTick;
  }

  /* If ticks cross usec in sec boundary */
  if ((previousTime.tv_usec + tickBump) / USEC_IN_SEC)
  {
      previousTime.tv_sec += ((previousTime.tv_usec + tickBump) / USEC_IN_SEC);
      previousTime.tv_usec = (previousTime.tv_usec + tickBump) % USEC_IN_SEC;
  }
  else
  {
      previousTime.tv_usec += tickBump;
  }
  

  tp->tv_sec = previousTime.tv_sec;
  tp->tv_usec = previousTime.tv_usec;
  previousTick = currentTick;

#if 0 /*def NP*/
  sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "tv_sec = %d,  tv_usec = %d\n", tp->tv_sec, tp->tv_usec);
#endif

  return OK;
}
#endif

int
TraceBufferGetTime(TimeSpec_t *tp)
{

#if 0 /* def Linux */
  unsigned long status;
  struct timezone ignore;
  struct timeval  value;

  status = gettimeofday(&value, &ignore);

  if(status == -1){
	printf("Couldn't get time of day\n");
  }

  /* Explicitly assign things; do not depend on casting. */
  tp->tv_sec = value.tv_sec;
  tp->tv_usec = value.tv_usec;

  return status;
#endif
  
#if 1
   struct timespec tpp;
    
   if (clock_gettime(CLOCK_REALTIME, &tpp) != 0)
   {
       tp->tv_sec = 0;
       tp->tv_usec = 0;
       return ERROR;
   }
   tp->tv_sec = tpp.tv_sec;
   tp->tv_usec = tpp.tv_nsec / NSEC_IN_USEC;

   return 0;
#endif

#ifndef Linux   
   if (firstTime == 1)
   {
       firstTime = 0;
       previousTick = GetTick();
       GetTime(&previousTime);
   }

   GetTime(tp);
   return OK;
#endif

}

/*>------------------------------------------------------------------------------
 *
 * (int)ret = TBPrintf(int traceID,
 *                     char *file, char *function, int line,
 *                     char *format,
 *                     int p1, int p2, int p3, int p4, int p5, int p6);
 *	
 *  DESCRIPTION:
 *  This function writes data to a trace buffer. A trace buffer contains
 *  a successive collection of formatted data. Instead of using printf to
 *  display data to the console, TBPrintf is used to write data to an internal
 *  trace buffer - to be displayed later by calling TBDisplay.
 *
 *
 *  ARGS:
 *  traceID  - ID of trace buffer
 *  file     - __FILE__ string
 *  function - __FUNCTION__ string
 *  line     - __LINE__ integer
 *  status   - status string
 *  format   - format
 *  p1..p6   - required (6) arguments
 *
 *  RETURNS:
 *  ret      - number of characters written to trace buffer.
 *  
 *  REMARKS:
 *  - variable argument lists could not be used when called from an ISR.
 *  - all ANSI C runtime calls have been replaced - since they are not all
 *    reentrant and/or callable from an ISR.
 *
 *----------------------------------------------------------------------------<*/
int
TBPrintf(int traceID,
         char *file,
         const char *function,
         int line,
         char *format, PLATFORM_NATIVE_T p1, PLATFORM_NATIVE_T p2, PLATFORM_NATIVE_T p3, PLATFORM_NATIVE_T p4, PLATFORM_NATIVE_T p5, PLATFORM_NATIVE_T p6)
{
    int i;
    int numberOfSpaces;
    uint32_t msrVal;
    
    PLATFORM_NATIVE_T idx = 0;
    PLATFORM_NATIVE_T inputLen = 0;
    PLATFORM_NATIVE_T len = 0;
    PLATFORM_NATIVE_T len2 = 0;
    PLATFORM_NATIVE_T len3 = 0;
    TraceBuffer_t *tb = &traceBuffer[traceID];
    static TimeSpec_t previousTime;
    static TimeSpec_t baseTime;
    TimeSpec_t currentTime; 
    TimeSpec_t result;
    TimeSpec_t result2;
    char time1[80];
#if 0
    time_t rawtime;
    struct tm *ptm;
    char *timeAndDate1;
#endif
    char *ptr, *ptr2;

    struct timeval tv;
    time_t sec;
    int msec;
    size_t charsWritten;
    size_t bufSize = 8; // Multiplied by 2 for the initial allocation.
    char *buf = NULL;
    static uint32_t firstTime = 1;
    
    if (traceState == TRACE_STOPPED)
    {
        return 0;
    }

    msrVal = intLockLocal();
    if (firstTime == 1)
    {
        firstTime = 0;
        
        /* Get previous time */
        TraceBufferGetTime(&previousTime);

        /* Get base time */
        TraceBufferGetTime(&baseTime);

        TBInit(NULL);
    }
        
    if (tb->busyChannel == 1)
    {
        tb->busyChannelCounter++;
        intUnlockLocal(msrVal);
        return 0;
    }

    tb->busyChannel = 1;
    intUnlockLocal(msrVal);

#if 0
    time(&rawtime);
    ptm = localtime(&rawtime);
    timeAndDate1 = asctime(ptm);
    strcpy(time1, timeAndDate1);
#else
   if (gettimeofday(&tv, NULL)) {
      TBChannelInsert(tb, "ERROR: Could not gettimeofday");
      /* Put something in there */
      sprintf(time1, "%s", "Nov  5 14:16:23.617");
   }
   else
   {
      sec = tv.tv_sec;
      msec = tv.tv_usec / 1000;

      /*
       * Loop repeatedly trying to format the time into a buffer, doubling the
       * buffer with each failure. This should be safe as the manpage for
       * strftime(3) seems to suggest that it only fails if the buffer isn't large
       * enough.
       *
       * The resultant string is encoded according to the current locale.
       */
      do {
         char *newBuf;
         bufSize *= 2;

         newBuf = realloc(buf, bufSize);
         if (newBuf == NULL) {
             // +++owen - ERROR - can't free buf and dateTime quite yet...
             // goto out;
             // free(buf);
             // free(dateTime);

             // +++owen - PUT something in there
             sprintf(time1, "%s", "Nov  5 14:16:23.617");
             return 0;
         }

         buf = newBuf;
         charsWritten = strftime(buf, bufSize, "%b %d %H:%M:%S", localtime(&sec));

      } while (charsWritten == 0);

      sprintf(time1, "%s.%03d", buf, msec);
      // printf("TIME: %s\n", time1);
   }
#endif   

    // Get rid of newline
    time1[strlen(time1)-1] = '\0';

    if (((COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 1) < 2) ||
        ((COLUMN_TWO_WIDTH - SPACES_BETWEEN_COLS - 1) < 2) ||
        ((COLUMN_THREE_WIDTH - SPACES_BETWEEN_COLS - 1) < 2))
    {
        TBChannelInsert(tb, "ERROR: Could not fit requested string");
    }

    /* Get current time */
    TraceBufferGetTime(&currentTime);
    TraceBufferTimeSubtract(&previousTime, &currentTime, &result);

    /* For next time around... */
    previousTime = currentTime;

    //TBSprintf(taskString, "CH%02d", (int)traceID, 0, 0, 0, 0, 0);
    SysTaskIdSelfToName(&taskString[0]);

    inputLen = TBSprintf(tb->inputString, format, p1, p2, p3, p4, p5, p6);

    // +++owen
    // idx = TBSprintf((char *)tb->fancyBuffer, "[%03d.%06d]: ", (int)result.tv_sec, (int)result.tv_usec, 0, 0, 0, 0);

   if (TraceColumnOn(1) == 1)
   {
       TraceBufferTimeSubtract(&baseTime, &currentTime, &result2);
       idx = TBSprintf((char *)tb->fancyBuffer, "%s [%03d.%06d] [%03d.%06d]: ", (PLATFORM_NATIVE_T)time1, (int)result2.tv_sec, (int)result2.tv_usec, (int)result.tv_sec, (int)result.tv_usec, 0);
   }

    if (0) /* (status == COLUMN_FORMATTING_DISABLE) */
    {
        /* Print out input string, without fancy formatting of columns */
        idx += TBSprintf((char *)&tb->fancyBuffer[idx], "%s", (PLATFORM_NATIVE_T)tb->inputString, 0, 0, 0, 0, 0);
    }
    else
    {
        if ((file != NULL) && (len = (strlen(file)) < MAX_STRING_SIZE) && (len > 0))
        {
            if ((ptr = strstr(file, "/cc/")) != 0)
            {
                ptr += 4;
                /* Strip leading up to next backslash */
                if ((ptr2 = strchr(ptr, '/')) == NULL)
                {
                    SysStrcpy(tb->fileCopy, ptr);
                }
                else
                {
                    ptr2++;
                    SysStrcpy(tb->fileCopy, ptr2);
                }
            }
            else
            {
                SysStrcpy(tb->fileCopy, file);
            }
        }
        else
        {
            tb->fileCopy[0] = '\0';
        }
        
        if ((function != NULL) && (len = (strlen(function)) < MAX_STRING_SIZE) && (len > 0))
        {
            SysStrcpy(tb->functionCopy, (char *)function);
        }
        else
        {
            tb->functionCopy[0] = '\0';
        }
        
        if ((len = (strlen(taskString)) < MAX_STRING_SIZE) && (len > 0))
        {
            SysStrcpy(tb->taskCopy, taskString);
        }
        else
        {
            tb->taskCopy[0] = '\0';
        }

        if (inputLen > 0)
        {
            if (TraceColumnOn(2) == 1)
            {
                /* Format the [{file}:{line}] column */
                if (tb->fileCopy[0] == '\0')
                {
                    len = TBSprintf(tb->columnString, "[xxxxx:%d]", (PLATFORM_NATIVE_T)line, 0, 0, 0, 0, 0);
                }
                else
                {
                    len = TBSprintf(tb->columnString, "[%s:%d]", (PLATFORM_NATIVE_T)tb->fileCopy, (PLATFORM_NATIVE_T)line, 0, 0, 0, 0);
                }
                
                /* See if length of [{file}:{line}] exceeds column width */
                if (len > COLUMN_ONE_WIDTH)
                {
#if 1
                    /* Keep most of the last part of filename:line */
                    // len = strlen(columnString) - COLUMN_ONE_WIDTH - 1;
                    // len2 = len - COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 2;
                    tb->columnString[0] = '[';
                    len2 = len - COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 2;
                    len3 = len - len2;

                    // strncpy(&columnString[1], &columnString[len2 + 6], (len3 - 6));
                    SysMemcpy(&tb->columnString[1], &tb->columnString[len2 + 6], (len3 - 6));
                    tb->columnString[1 + (len3 - 6)] = '\0';
                    len = 1 + (len3 - 6);
#else               
                    /* Shorten the file name */
                    tb->fileCopy[(strlen(tb->fileCopy))/2] = '\0';

                    /* Try it again */
                    len = TBSprintf(tb->columnString, "[%s:%d]", (PLATFORM_NATIVE_T)tb->fileCopy, (PLATFORM_NATIVE_T)line, 0, 0, 0, 0);

                    if (len > COLUMN_ONE_WIDTH)
                    {
                        /* Truncate it */
                        tb->columnString[COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 1] = ']';
                        tb->columnString[COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS] = '\0';
                        len = strlen(tb->columnString);
                    }
#endif
                }

                /* print [{file}:{line}] */
                idx += TBSprintf((char *)&tb->fancyBuffer[idx], "%s", (PLATFORM_NATIVE_T)tb->columnString, 0, 0, 0, 0, 0);

                /* Fill spaces to start of next column */
                numberOfSpaces = (COLUMN_ONE_WIDTH - len) + SPACES_BETWEEN_COLS;
                for (i = 0; i < numberOfSpaces; i++)
                {
                    idx += TBSprintf((char *)&tb->fancyBuffer[idx], " ", 0, 0, 0, 0, 0, 0);
                }
            }

            if (TraceColumnOn(3) == 1)
            {
                /* Format the [{function}] column */
                if  (tb->functionCopy[0] == '\0')
                {
                    len = TBSprintf(tb->columnString, "[xxxxx]", 0, 0, 0, 0, 0, 0);
                }
                else
                {
                    len = TBSprintf(tb->columnString, "[%s]", (PLATFORM_NATIVE_T)tb->functionCopy, 0, 0, 0, 0, 0);
                }
                
                /* See if [{function}] exceeds column width */
                if (len > COLUMN_TWO_WIDTH)
                {
                    /* Truncate it */
                    tb->columnString[COLUMN_TWO_WIDTH - SPACES_BETWEEN_COLS - 1] = ']';
                    tb->columnString[COLUMN_TWO_WIDTH - SPACES_BETWEEN_COLS] = '\0';
                    len = strlen(tb->columnString);
                }

                idx += TBSprintf((char *)&tb->fancyBuffer[idx], "%s", (PLATFORM_NATIVE_T)tb->columnString, 0, 0, 0, 0, 0);

                /* Fill spaces to start of next column */
                numberOfSpaces = (COLUMN_TWO_WIDTH - len) + SPACES_BETWEEN_COLS;
                for (i = 0; i < numberOfSpaces; i++)
                {
                    idx += TBSprintf((char *)&tb->fancyBuffer[idx], " ", 0, 0, 0, 0, 0, 0);
                }
            }

            if (TraceColumnOn(4) == 1)
            {
                if (tb->taskCopy[0] == '\0')
                {
                    len = TBSprintf(tb->columnString, "[xxxxx]", 0, 0, 0, 0, 0, 0);
                }
                else
                {
                    len = TBSprintf(tb->columnString, "[%s]", (PLATFORM_NATIVE_T)tb->taskCopy, 0, 0, 0, 0, 0);
                }

                if (len > COLUMN_THREE_WIDTH)
                {
                    /* Truncate it */
                    tb->columnString[COLUMN_THREE_WIDTH - SPACES_BETWEEN_COLS - 1] = ']';
                    tb->columnString[COLUMN_THREE_WIDTH - SPACES_BETWEEN_COLS] = '\0';
                    len = strlen(tb->columnString);
                }
                    
                /* Print status string */
                idx += TBSprintf((char *)&tb->fancyBuffer[idx], "%s", (PLATFORM_NATIVE_T)tb->columnString, 0, 0, 0, 0, 0);

                /* Fill spaces to start of next column */
                numberOfSpaces = (COLUMN_THREE_WIDTH - len) + SPACES_BETWEEN_COLS;
                for (i = 0; i < numberOfSpaces; i++)
                {
                    idx += TBSprintf((char *)&tb->fancyBuffer[idx], " ", 0, 0, 0, 0, 0, 0);
                }
            }

            if (TraceColumnOn(5) == 1)
            {
                if (inputLen > COLUMN_FOUR_WIDTH)
                {
                    /* Truncate it */
                    tb->inputString[COLUMN_FOUR_WIDTH - SPACES_BETWEEN_COLS] = '\0';
                    inputLen = strlen(tb->inputString);
                }

                /* Quick check to remove trailing newlines and/or carriage returns */
                SysNewLineStrip(tb->inputString);
                
                /* Print out input string */
                idx += TBSprintf((char *)&tb->fancyBuffer[idx], "%s", (PLATFORM_NATIVE_T)tb->inputString, 0, 0, 0, 0, 0);
                idx += TBSprintf((char *)&tb->fancyBuffer[idx], "\n", 0, 0, 0, 0, 0, 0);
            }
        }
        else
        {
            len = TBSprintf(tb->inputString,
                                     "ERROR: sysPrintf: Could not format input string\n",
                                     0, 0, 0, 0, 0, 0);

            idx += TBSprintf((char *)&tb->fancyBuffer[idx],
                                      "%s",
                                      (PLATFORM_NATIVE_T)tb->inputString, 0, 0, 0, 0, 0);
        }
    }

    TBChannelInsert(tb, tb->fancyBuffer);

    tb->busyChannel = 0;

    return(len); 
}

int traceprintf(char *file, const char *function, int line, const char *format, ...)
{
    int len;
    va_list vlist;

    va_start(vlist, format);
    len = vsnprintf(traceString, sizeof(traceString), format, vlist);
    va_end(vlist);

    TBPrintf(TB_ID_WDB, file, function, line, "%s\n", (PLATFORM_NATIVE_T)traceString, 0, 0, 0, 0, 0);

    return len;
}

int
TBPrintfMicrocode(int traceID, char *buffer)
{
    uint32_t msrVal;
    
    int idx = 0;
    TraceBuffer_t *tb = &traceBuffer[traceID];
    static TimeSpec_t previousTime;
    TimeSpec_t currentTime;
    TimeSpec_t result;
    static uint32_t firstTime = 1;
    
    if (traceState == TRACE_STOPPED)
    {
        return 0;
    }

    msrVal = intLockLocal();

    if (firstTime == 1)
    {
        firstTime = 0;
        
        TBInit(NULL);

        /* Get previous time */
        TraceBufferGetTime(&previousTime);
    }
        
    if (tb->busyChannel == 1)
    {
        tb->busyChannelCounter++;
        intUnlockLocal(msrVal);
        return 0;
    }

    tb->busyChannel = 1;
    intUnlockLocal(msrVal);
    
    if (((COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 1) < 2) ||
        ((COLUMN_TWO_WIDTH - SPACES_BETWEEN_COLS - 1) < 2) ||
        ((COLUMN_THREE_WIDTH - SPACES_BETWEEN_COLS - 1) < 2))
    {
        TBChannelInsert(tb, "ERROR: Could not fit requested string");
    }

    /* Get current time */
    TraceBufferGetTime(&currentTime);
    TraceBufferTimeSubtract(&previousTime, &currentTime, &result);

    /* For next time around... */
    previousTime = currentTime;

    idx = TBSprintf((char *)tb->fancyBuffer, "[%03d.%06d]: ", (PLATFORM_NATIVE_T)result.tv_sec, (PLATFORM_NATIVE_T)result.tv_usec, 0, 0, 0, 0);

    /* Print out input string, without fancy formatting of columns */
    idx += TBSprintf((char *)&tb->fancyBuffer[idx], "%s", (PLATFORM_NATIVE_T)buffer, 0, 0, 0, 0, 0);

    TBChannelInsert(tb, tb->fancyBuffer);

    tb->busyChannel = 0;

    return(idx); 
}


static uint32_t
TBChannelInsert(TraceBuffer_t *tbIgnore, char *insertBuff)
{
    int idx;
    char *ptr;
    char *warning = "!! WARNING !!BUFFER TOO LARGE!!! ";

    /*
     * +++owen - UNTIL we support the concept of multiple channels injecting
     * info. into their own buffer, and later on combining them, we physically
     * force all insertions from this point on to be over the first buffer
     */
    TraceBuffer_t *tb = &traceBuffer[TB_ID_WDB];

    if (busyBuffer == 1)
    {
        busyBufferCounter++;
        return FALSE;
    }

    busyBuffer = 1;

    if ((strlen(insertBuff) + SOME_NUMBER_OF_FORMAT_BYTES) > MAX_STRING_SIZE)
    {
#if 1
        /* INSERT marker telling us we are hitting the maximum - put some of it in there? */
        ptr = warning;
        idx = 0;
        while (*ptr != '\0')
        {
           insertBuff[idx++] = *ptr;
           ptr++;
        }
        insertBuff[MAX_STRING_SIZE - SOME_NUMBER_OF_FORMAT_BYTES - 2] = '\n';
        insertBuff[MAX_STRING_SIZE - SOME_NUMBER_OF_FORMAT_BYTES - 1] = '\0';
#endif
        /* Format too large */
        tb->formatTooLong++;
    }

    TBBufferInsert(tb, insertBuff);

    busyBuffer = 0;

    return 0;
}


static bool
TBBufferInsert(TraceBuffer_t *tb, char *insertBuff)
{
    /* Check to see if we are about to wrap around end of buffer */ 
    if ((PLATFORM_NATIVE_T)((PLATFORM_NATIVE_T)tb->tbCurrent + strlen(insertBuff)) >
        (PLATFORM_NATIVE_T)(tb->tbEnd - HIGH_WATER_MARK))
    {
        if (traceWrap == TRACE_WRAP_DISABLED)
        {
            return TRUE;
        }

        tb->wrappedCounter++;

        /* go back to beginning */
        tb->tbCurrent = tb->tbStart;

        TBSprintf((char *)tb->tbCurrent, "[Overflow, wrapped %d times]\n", (PLATFORM_NATIVE_T)tb->wrappedCounter, 0, 0, 0, 0, 0);
        tb->tbCurrent = (char *)((PLATFORM_NATIVE_T)tb->tbCurrent + strlen(tb->tbCurrent) + GET_PASSED_NULL_TERMINATOR);
    }

    if (traceRepeat == TRACE_REPEAT_ENABLED)
    {
        if (tb->previousString[0] == '\0')
        {
            SysStrcpy(tb->previousString, insertBuff);
        }
        else
        {
            /* Compare current string with previous, if the same, bump up repeatCounter, and leave */
            if (strcmp(insertBuff, tb->previousString) == 0)
            {
                /* bump up counter */
                tb->repeatCounter += 1;
                return TRUE;
            }
            else if (tb->repeatCounter > 0)
            {
                /* Shove in repeated string, with counter following */
                TBSprintf((char *)tb->tbCurrent, "%s", (PLATFORM_NATIVE_T)tb->previousString, 0, 0, 0, 0, 0);
                tb->tbCurrent = (char *)((PLATFORM_NATIVE_T)tb->tbCurrent + strlen(tb->tbCurrent));

                /* Counter */
                TBSprintf((char *)tb->tbCurrent,
                          "                         Last message repeated %ld times...\n",
                          tb->repeatCounter,
                          0, 0, 0, 0, 0);

                tb->tbCurrent = (char *)((PLATFORM_NATIVE_T)tb->tbCurrent + strlen(tb->tbCurrent) + GET_PASSED_NULL_TERMINATOR);

                /* reset repeat counter */
                tb->repeatCounter = 0;

                /* reset last counter */
                tb->lastCounter = 0;
            }
        }
    }
    
    /* Reset previousString */
    SysStrcpy(tb->previousString, insertBuff);
        
    /* Copy string into memory */
    SysStrcpy(tb->tbCurrent, insertBuff);

    /* Bump up current pointer */
    tb->tbCurrent = (char *)((PLATFORM_NATIVE_T)tb->tbCurrent + strlen(tb->tbCurrent) + GET_PASSED_NULL_TERMINATOR);

    /* +++owen - once we have wrapped, it is ok to clobber the next location */
    if (tb->wrappedCounter > 0)
    {
        /* +++owen - will get overwritten when next one comes in... */
        TBSprintf((char *)tb->tbCurrent, "[CURRENT POSITION]: currently in %d wrap]\n",
                  tb->wrappedCounter + 1,
                  0, 0, 0, 0, 0);

        /* DO NOT Bump up current pointer...we want to leave the position alone so that next write goes over it */
        //tbCurrent = (char *)((PLATFORM_NATIVE_T)tbCurrent + strlen(tbCurrent) + GET_PASSED_NULL_TERMINATOR);
    }

    tb->lastCounter += 1;

    return TRUE;
}

void TBDisplayAll()
{
    printf("Hello!!\n");
}


/*>------------------------------------------------------------------------------
 *
 * (void)TBDisplay(int traceID, uint32_t startIndex, uint32_t endIndex, uint32_t count,
 *                 PRINT_FP printFp);
 *	
 *  DESCRIPTION:
 *  This function displays the current contents of the trace buffer.
 *
 *  ARGS:
 *  <none>
 *
 *  RETURNS:
 *  <none>
 *
 *  REMARKS:
 *  <none>
 * 
 *----------------------------------------------------------------------------<*/
void
TBDisplay(int traceID, uint32_t startIndex, uint32_t endIndex, uint32_t count, PRINT_FP print_fp)
{
    char *ptr;
    uint32_t tempBufferLength;
    uint32_t currentIndex;
    uint32_t totalDisplayed = 0;
    char tempBuffer[MAX_STRING_SIZE];

    TraceBuffer_t *tb = &traceBuffer[traceID];
    char *tbPtr = tb->tbStart;

    if (tbPtr == (char *)0)
    {
        print_fp("Trace buffer empty\n");
        return ;
    }
    
    if ((*tbPtr == '\0') && (tb->repeatCounter == 0))
    {
        print_fp("Trace buffer empty\n");
        return ;
    }
    else if ((*tbPtr == '\0') && (tb->repeatCounter != 0))
    {
        print_fp("%s", tb->previousString);
        print_fp("                         Last message repeated %ld times...\n", tb->repeatCounter);
        return;
    }

    /* Go through, from start to end, applying the input filterString */
    currentIndex = 0;
    while (1)
    {
        if (tb->displayStop == 1)
        {
            print_fp("STOP: stopping display\n");
            break;
        }

        if ((*tbPtr == '\0') || (totalDisplayed >= count))
        {
            /* Check to see if we end with a repeat string */
            if (tb->repeatCounter > 0)
            {
                print_fp("                         Last message repeated %ld times...\n", tb->repeatCounter);
            }
            
            break;
        }

        if (strlen(tbPtr) > MAX_STRING_SIZE)
        {
            print_fp("DONE, strlen = %d is too large\n", strlen(tbPtr));
            break;
        }

        TBSprintf(tempBuffer, "%s", (PLATFORM_NATIVE_T)tbPtr, 0, 0, 0, 0, 0);

        tempBufferLength = strlen(tempBuffer);
        
        /* Find next occurance of '\n' - this assumes that ALL messages have a newline */
        if ((ptr = strchr(tbPtr, '\n')) == NULL)
        {
            /* Assumption is wrong, no newline found, shove one in and continue */
            tbPtr[tempBufferLength - 1] = '\n';
            tbPtr[tempBufferLength] = '\0';

            /* Bump up current pointer */
            tbPtr = (char *)((PLATFORM_NATIVE_T)tbPtr + tempBufferLength + GET_PASSED_NULL_TERMINATOR);
            
            continue;
        }

        /* Only display strings that fall within the specified range */
        if ((currentIndex >= startIndex) && (currentIndex <= endIndex))
        {
            print_fp("%s", tempBuffer);
            totalDisplayed += 1;
        }

        /* Bump up current pointer */
        tbPtr = (char *)((PLATFORM_NATIVE_T)tbPtr + tempBufferLength + GET_PASSED_NULL_TERMINATOR);

        /* Bump up index */
        currentIndex++;
    }

    print_fp("DONE, displayed %d entries\n", totalDisplayed);

#if 0
    /*
     * Send a newline to the FNS console so that you don't have to hit enter
     * to see the prompt - cosmetic
     */
    strcpy(tempBuffer, "\r\n");
    cliEntry(tempBuffer, &printf);
#endif
    
}

uint32_t
TBStateGet()
{
    return(traceState);
}

void
TBStart(int traceID, PRINT_FP print_fp)
{
    // TraceBuffer_t *tb = &traceBuffer[traceID];
    //tb->traceState = TRACE_RUNNING;
    traceState = TRACE_RUNNING;
}

void
TBStop(int traceID, PRINT_FP print_fp)
{
    // TraceBuffer_t *tb = &traceBuffer[traceID];
    //tb->traceState = TRACE_STOPPED;
    traceState = TRACE_STOPPED;
}

void
TBWrapEnable(int traceID, PRINT_FP print_fp)
{
    traceWrap = TRACE_WRAP_ENABLED;
}

void
TBWrapDisable(int traceID, PRINT_FP print_fp)
{
    traceWrap = TRACE_WRAP_DISABLED;
}

void
TBRepeatEnable(int traceID, PRINT_FP print_fp)
{
    traceRepeat = TRACE_WRAP_ENABLED;
}

void
TBRepeatDisable(int traceID, PRINT_FP print_fp)
{
    traceRepeat = TRACE_WRAP_DISABLED;
}

void
TBDisplayStop(int traceID)
{
    TraceBuffer_t *tb = &traceBuffer[traceID];
    tb->displayStop = 1;
}

void
TBDisplayStart(int traceID)
{
    TraceBuffer_t *tb = &traceBuffer[traceID];
    tb->displayStop = 0;
}

static void
update(char *buff, TimeSpec_t *wallTime, TimeSpec_t *relativeTime)
{
    char *ptr;
    char t1[32];
    
    /* Find first '[' */
    ptr = strchr(buff, '[');

    ptr++;
    
    /* replace with wallTime */
    sprintf(t1, "%03d.%06d", wallTime->tv_sec, wallTime->tv_usec);

    strncpy(ptr, t1, 10);

    /* Get next [ */
    ptr += 11;

    ptr = strchr(ptr, '[');

    ptr++;
    
    /* replace with relativeTime */
    sprintf(t1, "%03d.%06d", relativeTime->tv_sec, relativeTime->tv_usec);

    strncpy(ptr, t1, 10);
}

static void
convert(char *buff, TimeSpec_t *time1, TimeSpec_t *time2)
{
    uint32_t sec, usec;
    char *ptr;
    char t1[32];
    
    /* Find first '[' */
    ptr = strchr(buff, '[');

    ptr++;
    
    strncpy(t1, ptr, 10);

    sscanf(t1, "%03d.%06d", &sec, &usec);

#if 0
    if (usec >= 100000)
    {
        usec *= 10000;
    }
    else if (usec >= 10000)
    {
        usec *= 1000;
    }
    else if (usec >= 1000)
    {
        usec *= 100;
    }
    else if (usec >= 100)
    {
        usec *= 10;
    }
#endif
    
    time1->tv_sec = sec;
    time1->tv_usec = usec;

    /* Get next [ */
    ptr += 11;
    
    ptr = strchr(ptr, '[');

    ptr++;
    
    strncpy(t1, ptr, 10);

    sscanf(t1, "%03d.%06d", &sec, &usec);

#if 0
    if (usec >= 100000)
    {
        usec *= 10000;
    }
    else if (usec >= 10000)
    {
        usec *= 1000;
    }
    else if (usec >= 1000)
    {
        usec *= 100;
    }
    else if (usec >= 100)
    {
        usec *= 10;
    }
#endif
    time2->tv_sec = sec;
    time2->tv_usec = usec;
}


void
TBFind(char *filterString, PRINT_FP print_fp)
{
    int i;
    uint32_t index = 0;
    uint32_t totalDisplayed = 0;
    char *tbPtr;
    TraceBuffer_t *tb;

    TimeSpec_t prevWall;
    TimeSpec_t startingDiff;

    TimeSpec_t currWall;
    TimeSpec_t currDiff;

    TimeSpec_t resultWall;
    //TimeSpec_t resultDiff;

    TimeSpec_t zeroTime;

    int firstTime = 1;
    
    char tempBuffer[MAX_STRING_SIZE];

    zeroTime.tv_sec = 0;
    zeroTime.tv_usec = 0;

    TBInit(print_fp);

    for (i = 0; i < MAX_TRACE_BUFFERS; i++)
    {
        tb = &traceBuffer[i];
        tbPtr = tb->tbStart;
    
        if (*tbPtr == '\0')
        {
            continue;
        }

        /* Go through, from start to end, applying the input filterString */
        while (1)
        {
            if (*tbPtr == '\0')
            {
                break;
            }

            if (strlen(tbPtr) > MAX_STRING_SIZE)
            {
                print_fp("DONE, strlen = %d is too large\n", strlen(tbPtr));
                break;
            }

            sprintf(tempBuffer, "%s", tbPtr);

            if (strstr(tempBuffer, filterString) != 0)
            {
                totalDisplayed += 1;

                if (firstTime == 1)
                {
                    convert(tempBuffer, &prevWall, &startingDiff);
                    update(tempBuffer, &prevWall, &zeroTime);
                    firstTime = 0;
                }
                else
                {
                    convert(tempBuffer, &currWall, &currDiff);

                    TraceBufferTimeSubtract(&prevWall, &currWall, &resultWall);

                    // Carry over the first diff
                    // TraceBufferTimeSubtract(&startingDiff, &resultWall, &resultDiff);

                    // TraceBufferTimeSubtract(&prevDiff, &currDiff, &resultDiff);
                    // TraceBufferTimeSubtract(&prevDiff, &currDiff, &resultDiff);

                    // update(tempBuffer, &resultWall, &resultDiff);
                    // update(tempBuffer, &currWall, &resultDiff);

                    update(tempBuffer, &currWall, &resultWall);
                    prevWall = currWall;
                }

                print_fp("%d: %s", i, tempBuffer);
            }

            /* Bump up current pointer */
            tbPtr = (char *)((PLATFORM_NATIVE_T)tbPtr + strlen(tempBuffer) + GET_PASSED_NULL_TERMINATOR);

            index++;
        }
    }
    
    print_fp("DONE, found %d strings that matched filter %s\n",
             totalDisplayed, filterString);
    
}

static int 
SysIsAscii(int c)
{
	return (c >= 0 && c < 128);
}

static unsigned int SysIsPrint(int ch )
{
    return (unsigned int)((ch - ' ') < 127u - ' ');
}


static int hexlong2ascii(uint32_t val, uint8_t *ascstr)
{
    int i;

    memset(ascstr, 0, CHARS_PER_ASCII_LONGVAL + 1);

    for (i = 0; i < ASCII_BYTES_PER_LONGVAL; i++)
    {
        uint32_t c = (val & masks[i].mask) >> masks[i].shift;

        if (SysIsAscii(c) && SysIsPrint(c))
        {
            TBSprintf((char *)&ascstr[i*CHARS_PER_ASCII_BYTE],"  %c",(char)c, 0, 0, 0, 0, 0);
        }
        else
        {
            TBSprintf((char *)&ascstr[i*CHARS_PER_ASCII_BYTE], " ..", 0, 0, 0, 0, 0, 0);
        }
    }

    return i * CHARS_PER_ASCII_BYTE;
}

/*>------------------------------------------------------------------------------
 *
 * (void)TBHexDump(char *file, const char *function, int line,
 *                 unsigned char *start, uint32_t numBytes);
 *	
 *  DESCRIPTION:
 *  This function performs a dump of memory in hexadecimal. Instead of
 *  using printf to display data at the console, TBPrintf is called to
 *  write data to the trace buffer.
 *
 *  ARGS:
 *  start    - pointer to buffer
 *  numBytes - number of bytes
 *
 *  RETURNS:
 *  <none>
 *  
 *  REMARKS:
 *  <none>
 * 
 *----------------------------------------------------------------------------<*/
void
TBHexDump(char *file, const char *function, int line, unsigned int *start, uint32_t numBytes)
{
    int i,k;
    unsigned int *tmp;
    int insertPtr = 0;
    int asciiIndex = 0;
    
    tmp = (unsigned int *)start;

    if (traceState == TRACE_STOPPED)
    {
        return;
    }

    memset(asciistr, 0, sizeof(asciistr));

    for (i=0; i < ((numBytes/4)+1) ; i++)
    {
        if ((i & 0x3) == 0)
        {
//            TBSprintf(hexDumpString, "%08lx: ", (PLATFORM_NATIVE_T)tmp, 0, 0, 0, 0, 0);
            TBSprintf(hexDumpString, "%08lx: ", (PLATFORM_NATIVE_T)i*16, 0, 0, 0, 0, 0);
            insertPtr = 10;
        }

        TBSprintf(&hexDumpString[insertPtr], "%08x ", (PLATFORM_NATIVE_T)ENDIAN_32(*tmp), 0, 0, 0, 0, 0);

        asciiIndex += hexlong2ascii(ENDIAN_32(*tmp), &asciistr[asciiIndex]);

        insertPtr += 9;
        tmp++;
            
        if ((i & 0x3) == 3)
        {
            if (asciiIndex)
            {
                asciistr[asciiIndex] = 0;
                TBSprintf(&hexDumpString[insertPtr], "\t%s", (PLATFORM_NATIVE_T)&asciistr[0], 0, 0, 0, 0, 0);
                insertPtr += asciiIndex;
                asciiIndex = 0;
            }

            TBPrintf(TB_ID_WDB, file, function, line, "%s\n", (PLATFORM_NATIVE_T)&hexDumpString[0], 0, 0, 0, 0, 0);

            /* fill string with spaces */
            for (k = 0; k < sizeof(hexDumpString); k++)
            {
                hexDumpString[k] = ' ';
            }
        }
    }

  /* Finish display when count is not a multiple of 16 */
  if ((i & 3) != 3)
  {
      /*           1         2         3         4       
       * 01234567890123456789012345678901234567890123456789
       * 085BEC58: EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE    X
       */
      asciistr[asciiIndex] = '\0';
      hexDumpString[insertPtr] = ' ';
      /* Point to column that starts ascii string */
      insertPtr = 49;
      TBSprintf(&hexDumpString[insertPtr], "%s", (PLATFORM_NATIVE_T)asciistr, 0, 0, 0, 0, 0);
      TBPrintf(TB_ID_WDB, file, function, line, "%s\n", (PLATFORM_NATIVE_T)hexDumpString, 0, 0, 0, 0, 0);
  }

}

char *
TBAddrGet(int traceID)
{
    TraceBuffer_t *tb = &traceBuffer[traceID];
    return(tb->tbStart);
}

uint32_t
TBLastGet(int traceID)
{
    TraceBuffer_t *tb = &traceBuffer[traceID];
    return(tb->lastCounter);
}

uint32_t
TBNumGet(int traceID)
{
    TraceBuffer_t *tb = &traceBuffer[traceID];
    return((PLATFORM_NATIVE_T)((PLATFORM_NATIVE_T)tb->tbCurrent - (PLATFORM_NATIVE_T)tb->tbStart));
}

void TraceInfoDisplay(PRINT_FP print_fp)
{
    int i;
    TraceBuffer_t *tb;
    
    TBInit(print_fp);

    for (i = 0; i < 1 /*MAX_TRACE_BUFFERS*/; i++)
    {
        tb = &traceBuffer[i];

        print_fp("%d: start buffer address       : 0x%08x\n", i, tb->tbStart);
        print_fp("%d: current buffer address     : 0x%08x\n", i, tb->tbCurrent);
        print_fp("%d: buffer size                : %d\n", i, TRACE_BUFF_SIZE);
        print_fp("%d: number of entries          : %d\n", i, tb->lastCounter);
        print_fp("%d: total # bytes used         : %d\n", i, (PLATFORM_NATIVE_T)tb->tbCurrent - (PLATFORM_NATIVE_T)tb->tbStart);
        print_fp("%d: total # bytes remaining    : %d\n", i, TRACE_BUFF_SIZE - ((PLATFORM_NATIVE_T)tb->tbCurrent - (PLATFORM_NATIVE_T)tb->tbStart));
        print_fp("%d: # times wrapped            : %d\n", i, tb->wrappedCounter);
        print_fp("%d: # times busy (channel)     : %d\n", i, tb->busyChannelCounter);
        print_fp("%d: # times busy (buffer)      : %d\n", i, busyBufferCounter);
        print_fp("%d: # times format too long    : %d\n", i, tb->formatTooLong);
//        print_fp("%d: current state              : %s\n", i, tb->traceState == TRACE_RUNNING? "RUNNING" : "STOPPED");
        print_fp("%d: current state              : %s\n", i, traceState == TRACE_RUNNING? "RUNNING" : "STOPPED");
        print_fp("%d: buffer wrap                : %s\n", i, traceWrap == TRACE_WRAP_ENABLED? "ENABLED" : "DISABLED");
        print_fp("%d: repeat last message        : %s\n", i, traceRepeat == TRACE_REPEAT_ENABLED? "ENABLED" : "DISABLED");
//        print_fp("%d: FTP info                   : %s\n", i, "owen@10.1.4.85/home/owen/foobar.txt");
        print_fp("\n");
    }
    
}

uint32_t TBError(char *file, const char *function, int line, uint32_t errNum)
{
   TBPrintf(TB_ID_WDB_INT, file, function, line, "WDB_ERROR = 0x%08x (%d)", (int)errNum, (int)errNum, 0, 0, 0, 0);
   return errNum;
}

#ifdef Linux
// #if !defined(B_ATCA) && !defined(ACME_ARCH)
static int intLockLocal()
{
  return 0;
}

static int intUnlockLocal(int msr)
{
  return 0;
}
// #endif

void
TraceUDelay(unsigned int us)
{
}

#else
void
TraceUDelay(unsigned int us)
{
    uint32_t us_ticks = (DECREMENTER_FREQ + 500000) / 1000000;
    uint32_t ticks_until = asmGetTime() + us_ticks * us;

    while ((int32_t) (ticks_until - asmGetTime()) > 0)
    {
        taskDelay(0);
    }
}
#endif

void
OSSleep(int milliseconds)
{
#ifdef Linux
  struct timeval tv;

  tv.tv_sec = (milliseconds / 1000) /* seconds */;
  tv.tv_usec = 1000 * (milliseconds % 1000);
  (void)select( 0, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &tv );
#endif

#if defined(VXWORKS) && defined(USE_VXWORKS_60TH_OF_A_SECOND)
  /* +++owen - task delay is in 1/60th of a second (0.01666666) */
  taskDelay(60 * seconds);
#endif

#ifdef VXWORKS  
  /* +++owen - needed a little more accuracy */
  TraceUDelay(milliseconds * 1000);
#endif
}

int
TraceBufferTimeGreaterEqual(TimeSpec_t *t1, TimeSpec_t *t2)
{
    uint32_t sec1 = t1->tv_sec;
    uint32_t usec1 = t1->tv_usec;

    uint32_t sec2 = t2->tv_sec;
    uint32_t usec2 = t2->tv_usec;

    if (sec1 > sec2)
    {
        /* Done */
        return 1;
    }
    else if (sec1 == sec2)
    {
        if (usec1 >= usec2)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    return 0;
}

void
TraceBufferTimeAdd(TimeSpec_t *start, TimeSpec_t *delta)
{
    start->tv_sec += delta->tv_sec;

    if (start->tv_usec + delta->tv_usec < 1000000)
    {
	start->tv_usec += delta->tv_usec;
    }
    else
    {  /* wrap it */
	start->tv_sec++;
	start->tv_usec = (start->tv_usec + delta->tv_usec) - 1000000;
    }
}

void
TraceBufferTimeSubtract(TimeSpec_t* start, TimeSpec_t* end, TimeSpec_t *rslt)
{
    /*
     * we have to check if we have a simple subtraction
     * or a carry-over.
     */
    if (end->tv_usec >= start->tv_usec)
    {
        if (end->tv_sec < start->tv_sec)
        {
            printf("Ending time less than start time\n");
        }
        rslt->tv_sec = end->tv_sec - start->tv_sec;
        rslt->tv_usec = end->tv_usec - start->tv_usec;
    }
    else
    {
        if (end->tv_sec < start->tv_sec)
        {
            printf("Ending time less than start time\n");
        }
        rslt->tv_sec = end->tv_sec - start->tv_sec - 1;
        rslt->tv_usec = USEC_IN_SEC - (start->tv_usec - end->tv_usec);
    }
}

void TimeStampCmdTest(CLI_PARSE_INFO *pInfo)
{
    char *timeAndDate1;
    char time1[80];
    char *timeAndDate2;
    char time2[80];
    time_t rawtime;
    struct tm *ptm;
    TimeSpec_t timerExpiration1;
    TimeSpec_t timerExpiration2;
    TimeSpec_t result;
    uint32_t i;
    char fileString[180];

#if 0    
    char *ptr;
    uint32_t traceID = 0;
#endif

    (*pInfo->print_fp)("Sleeping for 1 second\n");

    time(&rawtime);
    ptm = localtime(&rawtime);
    timeAndDate1 = asctime(ptm);
    strcpy(time1, timeAndDate1);

    OSSleep(1000);

    time(&rawtime);
    ptm = localtime(&rawtime);
    timeAndDate2 = asctime(ptm);
    strcpy(time2, timeAndDate2);
    
#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "  Time before: %s   Time after: %s\n", time1, time2);
#else
    (*pInfo->print_fp)("  Time before: %s   Time after: %s\n", time1, time2);
#endif
    
#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "Sleeping for 2 seconds\n");
#else
    (*pInfo->print_fp)("Sleeping for 2 seconds\n");
#endif
    
    time(&rawtime);
    ptm = localtime(&rawtime);
    timeAndDate1 = asctime(ptm);
    strcpy(time1, timeAndDate1);

    OSSleep(2000);

    time(&rawtime);
    ptm = localtime(&rawtime);
    timeAndDate2 = asctime(ptm);
    strcpy(time2, timeAndDate2);

#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "  Time before: %s   Time after: %s\n", time1, time2);
#else
    (*pInfo->print_fp)("  Time before: %s   Time after: %s\n", time1, time2);
#endif
    
#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "Sleeping for 2 seconds\n");
#else
    (*pInfo->print_fp)("Sleeping for 2 seconds: ");
#endif

    TraceBufferGetTime(&timerExpiration1);
    OSSleep(2000);
    TraceBufferGetTime(&timerExpiration2);
    TraceBufferTimeSubtract(&timerExpiration1, &timerExpiration2, &result);

    //(*pInfo->print_fp)("Time = %f\n", TIME_TO_REAL8(&result));
#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "%d.%d\n", result.tv_sec, (result.tv_usec * 1000000));
#else
    (*pInfo->print_fp)("%d.%d\n", result.tv_sec, (result.tv_usec * 1000000));
#endif
    
#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "Sleeping for 1 second: \n");
#else
    (*pInfo->print_fp)("Sleeping for 1 second: ");
#endif

    TraceBufferGetTime(&timerExpiration1);
    OSSleep(1000);
    TraceBufferGetTime(&timerExpiration2);
    TraceBufferTimeSubtract(&timerExpiration1, &timerExpiration2, &result);

#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "%d.%d\n", result.tv_sec, (result.tv_usec * 1000000));
#else
    (*pInfo->print_fp)("%d.%d\n", result.tv_sec, (result.tv_usec * 1000000));
#endif

    tbInitialized = 0;
    TBInit(pInfo->print_fp);

    TBStart(0, pInfo->print_fp);

#if defined(PPC7447) && defined(DEBUG_TRACE)
    sysSerialPrintf(__FILE__, __FUNCTION__, __LINE__, "OK", "How close do the following timestamps get to the 1 second interval?\n");
#else
    (*pInfo->print_fp)("One second intervals:\n");
#endif

    sprintf(fileString, "%s", "/home/omcgonag/very/very/very/very/long/cc/omcgonag_TRUNK_zcli/tracebuffer.c");

#if 0    
    if ((ptr = strstr(fileString, "/cc/")) != 0)
    {
        ptr += 4;
        /* Strip leading up to next backslash */
        if ((ptr = strchr(ptr, '/')) == NULL)
        {
            (*pInfo->print_fp)("INTERNAL ERROR: Could not find backslash\n");
            return;
        }
        ptr++;
    }
    else
    {
        ptr = &fileString[0];
    }
#endif    
    
    for (i = 0; i < 5; i++)
    {
//        TB_PRINTF_0(traceID, "Tick...\n");
        TBPrintf(0, __FILE__, __FUNCTION__, __LINE__, "Tick...\n", 0, 0, 0, 0, 0, 0); 
//        TBPrintf(0, ptr, __FUNCTION__, __LINE__, "Tick...\n", 0, 0, 0, 0, 0, 0); 

        OSSleep(1000);
    }

#ifndef Linux
    for (i = 0; i < 5; i++)
    {
        TB_PRINTF_0(traceID,"Tick...\n");
        taskDelay(1);
    }

    for (i = 0; i < 5; i++)
    {
        TB_PRINTF_0(traceID,"Tick...\n");
        taskDelay(2);
    }

    for (i = 0; i < 5; i++)
    {
        TB_PRINTF_0(traceID,"Tick...\n");
        taskDelay(3);
    }

    for (i = 0; i < 5; i++)
    {
        TB_PRINTF_0(traceID,"Tick...\n");
        taskDelay(4);
    }

    for (i = 0; i < 5; i++)
    {
        TB_PRINTF_0(traceID,"Tick...\n");
        taskDelay(6);
    }
#endif
    
    TBDisplay(0, 0, DISPLAY_ALL, DISPLAY_ALL, pInfo->print_fp);
}

char *timestr_v1(void)
{
    time_t t = time(0);
    struct tm *tt = localtime(&t);
    static char buf[32];
    sprintf (buf, "[%02d:%02d:%02d] ", tt->tm_hour, tt->tm_min, tt->tm_sec);
    return buf;
}

void timestr(char *buf)
{
    char *timeAndDate;
    time_t rawtime;
    struct tm *ptm;

    time(&rawtime);
    ptm = localtime(&rawtime);
    timeAndDate = asctime(ptm);

    strcpy(buf, timeAndDate);
}


int
log_printf(const char *format, ...)
{
    static char buf[4096];
    char timeBuffer[80];
    va_list args;
    int len;

    va_start(args ,format);
    len = vsnprintf(buf, 4096, format, args);
    va_end(args);

    if (logfile != NULL)
    {
        if (len != 0)
        {
            timestr(timeBuffer);
            timeBuffer[strlen(timeBuffer) - 1] = ' ';
            if (runInBackground == 1)
            {
                fprintf(logfile, "%s: %s", timeBuffer, buf);
                fflush(logfile);

                //openlog(SERVER_LOG_IDENTIFIER, LOG_PID, LOG_LOCAL2);
                //syslog(LOG_WARNING, "%s", buf);
                //syslog(LOG_WARNING, "%s", buf);
                //syslog(LOG_LOCAL2 | LOG_ALERT, "%s", buf);
                syslog(LOG_ALERT, "%s", buf);
                //closelog();
                sleep(1);
            }
            else
            {
                fprintf(stdout, "%s", buf);
                fflush(stdout);
            }
        }
        else
        {
            fprintf(stderr, "INTERNAL ERROR: could not write to log file %s\n", SERVER_LOG_TMP);
        }
    }

    return len;
}

int
log_printf_columns(const char *file, const char *function, int line, const char *status, const char *format, ...)
{
    char buf[4096];
    char bufLocation[4096]; 
    char timeBuffer[80];
    va_list args;
    int len;

    va_start(args ,format);
    len = vsnprintf(buf, 4096, format, args);
    va_end(args);

    // convert to fancier columns
    usrSPrintf(bufLocation, file, function, line, status, buf);

    if (logfile != NULL)
    {
        if (len != 0)
        {
            timestr(timeBuffer);
            timeBuffer[strlen(timeBuffer) - 1] = ' ';
            if (runInBackground == 1)
            {
                fprintf(logfile, "%s: %s", timeBuffer, bufLocation);
                fflush(logfile);

                //openlog(SERVER_LOG_IDENTIFIER, LOG_PID, LOG_LOCAL2);
                //syslog(LOG_WARNING, "%s", buf);
                //syslog(LOG_WARNING, "%s", bufLocation);
                //syslog(LOG_LOCAL2 | LOG_ALERT, "%s", bufLocation);

                syslog(LOG_ALERT, "%s", bufLocation);
                //closelog();
                sleep(1);
            }
            else
            {
                fprintf(stdout, "%s", bufLocation);
                fflush(stdout);
            }
        }
        else
        {
            fprintf(stderr, "INTERNAL ERROR: could not write to log file %s\n", SERVER_LOG_TMP);
        }
    }
    // +++hack
    else
    {
      fprintf(stdout, "%s", bufLocation);
      fflush(stdout);
    }

    return len;
}

int
log_tb_printf_columns(char *file, const char *function, int line, const char *status, const char *format, ...)
{
    char buf[4096];
    // char bufLocation[4096]; 
    va_list args;
    int len;

    va_start(args ,format);
    len = vsnprintf(buf, 4096, format, args);
    va_end(args);

    // convert to fancier columns
    // usrSPrintf(bufLocation, file, function, line, status, buf);

    // Put into tracebuffer
    TBPrintf(TB_ID_WDB, file, function, line, "%s\n", (PLATFORM_NATIVE_T)buf, 0, 0, 0, 0, 0);
    
    return len;
}

#ifndef UNIT_TEST

extern int taskIdFigure(int taskNameOrId);

void TBStack(char *file, const char *function, int line)
{
#if 0
#if 0
    unsigned int msrVal;
    uint32_t stackPtr = dumpGetSp();
    uint32_t stack1;
    uint32_t stack2;
    uint32_t stack3;
    uint32_t stack4;
    uint32_t stack5;

    if (traceState == TRACE_STOPPED)
    {
        return;
    }

    msrVal = intLock();

    /* Get one up (i.e., the caller) */
    stackPtr = *(uint32_t *)stackPtr;

    stack1 = *((uint32_t *)stackPtr + 1);
    stackPtr = *(uint32_t *)stackPtr;
    stack2 = *((uint32_t *)stackPtr + 1);
    stackPtr = *(uint32_t *)stackPtr;
    stack3 = *((uint32_t *)stackPtr + 1);
    stackPtr = *(uint32_t *)stackPtr;
    stack4 = *((uint32_t *)stackPtr + 1);
    stackPtr = *(uint32_t *)stackPtr;
    stack5 = *((uint32_t *)stackPtr + 1);

    TBPrintf(TB_ID_WDB, file, function, line, "[stack: 0x%08x 0x%08x 0x%08x]", (int)stack1, (int)stack2, (int)stack3, (int)stack4, (int)stack5, 0);

    intUnlock(msrVal);
#else
    int i;
    uint32_t stackPtr;
    stackInfo_t stack;
    REG_SET regs;
    int depth = 0;
    char *sp;
    uint32_t stackLower, stackUpper;
    extern unsigned long FREE_RAM_ADRS;
    int tid;
    TASK_DESC	td;			/* task info structure */

    tid = taskIdFigure (0);
    taskInfoGet (tid, &td);
    
    sp = (char *)td.td_stackCurrent;
    
    TBPrintf(TB_ID_WDB, file, function, line, "sp = 0x%08x", (int)sp, 0, 0, 0, 0, 0);

    stackPtr = (uint32_t)sp;

    memset(&stack, 0, sizeof(stackInfo_t));

    //dumpSaveStack(&stack);
    //stackPtr = dmpAsmRegsSave[DMP_SAVREG_INDX(DMP_SAVREG_SP)];

    stackLower = (uint32_t) &FREE_RAM_ADRS;
    stackUpper = stackLower + 5000 /*ISR_STACK_SIZE*/;
    if ((stackPtr < stackLower) || (stackPtr > stackUpper))
    {
        stackLower = (uint32_t)td.td_pStackEnd - OSL_TASK_STACK_MMS_HEADER_ROOM;
        stackUpper = (uint32_t)td.td_pStackBase;
    }

    while (1)
    {
        if (stackPtr == 0)
        {
            break;
        }

        if (stackPtr > stackUpper ||
            stackPtr < stackLower)
        {
            stack.flags |= OUT_OF_RANGE;
            break;
        }
        if ((stackPtr & ~0x3) != stackPtr)
        {
            stack.flags |= UNALIGNED_SP;
            break;
        }
        if (depth >= SAVE_DEPTH)
        {
            stack.flags |= EXCEED_MAX_BACKTRACE;
            break;
        }
        if (depth >= 0)
        {
            stack.backtrace[depth] = *((uint32_t *)stackPtr+1);
        }
        stackPtr = *(uint32_t *)stackPtr;
        depth++;
    }

    if (stack.flags & UNALIGNED_SP)
    {
        TBPrintf(TB_ID_WDB, file, function, line, "(invalid)\r\n", 0, 0, 0, 0, 0, 0);
    }
    else
    {
        if (stack.flags & EXCEED_MAX_BACKTRACE)
        {
            TBPrintf(TB_ID_WDB, file, function, line, "(last function pointers before task crashed)\r\n", 0, 0, 0, 0, 0, 0);
        }
        else
        {
            TBPrintf(TB_ID_WDB, file, function, line, "\n", 0, 0, 0, 0, 0, 0);
        }

        for (i=0; i<SAVE_DEPTH; i++)
        {
            TBPrintf(TB_ID_WDB, file, function, line, "0x%08x ",stack.backtrace[i], 0, 0, 0, 0, 0);
        }

        TBPrintf(TB_ID_WDB, file, function, line, "\n", 0, 0, 0, 0, 0, 0);
    }
#endif
#endif
}
#endif /* !UNIT_TEST */


#ifdef UNIT_TEST

/* file: cli/cliconvert.c */

/*------------------- cliCharToLong() ----------------------
 *
 * FUNCTION:
 * - Converts a char value (like argv[x]) to an long
 *
 * ARGS:
 * - Char string
 *
 * RETURN CODES:
 * - signed value
 *
 *------------------------------------------------------------*/
unsigned long cliCharToLong( char *s )
{
    char *p;
    unsigned long v;

    if( s != NULL )
    {
        p = strstr(s, "0x");
        if( p == s )
        {
            v = strtol(&s[2], NULL, 16);
        }
        else
        {
            v = strtol(s, NULL, 10);
        }
    }
    else
    {
        v = 0;
    }
    return v;
}


/*------------------- cliCharToUnsignedLong() ----------------------
 *
 * FUNCTION:
 * - Converts a char value (like argv[x]) to an unsigned long
 *
 * ARGS:
 * - Char string
 *
 * RETURN CODES:
 * - unsigned value
 *
 *------------------------------------------------------------*/
unsigned long cliCharToUnsignedLong( char *s )
{
    char *p;
    unsigned long v;

    if( s != NULL )
    {
        p = strstr(s, "0x");
        if( p == s )
        {
            v = strtoul(&s[2], NULL, 16);
        }
        else
        {
            v = strtoul(s, NULL, 10);
        }
    }
    else
    {
        v = 0;
    }
    return v;
}

/* file: util/strmatch.c */

/*
 * FUNCTION:	iterator
 *
 * DESCRIPTION:	Iterate over a collection of strings that are match candidates
 *
 * INPUTS:	iteration
 *                  iteration number, starting from 0 (this could be used, for
 *                  example, as an index into an array)
 *              closure
 *                  arbitrary void pointer provided in the call to strFindMatch
 *
 * OUTPUTS:	None
 *
 * RETURNS:	Pointer to const string, or NULL to indicate no more strings.
 *
 * NOTES:	This is only the typedef for an iterator function.
 */

typedef const char * (* strFindMatchIterator_t) (
    int    iteration,
    void * closure
    );

/*
 * FUNCTION:	defaultIterator
 *
 * DESCRIPTION:	Simple iterator to use with strFindMatch.  This iterator
 *              assumes the closure argument points to an array of const char*
 *              pointers, terminated by a NULL pointer.
 *
 * INPUTS:	iteration
 *                  iteration number, starting from 0 (used to index into the
 *                  array of const char* pointers)
 *              closure
 *                  const char** pointer (an array of char*)
 *
 * OUTPUTS:	None
 *
 * RETURNS:	Pointer to next string to match, or NULL at the end.
 *
 * NOTES:	
 *
 */

static const char * defaultIterator (int iteration, void * closure)
{
    return (((const char **) closure) [iteration]);
}

/*
 * FUNCTION:	strFindMatch
 *
 * DESCRIPTION:	Find the nearest match for a given string
 *
 * INPUTS:	string
 *                  string for which the function finds a match
 *
 *              iterator
 *                  callback function to provide candidate strings
 *
 *              closure
 *                  void pointer to be passed to the iterator callback
 *
 * OUTPUTS:	match
 *                  index of the best match found (or the first match if
 *                  multiple equally good matches are found)
 *
 * RETURNS:	Number of matches found
 *
 * NOTES:	As a convenience, if iterator is NULL, we will use the
 *              iterator defaultIterator above and assume closure points
 *              to an array of char* pointers.
 */

unsigned long strFindMatch (
    const char * string,
    int * match,
    strFindMatchIterator_t iterator,
    void * closure)
{
    unsigned long match_len = 0, i = 0, count = 0;
    const char * candidate;
    const char * matched_string = NULL;


    if (iterator == NULL)
    {
        iterator = defaultIterator;
    }

    * match   = -1;
    if (string == NULL) {
      return 0;
    }
    
    candidate = (* iterator) (0, closure);

    while (candidate != NULL)
    {
        unsigned long j;
        bool_t mismatch;

        for (mismatch = FALSE, j = 0;
             (candidate[j] != '\0') && (string[j] != '\0');
             ++ j)
        {
            if (tolower (candidate[j]) != tolower (string[j]))
            {
                mismatch = TRUE;
                j = 0;
                break;
            }
        }
	/* check for the string being longer than the candidate. */
	if (candidate[j] =='\0' && string[j]!='\0') {
	  mismatch = TRUE;
	}

        if (mismatch == FALSE)
        {
            if (match_len < j)
            {
                matched_string = candidate;
                match_len      = j;
                * match        = i;
                count          = 1;
            }
            else if (match_len == j)
            {
	      count += 1;
            }
            else
            {
                /* This match is shorter than the best we've seen. */
            }

	    if (j == strlen(candidate))
	    {
	      /* If the match is exact for the current candidate,
	       *  consider this one as our only one.
	       */
	      count =1;
	      matched_string = candidate;
	      *match = i;
	      break;
	    }
	}

        i += 1;
        candidate = (* iterator) (i, closure);
    }

    /*
     * If the matched string is a prefix of the input string, don't consider
     * this a match.  (It's suboptimal to have such a case!)
     */

    if (matched_string != NULL)
    {
        if (strlen (string) > strlen (matched_string))
        {
            * match = -1;
            count   = 0;
        }
    }

    return count;
}

/* file: cli/cli.c */

#define CLI_SEP_CHARS   " \t"

#define CLI_IS_COMMAND_HIDDEN(cmd) (cmd[0] == '.' ? TRUE:FALSE)

static void cmd_exit(CLI_PARSE_INFO *info)
{
    exit(0);
}

static void cmd_echo(CLI_PARSE_INFO *info)
{
    int i;
    
    if ( info->argc == 1)
    {
        (info->print_fp)("\n");
        return;
    }
    
    /* Echo message to console */

    for (i=1; i < info->argc; ++i)
    {
        (info->print_fp)("%s ", info->argv[i]);
    }

    (info->print_fp)("\n");
}

static void cmd_quit(CLI_PARSE_INFO *info)
{
    exit(0);
}


// #define LOG_PRINTF(format, args...) log_printf_columns(__FILE__, __FUNCTION__, __LINE__, "OK", format, ##args)

#define LOG_PRINTF_TEST1(format, args...) \
    log_printf_columns("/home/omcgonag/cc/omcgonag_attach_raghsrid_TRUNK_3/readline/openssl/usbc_gnu_64/crypto/zcli_server/remoteclient.c", __FUNCTION__, __LINE__, "OK", format, ##args)

#define LOG_PRINTF_TEST2(format, args...) \
           log_tb_printf_columns("/home/omcgonag/cc/omcgonag_attach_raghsrid_TRUNK_3/readline/openssl/usbc_gnu_64/crypto/zcli_server/remoteclient.c", __FUNCTION__, __LINE__, "OK", "IT_WORKED: " format, ##args)

void UnitTestCmd(CLI_PARSE_INFO *info)
{
    int i;
    char testBuffer[256];
    int testNum = 0;
    int traceID = 0;
    
    PRINT_FP print_fp = info->print_fp;

    tbInitialized = 0;
    TBInit(info->print_fp);

    TBStart(0, print_fp);

    LOG_PRINTF("%s", "HELLO");

#if 0
    LOG_PRINTF_TEST1("%s", "HELLO");
    LOG_PRINTF_TEST2("%s", "HELLO");

    LOG_PRINTF_TEST2("%s", "200+ characters in this message....................................................................................................................................................................abcd123456789ABCD");

    LOG_PRINTF_TEST2("%s", "500+ characters in this message........................WE SHOULD SEE AN ERROR ON MESSAGE TOO LONG............................................................................................................................................abcd123456789ABCD..............................................................................................................................................................................................................................................................................................................................................................................................................");
#endif

   debug_print(mod_srtp, "!!!!! IT IS ALIVE !!!! waiting here forever.....\n");
    
   for (i=0; i<256; i++)
   {
       testBuffer[i] = i;
   }

   TBHexDump(__FILE__, 0 /*__FUNCTION__*/, __LINE__, (unsigned int *)testBuffer, 256);


   TBDisplay(0, 0, DISPLAY_ALL, DISPLAY_ALL, print_fp);

// +++owen - temporarily disable for now
#if 0
    TB_PRINTF_0(traceID,"Testing, 1, 2, 3, ...\n");
    TB_PRINTF_0(traceID,"Testing, 2, 2, 3, ...\n");
    TB_PRINTF_0(traceID,"Testing, 3, 2, 3, ...\n");
    TB_PRINTF_0(traceID,"Testing, 4, 2, 3, ...\n");
    TB_PRINTF_0(traceID,"Testing, 5, 2, 3, ... IN\n");
    OSSleep(111);
    TB_PRINTF_0(traceID,"Testing, 6, 2, 3, ... IN\n");
    OSSleep(222);
    TB_PRINTF_0(traceID,"Testing, 7, 2, 3, ... IN\n");
    OSSleep(333);
    TB_PRINTF_0(traceID,"Testing, 8, 2, 3, ...\n");
    TB_PRINTF_0(traceID,"Testing, 9, 2, 3, ...\n");
    OSSleep(1000);
    TB_PRINTF_0(traceID,"Testing, A, 2, 3, ... IN\n");
    TB_PRINTF_0(traceID,"Testing, B, 2, 3, ...\n");
    TB_PRINTF_0(traceID,"Testing, C, 2, 3, ...\n");
    TBDisplay(0, 0, DISPLAY_ALL, DISPLAY_ALL, print_fp);

    (info->print_fp)("\nFiltering in on \"IN\", should find 3:\n");
    TBFind("IN", print_fp);


    (info->print_fp)("\nFiltering in on \"Testing\", should find 12:\n");
    TBFind("Testing,", print_fp);

    (info->print_fp)("\nDisplaying first 2 entries:\n");
    TBDisplay(0, 0, 1, 2, print_fp);

    (info->print_fp)("\nDisplaying first 100 entries:\n");
    TBDisplay(0, 0, 1, 100, print_fp);

    (info->print_fp)("\nDisplaying 1st, 2nd, 3rd, and 4th entries:\n");
    TBDisplay(0, 0, 3, 4, print_fp);

    (info->print_fp)("\nDisplaying 1st, 2nd, and 3rd entries\n");
    TBDisplay(0, 0, 3, 3, print_fp);

    (info->print_fp)("\nFiltering in on \"Testing, A\":\n");
    TBFind("Testing, A", print_fp);

    (info->print_fp)("\nFiltering in on \"Testing\", should find 12:\n");
    TBFind("Testing,", print_fp);

    tbInitialized = 0;
    TBInit(info->print_fp);

    (info->print_fp)("\nTesting repeat counter:\n");
    for (i=0; i<10000; i++)
    {
        TB_PRINTF_0(traceID,"HOOHA\n");
    }
    TBDisplay(0, 0, 3, 4, print_fp);

    tbInitialized = 0;
    TBInit(info->print_fp);

    (info->print_fp)("\nTesting wrap around capabilities: ONLY 4 entries\n");
    for (i=0; i<10000; i++)
    {
        TB_PRINTF_1(traceID, "HOOHA = %d\n", i);
    }
    TBDisplay(0, 0, 3, 4, print_fp);

    for (i=0; i<256; i++)
    {
        testBuffer[i] = i;
    }

    (info->print_fp)("\nTesting hexdump\n");
    TBHexDump(__FILE__, 0 /*__FUNCTION__*/, __LINE__, (unsigned int *)testBuffer, 256);
    TBDisplay(0, 0, DISPLAY_ALL, DISPLAY_ALL, print_fp);

    tbInitialized = 0;
    TBInit(info->print_fp);

    (info->print_fp)("\nTesting wrap around capabilities: ALL entries\n");
    for (i=0; i<10000; i++)
    {
        TB_PRINTF_1(traceID, "HOOHA = %d\n", i);
    }
    TBDisplay(0, 0, DISPLAY_ALL, DISPLAY_ALL, print_fp);

    (info->print_fp)("\nTesting FIND during wrap around capabilities: search for \"HOOHA = 9493\"\n");
    TBFind("HOOHA = 9493", print_fp);
#endif
}

static void TraceStartCmd(CLI_PARSE_INFO *pInfo)
{
    int i;
    uint32_t traceID;
    
    if ( pInfo->argc == 1)
    {
        for (i = 0; i < MAX_TRACE_BUFFERS; i++)
        {
            TBStart(i, pInfo->print_fp);
        }
    }
    else
    {
        traceID = cliCharToUnsignedLong(pInfo->argv[1]);
        TBStart(traceID, pInfo->print_fp);
    }
}

static void TraceStopCmd(CLI_PARSE_INFO *pInfo)
{
    int i;
    uint32_t traceID;
    
    if ( pInfo->argc == 1)
    {
        for (i = 0; i < MAX_TRACE_BUFFERS; i++)
        {
            TBStop(i, pInfo->print_fp);
        }
    }
    else
    {
        traceID = cliCharToUnsignedLong(pInfo->argv[1]);
        TBStop(traceID, pInfo->print_fp);
    }
}

static void TraceClearCmd(CLI_PARSE_INFO *pInfo)
{
    tbInitialized = 0;
    TBInit(pInfo->print_fp);
}

static void TraceDisplayCmd(CLI_PARSE_INFO *pInfo)
{
    uint32_t startIndex = 0;
    uint32_t endIndex = DISPLAY_ALL;
    uint32_t count = DISPLAY_ALL;
    uint32_t previousState = traceState;

    if ( pInfo->argc == 1)
    {
        (*pInfo->print_fp)("INFO: %s {count} | {start} {stop} [{count}]\n", pInfo->argv[0]);
    }
    else if ( pInfo->argc == 2)
    {
        count = cliCharToUnsignedLong(pInfo->argv[1]);
    }
    else if ( pInfo->argc == 3)
    {
        startIndex = cliCharToUnsignedLong(pInfo->argv[1]);
        endIndex = cliCharToUnsignedLong(pInfo->argv[2]);
    }
    else if ( pInfo->argc == 4)
    {
        startIndex = cliCharToUnsignedLong(pInfo->argv[1]);
        endIndex = cliCharToUnsignedLong(pInfo->argv[2]);
        count = cliCharToUnsignedLong(pInfo->argv[3]);
    }

    (*pInfo->print_fp)("%s: startIndex = %ld  endIndex = %ld  count = %ld\n",
                       pInfo->argv[0], startIndex, endIndex, count);

    TBStop(0, pInfo->print_fp);
    
    TBDisplay(0,startIndex, endIndex, count, pInfo->print_fp);

    if (previousState == TRACE_RUNNING)
    {
        TBStart(0, pInfo->print_fp);
    }
}

static void TraceFindCmd(CLI_PARSE_INFO *pInfo)
{
  if ( pInfo->argc != 2)
  {
    (*pInfo->print_fp)("USAGE: %s {string}\n", pInfo->argv[0]);
    return;
  }

  TBFind(pInfo->argv[1], pInfo->print_fp);
}

static void TraceInfoCmd(CLI_PARSE_INFO *pInfo)
{
    TraceInfoDisplay(pInfo->print_fp);
}

static void TraceFilter(CLI_PARSE_INFO *pInfo)
{
    (*pInfo->print_fp)("USAGE: <not yet implemented>\n");
}

static void TraceColumnCmdShow(CLI_PARSE_INFO *pInfo)
{
    int i;

    for (i = 0; i < MAX_COLUMNS; i++)
    {
      (*pInfo->print_fp)("column %d is %s\n", i, (columnState[i].on == 1) ? "on" : "off");
    }
}

static void TraceColumnSetState1(CLI_PARSE_INFO *pInfo)
{
    if (pInfo->argc < 2)
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        columnState[0].on = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        columnState[0].on = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
    }
}

static void TraceColumnSetState2(CLI_PARSE_INFO *pInfo)
{
    if (pInfo->argc < 2)
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        columnState[1].on = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        columnState[1].on = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
    }
}

static void TraceColumnSetState3(CLI_PARSE_INFO *pInfo)
{
    if (pInfo->argc < 2)
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        columnState[2].on = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        columnState[2].on = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
    }
}

static void TraceColumnSetState4(CLI_PARSE_INFO *pInfo)
{
    if (pInfo->argc < 2)
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        columnState[3].on = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        columnState[3].on = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
    }
}

static void TraceColumnSetState5(CLI_PARSE_INFO *pInfo)
{
    if (pInfo->argc < 2)
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        columnState[4].on = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        columnState[4].on = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
    }
}

static const CLI_PARSE_CMD traceColumnSetState [] =      
{                                                        
    { "1",         TraceColumnSetState1,    "turn column 1 on/off" },
    { "2",         TraceColumnSetState2,    "turn column 2 on/off" },
    { "3",         TraceColumnSetState3,    "turn column 3 on/off" },
    { "4",         TraceColumnSetState4,    "turn column 4 on/off" },
    { "5",         TraceColumnSetState5,    "turn column 5 on/off" },
    { NULL,        NULL,                NULL                               }
};


static void TraceColumnCmdSetState(CLI_PARSE_INFO *pInfo)
{
    cliDefaultHandler( pInfo, traceColumnSetState );
}

static const CLI_PARSE_CMD traceColumnCmds [] =
{
    { "show",      TraceColumnCmdShow,      "Show trace settings" },
    { "set",       TraceColumnCmdSetState,  "Set column on/off"   },
    { NULL,        NULL,                NULL                               }
};

static void TraceColumnCmd(CLI_PARSE_INFO *info)
{
    TBInit(info->print_fp);

    cliDefaultHandler( info, traceColumnCmds );
}


static const CLI_PARSE_CMD tracebufferCommands [] =
{
    { "display",      TraceDisplayCmd,    "Display contents of trace buffer"  },
    { "reset",        TraceClearCmd,      "Clear trace buffer contents"       },
    { "start",        TraceStartCmd,      "Start trace"                       },
    { "stop",         TraceStopCmd,       "Stop trace"                        },
    { "find",         TraceFindCmd,       "Use {string} to filter display"    },
    { "test",         UnitTestCmd,        "Unit test(s)"                      },
    { "info",         TraceInfoCmd,       "trace information"                 },
    { "timestamp",    TimeStampCmdTest,   "test timestamp code"               },
    { "column",       TraceColumnCmd,     "column on/off commands"            },
    { NULL,           NULL,                NULL                               }
};

static void cmd_tracebuffer(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, tracebufferCommands );
}

/* +++owen - need it global to handle the concept of includeing "run" command */
static char cmdLineSource[CMD_BUF_LEN];
static void cmd_source(CLI_PARSE_INFO *info)
{
    int i,j;
    FILE *fp ;
    int loop;
    int loopCnt = 1;
    char fileName[120];
    int sessionId;
    int stringEmpty;
    char sessionString[100];
    
    if (info->argc < 2)
    {
        printf("USAGE: %s {filename} [repeat]\n", info->argv[0]);
        return;
    }

    if (info->argc >= 2)
    {
      // filename MUST be copied because argv[1] is overwritten with command line from file being sourced
      strcpy(fileName, info->argv[1]);
    }
    
    if (info->argc == 3)
    {
        loopCnt = cliCharToUnsignedLong(info->argv[2]);
    }
    
    for (loop = 1; loop <= loopCnt; loop++)
    {
        /* open input file containing DCP commands */
        fp = fopen(fileName, "r");

        if (!fp)
        {
            printf("ERROR: Couldn't open %s for reading, loop = %d\n", fileName, loop);
            return;
        }

        while (1)
        {
            if (!fgets(cmdLineSource, CMD_BUF_LEN, fp))
            {
                /* Done reading file */
                break;
            }

            /* Check if line is empty of just full of spaces */
            stringEmpty = 1;
            for (i = 0; i < CMD_BUF_LEN; i++)
            {
                if (cmdLineSource[i] == '\0')
                {
                    break;
                }

                if ((cmdLineSource[i] != ' ') && (isprint(cmdLineSource[i])))
                {
                    stringEmpty = 0;
                    break;
                }
            }

            /* Skip comment lines */
            if ((strstr(cmdLineSource, "#") != 0) ||
                (stringEmpty == 1))
            {
                //printf("INFO: string is EMPTY or a comment!!\n");
                continue;
            }

            /* Get rid of linefeed <lf> */
            cmdLineSource[strlen(cmdLineSource) - 1] = '\0';
                              
            /* +++owen - special case of run commands in script */
            if (strstr(cmdLineSource, "run") != 0)
            {
               //printf("cmd_source: COMMAND = %s\n", commandRunBuffer);
               // +++owen - find keyword run
               i = 0;
               j = 0;
                    
               if (cmdLineSource[i] != 'r')
               {
                  while (cmdLineSource[i] != 'r')
                  {
                     if (i > CMD_BUF_LEN)
                     {
                        (info->print_fp)("ERROR: format of command = %s incorrect\n", cmdLineSource);
                        return;
                     }
                     i++;
                  }
               }

               // +++owen - find first space after run
               if (cmdLineSource[i] != ' ')
               {
                  while (cmdLineSource[i] != ' ')
                  {
                     if (i > CMD_BUF_LEN)
                     {
                        (info->print_fp)("ERROR: format of command = %s incorrect\n", cmdLineSource);
                        return;
                     }
                     i++;
                  }
               }

               // +++owen - find next non-space
               if (cmdLineSource[i] == ' ')
               {
                  while (cmdLineSource[i] == ' ')
                  {
                     if (i > CMD_BUF_LEN)
                     {
                        (info->print_fp)("ERROR: format of command = %s incorrect\n", cmdLineSource);
                        return;
                     }
                     i++;
                  }
               }
                    
               /* Handle it locally, these commands not part of FNS command table */
               cliEntry(cmdLineSource, &printf);
            }
            else
            {
               cliEntry(cmdLineSource, &printf);
            }

        } // while(1)
        
        fclose(fp);

    } // for (loop=0; loop < loopCnt; loop++)
}

CLI_PARSE_CMD cliRocEntry[] =
{
    { "help",             cmdHelp,             "Show Help"},
    { ".help",            cmdHelp,             "Show Help"},
    { "?",                cmdHelp,             "Show Help"},
    { ".?",               cmdHelp,             "Show Help"},
    { "exit",             cmd_exit,            "exit" },
    { "echo",             cmd_echo,            "echo" },
    { "quit",             cmd_quit,            "quit" },
    { "tracebuffer",      cmd_tracebuffer,     "tracebuffer"},
    { "party",            cmd_party,           "party commands"},
    { "source",           cmd_source,          "source" },
    { NULL,               NULL,                NULL }
};

/*---------------------- cliPrintHelp() ----------------------------
 *
 * FUNCTION:
 * - Print help messages
 *
 * ARGS:
 * - info: Parse context
 * - cmds: current command location
 *
 * RETURNS:
 * - none
 *
 *-----------------------------------------------------------------*/
void cliPrintHelp( CLI_PARSE_INFO *info, const CLI_PARSE_CMD *cmds )
{
    unsigned long i;
    bool_t show_hidden;

    if( info->argc != 0 && info->argv[0][0] == '.' )
    {
        show_hidden = TRUE;
    }
    else
    {
        show_hidden = FALSE;
    }

    /*
    ** Print all matched command tokens
    */
    (info->print_fp)( "USAGE: " );
    for( i=0 ; i<(info->level-1) ; i++ )
        if( info->orig_argv[i] != NULL )
        {
            (info->print_fp)( "%s ", info->orig_argv[i] );
        }

    (info->print_fp)( "[ " );
    for( i=0 ; cmds[i].cmd_name != NULL ; i++ )
    {
        if( (CLI_IS_COMMAND_HIDDEN(cmds[i].cmd_name) == FALSE) ||
            (show_hidden == TRUE) )
        {
            (info->print_fp)( "%s ", cmds[i].cmd_name );
        }
    }
    (info->print_fp)( "]\r\n" );


    for( i=0 ; cmds[i].cmd_name != NULL ; i++ )
    {
        if( (CLI_IS_COMMAND_HIDDEN(cmds[i].cmd_name) == FALSE) ||
            (show_hidden == TRUE) )
        {
            (info->print_fp)( "%15s: %s\r\n", 
                cmds[i].cmd_name,
                cmds[i].help == NULL ? "" : cmds[i].help
                );
        }
    }
 
    return;
}

static const char *cliNextCommandName(
    int index,
    void *closure )
{
    const CLI_PARSE_CMD *cmds = (const CLI_PARSE_CMD *) closure;
    return cmds[index].cmd_name;
}

/*------------------- cliFindCommand() ----------------------
 *
 * FUNCTION:
 * - Finds the closest commnad matching user input
 *
 * ARGS:
 * - cmds:  command table
 * - argv:  Command from argv[0]
 * - match: returned match
 *
 * RETURN CODES:
 * - Number of matches found
 *
 *------------------------------------------------------------*/
static unsigned long cliFindCommand( 
    const CLI_PARSE_CMD *cmds, 
    char *argv, 
    const CLI_PARSE_CMD **match )
{
    int match_index;
    unsigned long count;


    count  = strFindMatch( argv, &match_index, cliNextCommandName, (CLI_PARSE_CMD *) cmds );
    
    if( match_index != -1 )
    {
        *match = &cmds[match_index];

        if( strlen(argv) > strlen((*match)->cmd_name) )
        {
            *match = NULL;
            count = 0;
        }
    }

    return count;
}

/*------------------- cliParseNextLevel() ----------------------
 *
 * FUNCTION:
 * - Parses a second level of table.
 * - Matches the argv[0] on a subset of commands
 * - When a unique match is found, the handler is called with 
 *   (argc-1) and the arguments starting at argv[1].
 *
 * ARGS:
 * - argc:  Command line argument count
 * - argv:  Cmmand line arguments
 * - env: Current envioronemnt 
 * - cmds:  List of command names+handlers to match on.
 *
 * RETURN CODES:
 * - CLI_PARSE_MATCHED:  Command matched successfully.
 * - CLI_PARSE_AMBIGUOUS:  More than one command matched.
 * - CLI_PARSE_NOMATCH:  No match found.
 * - CLI_PARSE_NOARG: No argument found
 *
 *------------------------------------------------------------*/
CLI_PARSE_OUTCOME cliParseNextLevel( CLI_PARSE_INFO *info, const CLI_PARSE_CMD *cmds)
{
    unsigned long count;
    const CLI_PARSE_CMD *match;
    CLI_PARSE_OUTCOME outcome;

    /*
    ** Decrement argc, slide all arguments by one
    */
    #if 0
    info->argc--;
    info->argv = &(info->argv[1]);
    #endif
    info->argc = info->orig_argc - info->level;
    info->argv = &(info->orig_argv[info->level]);

    /*
    ** Increment parse level
    */
    info->level++;

    if( info->argv[0] != NULL )
    {
        count = cliFindCommand( cmds, info->argv[0], &match );
    }
    else
    {
        count = 0;
    }
    switch( count )
    {
        case 0:
            if( info->argv[0] == NULL )
            {
                outcome = CLI_PARSE_NOARG;
            }
            else
            {
                outcome = CLI_PARSE_NOMATCH;
            }
            break;

        case 1:
            outcome = CLI_PARSE_MATCHED;
			/*lint -e(644) */
            (match->fp) ( info );
            break;
            
        default:
            outcome = CLI_PARSE_AMBIGUOUS;
            break;
    }
    return outcome;
}

/*-------------------------- cliDefaultHandler() ---------------------
 *
 * FUNCTION:
 *  - Default Handler suitable for most commands
 *  - When a command is not matched, 
 *
 * ARGS:
 * - info: Parse context
 * - cmds: current command location
 *
 * RETURNS:
 * - none
 *
 *------------------------------------------------------------*/
CLI_PARSE_OUTCOME cliDefaultHandler( CLI_PARSE_INFO *info, const CLI_PARSE_CMD *cmds)
{
    CLI_PARSE_OUTCOME outcome;

    outcome = cliParseNextLevel(info, cmds );
    switch( outcome )
    {
        case CLI_PARSE_NOARG:
                cliPrintHelp( info, cmds );
                break;

        case CLI_PARSE_NOMATCH:
            (info->print_fp)( "%s: Command or sub-command is bogus\r\n", 
                info->argv[0] );
            break;

        case CLI_PARSE_MATCHED:
            break;

        case CLI_PARSE_AMBIGUOUS:
            (info->print_fp)( "%s: Ambiguous (sub) command\r\n", 
                info->argv[0] );
            break;

    }
    return outcome;
}

void cliEntry( char *input_line, PRINT_FP print_fp )
{
    CLI_PARSE_INFO info;
    char *curPointer;

    if( input_line != NULL )
    {

        info.orig_argc = 0;
#if 0 /* defined (WIN32) */
        info.orig_argv[info.orig_argc] = strtok( input_line, CLI_SEP_CHARS);
#else
        info.orig_argv[info.orig_argc] = strtok_r( input_line, CLI_SEP_CHARS, &curPointer);
#endif

        while( info.orig_argv[info.orig_argc] != NULL )
        {
            info.orig_argc++;
            if( info.orig_argc == CLI_MAX_ARGS_ZCLI )
            {
                break;
            }
#if 0 /* defined (WIN32) */
            info.orig_argv[info.orig_argc] = strtok( NULL, CLI_SEP_CHARS);
#else
            info.orig_argv[info.orig_argc] = strtok_r( NULL, CLI_SEP_CHARS, &curPointer );
#endif
        }

        info.argc = info.orig_argc;
        info.level = 0;
        info.argv = info.orig_argv;
        info.p = NULL;                                     
        info.print_fp = print_fp;

        if( info.argc > 0 )
        {
            cliDefaultHandler( &info, cliRocEntry );
        }
    }

    return;
}

/* file: clicmds_common.c */

void cmdHelp( CLI_PARSE_INFO *info)
{
  cliPrintHelp( info, cliRocEntry );
}

void
PrintUsage(int exitFlag)
{
    printf("Usage: rcli [OPTION...]\n");
    printf("\n");
    printf("  -D, --DDR          start DSP Data Record (DDR) recording...\n");
    printf("  -P, --PDR          start PFE/DSP Data Record (PDR) recording...\n");
    printf("  -L, --Local        run locally - do not connect to server (q20 support)\n");
    printf("  -T, --Top          top - screen update every -T {seconds}\n");
    printf("  -R, --Refresh      top - idle task refresh rate (default - refresh after 4 idle ticks)\n");
    printf("  -C, --Cut          top - cut idle and non-idle tasks into two tables (default single table)\n");
    printf("  -F, --Flow         top - disable automatic reset of screen - enable flow of results\n");
    printf("  -s, --script       run script 1/second\n");
    printf("  -r, --rate         change rate of execution\n");
    printf("  -I, --ip-primary   primiary servier IP address\n");
    printf("  -S, --ip-secondary secondary servier IP address\n");
    printf("  -u, --usdp         connect to USDP (default USBC)\n");
    printf("  -m, --monitor      connect to monitor (default USBC)\n");
    printf("  -e, --media        connect to media (default USBC)\n");
    printf("  -U, --unittest     connect to UNITTEST jig\n");
    printf("  -d, --debug        turn debug messages on (1) or off (0)\n");
    printf("  -?, --help         show this help information\n");
    printf("  -h, --help         show this help information\n");

    if (exitFlag == 1)
    {
        exit(-1);
    }
}

int
main(int argc, char *argv[])
{
    char input [4096];
    char c;
    int idx = 0;
    CLI_PARSE_INFO pInfo;
    
    int repeatLastEnabledLocal = 0;
    static char *line_read = (char *)NULL;
    int firstTime = 1;
    int firstSession = 1;

#if 0
    input[0] = '\0';
    while (1)
    {
        printf ("partytime >> ");

        while((c = getchar()) != EOF)
        {
            input[idx++] = c;

            if (c == '\n')
            {
                break;
            }
        }

        if (idx == 0)
        {
            continue;
        }
        
        /* get rid of newline */
        input[idx-1] = '\0';
         
        cliEntry(input, &printf);
        idx = 0;
        input[0] = '\0';
    }
#else
    /* Create a new session info structures */
    pInfo.print_fp = printf;

    arguments.captureScript[0] = '\0';
    arguments.rate = 1;
    arguments.alignment = 1;
    arguments.debug = 0;
    arguments.showFatal = 0;
    arguments.ddr = 0;
    arguments.pdr = 0;
    arguments.local = 0;
    arguments.top = 0;
    arguments.idleRefresh = 4;
    arguments.idleCut = 0;
    arguments.topFlow = 0;
    arguments.topRefresh = 1;
    arguments.ip_primary[0] = '\0';
    arguments.ip_secondary[0] = '\0';

    while (1) 
    {
        int option_index = 0;

        static struct option long_options[] = 
            {
                {"DDR",               no_argument,          0, 'D'},
                {"PDR",               no_argument,          0, 'P'},
                {"Local",             no_argument,          0, 'L'},
                {"Top",               required_argument,    0, 'T'},
                {"Refresh",           required_argument,    0, 'R'},
                {"Cut",               no_argument,          0, 'C'},
                {"Flow",              no_argument,          0, 'F'},
                {"script",            required_argument,    0, 's'},
                {"rate",              required_argument,    0, 'r'},
                {"ip-primary",        required_argument,    0, 'I'},
                {"ip-secondary",      required_argument,    0, 'S'},
                {"usdp",              no_argument,          0, 'u'},
                {"monitor",           no_argument,          0, 'm'},
                {"media",             no_argument,          0, 'e'},
                {"unittest",          no_argument,          0, 'U'},
                {"debug",             required_argument,    0, 'd'},
                {"help",              no_argument,          0, '?'},
                {"help",              no_argument,          0, 'h'},
                {0,0,0,0}
            };

        // +++owen - word to the wise - for example, since 'D' does not require a parameter, DO NOT
        // put semi-colon after it int the following list. a semi-colon after indicates required_argument, OVERIDING
        // the forced (supposed) setting above...
        
        c = getopt_long_only(argc, argv, "DPLT:R:CFs:r:I:S:umeUd:h?",
                             long_options, &option_index);
        
        if (c == -1)
            break;

        switch (c) 
        {
        case 0:
            printf("option %s", long_options[option_index].name);
            
            if (optarg)
                printf(" with arg %s", optarg);
            
            printf("\n");
            
            break;

        case 's':
            if (strlen(optarg) > MAX_NAME_LENGTH)
            {
                printf("ERROR: name = %s exceeds max length of %d\n", optarg, MAX_NAME_LENGTH);
                PrintUsage(1);
            }
            
            strcpy(arguments.captureScript, optarg);
            break;

        case 'r':
            arguments.rate = strtoul(optarg, NULL, 10);
            break;

        case 'I':
            strcpy(arguments.ip_primary, optarg);
            break;

        case 'S':
            strcpy(arguments.ip_secondary, optarg);
            break;

        case 'D':
            arguments.ddr = 1;
            break;

        case 'u':
            arguments.port = CLI_SERVER_PORT_USDP;
            break;

        case 'm':
            arguments.port = CLI_SERVER_PORT_MONITOR;
            break;

        case 'e':
            arguments.port = CLI_SERVER_PORT_MEDIA;
            break;

        case 'U':
            arguments.port = CLI_SERVER_PORT_UNITTEST;
            break;

        case 'P':
            arguments.pdr = 1;
            break;

        case 'L':
            arguments.local = 1;
            break;

        case 'T':
            arguments.top = 1;

            arguments.topRefresh = strtoul(optarg, NULL, 10);

            /* FORCE local execution */
            //arguments.local = 1;
            break;

        case 'R':
            arguments.idleRefresh = strtoul(optarg, NULL, 10);
            break;
            
        case 'C':
            arguments.idleCut = 1;
            break;
            
        case 'F':
            arguments.topFlow = 1;
            break;
            
        case 'a':
            arguments.alignment = strtoul(optarg, NULL, 10);
            break;

        case 'd':
            arguments.debug = strtoul(optarg, NULL, 10);
            break;

        case 'f':
            arguments.showFatal = strtoul(optarg, NULL, 10);
            break;

        case 'h':
        case '?':
            PrintUsage(1);
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
            
        }
    }

    if (optind < argc) 
    {
        printf("command line argument not recognized: ");
        
        while (optind < argc)
            printf("%s ", argv[optind++]);
        
        printf("\n");
    }

    if (arguments.captureScript[0] != '\0')
    {
       sprintf(newCommandLine, "source %s", arguments.captureScript);
       cliEntry(newCommandLine, &printf);
    }

    input[0] = '\0';
    while (1)
    {
        printf ("partytime >> ");

        while((c = getchar()) != EOF)
        {
            input[idx++] = c;

            if (c == '\n')
            {
                break;
            }
        }

        if (idx == 0)
        {
            continue;
        }
        
        /* get rid of newline */
        input[idx-1] = '\0';
         
        cliEntry(input, &printf);
        idx = 0;
        input[0] = '\0';
    }

#endif
}

#endif /* UNIT_TEST */

