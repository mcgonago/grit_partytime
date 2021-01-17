#ifndef __ZCLI_H__
#define __ZCLI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

#ifndef Linux
#define Linux
#endif

#ifndef LINUX
#define LINUX
#endif

#ifndef ERROR
#define ERROR  (-1)
#endif

#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif

#ifndef SUCCESS
#define SUCCESS         0
#endif

#ifndef FAIL
#define FAIL            1
#endif

#define MAX_NAME_LENGTH (120)

typedef struct Arguments_s {
    int rate;
    int alignment;
    int debug;
    int showFatal;
    int ddr;
    int pdr;
    int sync;
    int local;
    int top;
    int topRefresh;
    int idleRefresh;
    int idleCut;
    int topFlow;
    int pdrJSON;
    char filename[MAX_NAME_LENGTH];
    char captureScript[MAX_NAME_LENGTH];
    char executeCommand[MAX_NAME_LENGTH];
    char ip_primary[MAX_NAME_LENGTH];
    char ip_secondary[MAX_NAME_LENGTH];
    int  port;
    int  mode;
} Arguments_t;

extern Arguments_t arguments;

typedef struct TimeTable_s
{
    int on;
    int toleranceFraction;
    int toleranceSeconds;
} TimeTable_t;

extern TimeTable_t timeTable;

/* This returns the offset of a field in a struct */
#define STRUCT_OFFSET(s,f)      ((int)(&(((struct s*)0)->f)))

/* This returns the offset of a field in a typedef'd structure */
#define TSTRUCT_OFFSET(t,f)     ((int)(&(((t*)0)->f)))

/* Returns number of elements in an array */
#define ARRAY_CNT(arr)  (sizeof(arr)/sizeof(arr[0]))

/* Defining bool_t as int is not good if you want to define memory
 * efficient structures, e.g:
 *
 *   struct
 *   {
 *        char   c;
 *        bool_t b : 1;
 *   } s;
 *
 * If bool_t is int, then the two values b can have are '0' and '-1'.
 * Hence, the following code would behave incorrect:
 *
 *   s.b = TRUE;
 *   if (s.b == TRUE)
 *        printf ("This print statement will never be executed!!");
 *
 * Since redefining bool_ at this point in time (9/22/04) would cause
 * major disruption, I (Thomas G. that is) just add a "unsigned bool" type.
 */

#ifndef bool_t
typedef int bool_t;
#endif

typedef unsigned int ubool_t;

#if ! defined(__cplusplus)
#if ! defined(bool)
typedef int bool;
#endif
#endif

#if !defined(INTEL_DATAPLANE) && !defined(PFE_MAX_VPORT)
typedef void (*FUNC_PTR)();
typedef int  (*INT_FUNC_PTR)();
typedef int  (*INT_FUNC_PTR_UINT32)(unsigned long);
#endif

#ifdef __cplusplus
#define EXTERNC  extern "C"
#define Q_HFILE_START \
        extern "C" {
#define Q_HFILE_END \
        }
#else
#define EXTERNC
#define Q_HFILE_START
#define Q_HFILE_END
#endif

#ifdef __i386__
typedef uint32_t PLATFORM_POINTER_T;
#else
typedef uint64_t PLATFORM_POINTER_T;
#endif

/* ---------------------------------------
 * Characteristics of the CLI command line
 * ---------------------------------------
 */

/*
 * From the IANA list:
 *
 * The Dynamic and/or Private Ports are those from 49152 through 65535
 *
 * #               15741-16160  Unassigned
 * #               49152-65535  Unassigned
 */
#define CLI_SERVER_PORT                      15789
#define CLI_REMOTE_SERVER_PORT               63075
#define CLI_SERVER_PORT_UGDB_FP              63076
#define CLI_SERVER_PORT_UGDB_REMOTE_BOARD    63077
#define CLI_SERVER_PORT_UGDB_NATIVE          63079
#define CLI_SERVER_PORT_USBC                 63079
#define CLI_SERVER_PORT_USDP                 63080
#define CLI_SERVER_PORT_MONITOR              63081
#define CLI_SERVER_PORT_MEDIA                63082
#define CLI_SERVER_PORT_UNITTEST             63083


/*-------------------------------------------------------------------*/
/*--------------------------- Parsing Tools -------------------------*/
/*-------------------------------------------------------------------*/

#define CLI_MAX_ARGS_ZCLI 128
typedef int (*PRINT_FP)(const char *format, ...);


typedef struct CLI_PARSE_INFO
{
    int orig_argc;                  /* Original Argument count */
    char *orig_argv[CLI_MAX_ARGS_ZCLI];  /* Original arguments */
    unsigned long level;            /* Current parse level */
    int argc;                       /* Current argc */
    char **argv;                    /* Current argv */
    void *p;                        /* Pointer to private data */
    PRINT_FP print_fp;              /* Function to use a printf */
#ifdef DIAGS
	int status;                     /* Return Status */
#endif
} CLI_PARSE_INFO;

typedef enum CLI_PARSE_OUTCOME
{
    CLI_PARSE_MATCHED,
    CLI_PARSE_AMBIGUOUS,
    CLI_PARSE_NOMATCH,
    CLI_PARSE_NOARG
} CLI_PARSE_OUTCOME;

typedef void (*CLI_PARSE_FP)( CLI_PARSE_INFO *info );

typedef struct CLI_PARSE_CMD
{
    const char *cmd_name;     /* Command or sub command to match on */
    CLI_PARSE_FP fp;          /* Handler to envoke */
    const char *help;         /* Optional help string */
} CLI_PARSE_CMD;

/*************************************************************************/
/* This is for a reenatrant version of getopt() from the openssh code    */
/*************************************************************************/

typedef struct getOptInfo_tag
{
    int  opterr;     /* if error message should be printed */
    int  optind;     /* index into parent argv vector */
    int  optopt;     /* character checked for validity */
    int  optreset;   /* reset getopt */
    char *optarg;    /* argument associated with option */
    char *place;    /* option letter processing */
} getOptInfo_t;

int getopt_r(getOptInfo_t *oi, CLI_PARSE_INFO *info, char *ostr);

/* Message header shared between Microcode Remote GDB client/server */
typedef struct UGdbHeader_s
{
    uint32_t bytesSent;
} UGdbHeader_t;

/* Number of bytes in "BYTES=XXXXXXXX" UGDB header descriptor */
#define UGDB_BYTE_STRING_SIZE (14)

/*-------------------------------------------------------------------*/
/*--------------------------- CLI Prototypes ------------------------*/
/*-------------------------------------------------------------------*/

CLI_PARSE_OUTCOME cliDefaultHandler( CLI_PARSE_INFO *info, const CLI_PARSE_CMD *cmds);
void cliEntry( char *input_line, PRINT_FP print_fp );
unsigned long cliCharToUnsignedLong( char *s );
unsigned long cliCharToLong( char *s );
unsigned long long cliCharToUnsignedLongLong( char *s );

// +++owen - moved all previous trace.h content here
// +++owen - SBC has its own aplib/private/losel/h/trace.h

// #include <time.h>
// #include <stdio.h>
// #include <stdint.h>
// #include "zcli.h"

typedef struct TimeSpec_s
{
    uint32_t tv_sec;
    uint32_t tv_usec;
} TimeSpec_t;

#define MAX_COLUMNS (5)

typedef struct ColumnState_s
{
       uint32_t on;
} ColumnState_t;

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

#define TIME_TO_REAL8(tvp) ((tvp)->tv_sec + (tvp)->tv_usec/1.0E6)

extern int tbInitialized;

#define COLUMN_START_ONE    (0)
#define COLUMN_START_TWO    (53)
#define COLUMN_START_THREE  (85)
#define COLUMN_START_FOUR   (105)
#define COLUMN_START_FIVE   (300)

#define COLUMN_ONE_WIDTH    (COLUMN_START_TWO)
#define COLUMN_TWO_WIDTH    (COLUMN_START_THREE - COLUMN_START_TWO - 1)
#define COLUMN_THREE_WIDTH  (COLUMN_START_FOUR  - COLUMN_START_THREE - 1)
#define COLUMN_FOUR_WIDTH   (COLUMN_START_FIVE  - COLUMN_START_FOUR - 1)
#define SPACES_BETWEEN_COLS (2)

extern int TraceGetTime(TimeSpec_t *tp);
extern void TraceTimeSubtract(TimeSpec_t* start, TimeSpec_t* end, TimeSpec_t *rslt);
extern void TraceTimeAdd(TimeSpec_t *start, TimeSpec_t *delta);
extern void OSSleep(int milliseconds);
extern int TraceBufferGetTime(TimeSpec_t *tp);
extern void TraceBufferTimeSubtract(TimeSpec_t* start, TimeSpec_t* end, TimeSpec_t *rslt);
extern void TraceBufferTimeAdd(TimeSpec_t *start, TimeSpec_t *delta);
extern int TraceColumnOn(int columnNum);
extern void TBInit(PRINT_FP print_fp);

#ifdef __cplusplus
}
#endif

#endif
