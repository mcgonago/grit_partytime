#ifndef _ZCOLOR_H__
#define _ZCOLOR_H__

#include "zcli_server.h"
#include "zcli.h"
//#include "xstatcommands.h"
#include "commas.h"
//#include "xstatcustom.h"
#include "link.h"
#include "linklist.h"
//#include "xstatbuffer.h"
//#include "rcli.h"
#include "pthread.h"
//#include "timespec.h"
//#include "trace.h"

#define BLUE          "\033[0;34m "
#define RED           "\033[0;31m "
#define LIGHT_RED     "\033[1;31m "
#define WHITE         "\033[1;37m "
#define NOCOLOR       "\033[0m "
#define GREEN         "\033[22;32m "
#define YELLOW        "\033[01;33m "
#define BROWN         "\033[22;33m "
#define CYAN          "\033[22;36m "
#define LIGHT_GREEN   "\033[01;32m "
#define LIGHT_BLUE    "\033[01;34m "
#define GRAY          "\033[22;37m "
#define DARK_GRAY     "\033[01;30m "
#define BLACK         "\033[22;30m "
#define REVERSE_VIDEO "\033[7m "
#define SPACES        " "

void XStatColorCmd(CLI_PARSE_INFO *pInfo);

#define SKIP_EARLY    (1)
#define SKIP_LATE     (2)
#define SKIP_NO       (3)

#define SHOW_SAR_ON   (1)
#define SHOW_SAR_OFF  (2)

#define CPU_THRESHOLD (100.0)

#define SAR_QUEUE_LENGTH (5)

#define PID_CHECK_ALL  (1)
#define PID_CHECK_IDLE (2)
#define PID_CHECK_NONE (3)

#define PIPE_TYPE_SSM       (1)
#define PIPE_TYPE_SSM_FILE  (2)

#define TOTAL_COLS           (2)    /* configurable */
#define TOTAL_ROWS           (50)   /* configurable */

#define ALL_IDLE_COUNT_MAX   (2)

#define USEC_IN_SEC                         (1000000)
#define MAX_TIMESTAMP_LEN                   (128)

#define SPLIT_TYPE_HOUR      (1)
#define SPLIT_TYPE_RANGE     (2)  

#define SPLIT_HOUR_TRIGGER   (1)
#define SPLIT_RANGE_TRIGGER  (2)
#define SPLIT_RANGE_DONE     (3)
#define SPLIT_NO_TRIGGER     (4)

#define FIRST_COLUMN_WIDTH_COLOR   (180)   /* configurable */
#define NTH_COLUMN_WIDTH_COLOR     (100)    /* configurable */

#define FIRST_COLUMN_WIDTH_BRACKET (100)   /* configurable */
#define NTH_COLUMN_WIDTH_BRACKET   (43)    /* configurable */

#define OVERFLOW_ROWS            (3)
#define TOO_CLOSE_FOR_COLUMN     (100)
#define TOO_CLOSE_FOR_ROW        (2)

#define FORMAT_SPECIFIER_LEN (40)

#define DEFAULT_COL_START    (0x1000000)

#define INSERT_INDIVIDUAL    (1)
#define INSERT_TOTALS        (2)

#define COUNTER_CLEAR_NO     (0)
#define COUNTER_CLEAR_YES    (1)

/* Maximum number of "lead in" PID lines before first legit PID statistic collection/output */
#define LEAD_IN_MAX          (10)

#define GET_ABSOLUTE         (1)
#define GET_RELATIVE         (2)
#define DISPLAY_COMBO        (3)
#define DISPLAY_SINGLE       (4)

#define FILTER_SHOW_A    (0x00000001)
#define FILTER_SHOW_B    (0x00000002)
#define FILTER_SHOW_C    (0x00000004)
#define FILTER_SHOW_D    (0x00000008)
#define FILTER_SHOW_USER (0x00000010)
#define FILTER_SHOW_ALL  (0x0000001F)

#define FILTER_SELECT_A    (0x00000001)
#define FILTER_SELECT_B    (0x00000002)
#define FILTER_SELECT_C    (0x00000004)
#define FILTER_SELECT_D    (0x00000008)
#define FILTER_SELECT_USER (0x00000010)
#define FILTER_SELECT_TIME (0x00000020)
#define FILTER_SELECT_ALL  (0x0000001F)
#define FILTER_SELECT_NONE (0x00000000)

#define ORDER_SEEN (1)
#define ORDER_LAST (2)
#define ORDER_MIN  (3)
#define ORDER_MAX  (4)
#define ORDER_AV   (5)
#define ORDER_DATE (6)
#define ORDER_NAME (7)
#define ORDER_CPU  (8)

#define NAME_LENGTH_MAX (128)

#define PID_FILE_DISABLED (NULL)

#define MONITOR_STRING_SIZE      (1024)

#define MAX_CORES                (12)
#define MAX_PFE_PORTS            (12)
#define MAX_PROC_NET_DEV_PORTS   (40)

#define XSTAT_MAX_NUM_COUNTERS   (220)
#define STAT_BUFFER_SIZE_MAX     (2 * XSTAT_MAX_NUM_COUNTERS)


typedef enum CounterCustomId_e
{
    COUNTER_CUSTOM_ID_HIDE_COLUMN = 0,
    COUNTER_CUSTOM_ID_HIDE_COLUMN_SPLIT,
    COUNTER_CUSTOM_ID_HIDE_COLUMN_EVERY
} CounterCustomId_t;


typedef struct CounterCustom_s
{
    uint32_t hideColumn;
    uint32_t hideColumnSplit;
    uint32_t hideColumnEvery;
} CounterCustom_t;

typedef struct XStatBuffer_s
{
    uint32_t idx;
    uint32_t lastIndexArray[STAT_BUFFER_SIZE_MAX + 1];
    char xstatBuffer[STAT_BUFFER_SIZE_MAX + 1][MONITOR_STRING_SIZE + 1];
} XStatBuffer_t;

typedef struct XStatCounterInfo_s
{
    uint32_t maxChars;
    uint64_t counter[XSTAT_MAX_NUM_COUNTERS];
    double rate[XSTAT_MAX_NUM_COUNTERS];
} XStatCounterInfo_t;

typedef struct XStatInfo_s
{
    uint16_t taken;
    uint32_t id;
    TimeSpec_t previousTime;

    XStatCounterInfo_t ssm[MAX_PROC_NET_DEV_PORTS];
    XStatCounterInfo_t ssmTotals;

    XStatBuffer_t xstatBufferProcNetDev;

} XStatInfo_t;

extern XStatInfo_t xstatInfoCleared;
extern uint32_t mapStart;
extern uint32_t mapWidth;
extern uint32_t mapTotal;

#define LARGE_WINDOW_NCOLS  (4096)
#define LARGE_WINDOW_NROWS  (4096)

// In terms of total rows of total (top) + (idle) + (max user) + (max idle) + (max resched) + (sar CPU)
#define TOTAL_ROW_MAX       (LARGE_WINDOW_NROWS * 6)

#define PID_NAME_LENGTH_DEF  (21)
#define PID_NAME_LENGTH_MAX  (80)
#define CHARS_IN_COMMAND     (5)    /* leave space for color formatting */


#define TIME_STAMP_LENGTH    (9)

#define PID_TYPE_PROCESS     (1)
#define PID_TYPE_THREAD      (2)

#define DEFAULT_CDR_STRING_LENGTH (342)

#define BIG_DATA_LEN (4095)

typedef struct ProcNetDevSet_s
{
    int debugMode;
    int unitTest;
} ProcNetDevSet_t;

typedef struct NetDescriptor_s
{
    union
    {
        FILE * fpipe;
        int fd;
    } u;
    int type;
} NetDescriptor_t;

typedef struct fmt_color_s
{
    uint32_t changed;
} fmt_color_t;
     
typedef struct fmt_uint32_s
{
    uint32_t val;
    fmt_color_t color;
} fmt_uint32_t;

typedef struct fmt_uint64_s
{
    uint64_t val;
    fmt_color_t color;
} fmt_uint64_t;

typedef struct fmt_double_s
{
    double val;
    fmt_color_t color;
} fmt_double_t;

//struct PidProcess_t;

#define MAX_NAME (128)
#define TOP_NUM  (5)

typedef struct PidProcess_s
{
    Link_t *link;
    uint32_t objId;

    int pidType;
    int showedUpNow;
    int showedUpPrev;
    int adjustUptime;
    int idleCount;
    int uptimeCount;
    int disabled;
    int newEntry;
    uint32_t tid;
    fmt_double_t user;
    fmt_double_t system;
    fmt_double_t guest;
    fmt_double_t cpuPercent;
    fmt_double_t cpuPercentMax;
    fmt_double_t cpuPercentAv;
    fmt_double_t cpuPercentTotal;
    fmt_uint32_t cpuNumber;
    uint32_t rawTime;
    char timeStamp[TIME_STAMP_LENGTH];
    char name[PID_NAME_LENGTH_MAX];
    uint32_t totalSeenIdle;
    uint32_t totalTicksIdle;
    uint32_t totalSeenUptime;
    uint32_t totalTicksUptime;
    uint32_t totalTicksLastIdle;
    uint32_t totalTicksLastUptime;
    uint32_t uptimeMin;
    uint32_t uptimeMax;
    uint32_t uptimeAverage;
    uint32_t uptimeTotal;
    uint32_t idleMin;
    uint32_t idleMax;
    uint32_t idleAverage;
    uint32_t idleTotal;
    uint32_t crazy;
    uint32_t startTick;
    uint32_t endTick;

    double av;
    fmt_double_t *cpu[32];
    
    fmt_double_t cpu_00;
    fmt_double_t cpu_01;
    fmt_double_t cpu_02;
    fmt_double_t cpu_03;
    fmt_double_t cpu_04;
    fmt_double_t cpu_05;
    fmt_double_t cpu_06;
    fmt_double_t cpu_07;
    fmt_double_t cpu_08;
    fmt_double_t cpu_09;
    fmt_double_t cpu_10;
    fmt_double_t cpu_11;
    fmt_double_t cpu_12;
    fmt_double_t cpu_13;
    fmt_double_t cpu_14;
    fmt_double_t cpu_15;
    fmt_double_t cpu_16;
    fmt_double_t cpu_17;
    fmt_double_t cpu_18;
    fmt_double_t cpu_19;
    fmt_double_t cpu_20;
    fmt_double_t cpu_21;
    fmt_double_t cpu_22;
    fmt_double_t cpu_23;
    fmt_double_t cpu_24;
    fmt_double_t cpu_25;
    fmt_double_t cpu_26;
    fmt_double_t cpu_27;
    fmt_double_t cpu_28;
    fmt_double_t cpu_29;
    fmt_double_t cpu_30;
    fmt_double_t cpu_31;

    int topNum;
    char topThree[TOP_NUM][PID_NAME_LENGTH_MAX];
    
} PidProcess_t;

typedef struct PidStatConfig_s
{
    uint32_t numberOfRows;
    uint32_t numberOfColumns;
    uint32_t displayMode;
    uint32_t filterSelect;
    uint32_t idleMax;
    uint32_t silent;
    uint32_t oneHitTracking;
    uint32_t filterMatchYesSkip;
    uint32_t filterMatchNoSkip;
    uint32_t order;
    uint32_t sar;
    uint32_t nameLenMax;
    uint32_t maxRows;
    double cpuThreshold;
    uint32_t diffColor;
    uint32_t top;
    uint32_t split;
    uint32_t sarQueue;
    uint32_t pfeEngine;
    char fname[NAME_LENGTH_MAX];
    char pidstatCommand[NAME_LENGTH_MAX];
    
} PidStatConfig_t;

#define MAX_CORES               (12)
#define MAX_PFE_PORTS           (12)  
#define MAX_PROC_NET_DEV_PORTS  (40)
#define MAX_VQM_REDIR           (40)

#define NULL_COLUMN                   (1)
#define NULL_ROW                      (1)
#define XSTAT_MAX_COLS                (MAX_PROC_NET_DEV_PORTS + NULL_COLUMN)
#define XSTAT_MAX_ROWS                (220 + NULL_ROW)

#define COLUMN_OFF (0)
#define COLUMN_ON  (1)

#define CUSTOM_NAME_MAX (64)

#define XSTAT_TYPE_UINT64_COMMAS          (1)
#define XSTAT_TYPE_UINT32_COMMAS          (2)
#define XSTAT_TYPE_UINT64_HEX             (3)
#define XSTAT_TYPE_UINT32_HEX             (4)
#define XSTAT_TYPE_IP                     (5)
#define XSTAT_TYPE_MAC                    (6)
#define XSTAT_TYPE_PERCENT                (7)
#define XSTAT_TYPE_READ_CSR               (XSTAT_TYPE_UINT64_COMMAS)
#define XSTAT_TYPE_UINT64_COMMAS_NO_CLEAR (8)
#define XSTAT_TYPE_DASH                   (9)
#define XSTAT_TYPE_UINT64_COMMAS_NO_RATE  (10)
#define XSTAT_TYPE_CPU                    (11)
                                          
                                          
#define BLOCK_TYPE_NEW                (1)
#define BLOCK_TYPE_PAGE               (2)
#define BLOCK_TYPE_CONTINUE           (3)
#define BLOCK_TYPE_FILLER             (4)


typedef struct XStatCustomHeader_s
{
    uint32_t headerId;
    char title[CUSTOM_NAME_MAX];
    uint32_t numRegs;
    uint32_t numCols;
    uint32_t startCol;
    uint32_t endCol;
    char nameCol[XSTAT_MAX_COLS][CUSTOM_NAME_MAX];
} XStatCustomHeader_t;
    
typedef struct XStatCustomEntry_s
{               
    char rowName[CUSTOM_NAME_MAX];
    uint32_t entryId;
    uint32_t rowState;
    uint32_t colState;
    uint32_t entryType;
} XStatCustomEntry_t;

typedef struct XStatCustom_s
{
    XStatCustomHeader_t header;
    XStatCustomEntry_t entry[XSTAT_MAX_COLS][XSTAT_MAX_ROWS+1];
} XStatCustom_t;


void CounterCustomInit(void);
int CounterCustomGet(CounterCustomId_t id);
int CounterCustomSet(CounterCustomId_t id, int val);

void AllPidstat(CLI_PARSE_INFO *pInfo, uint32_t pidCheck, uint32_t silent, uint32_t topOn, uint32_t splitOn, uint32_t sarQueue, uint32_t pfeEngine);

void ColumnInsertCounterCustom(CLI_PARSE_INFO *pInfo, char *title, uint32_t numRegs, uint32_t startCol, uint32_t endCol, uint32_t blockType, XStatInfo_t *xstatInfo, XStatInfo_t *prev,
                               XStatCounterInfo_t *currInfo, XStatCounterInfo_t *prevInfo, XStatBuffer_t *xstatBuffer, XStatCustom_t *xstatCustom, uint32_t insertType);

void ColumnInsertRateCustom(CLI_PARSE_INFO *pInfo, char *title, uint32_t numRegs, uint32_t startCol, uint32_t endCol, uint32_t blockType, XStatInfo_t *xstatInfo, XStatInfo_t *prev,
                            XStatCounterInfo_t *currInfo, XStatCounterInfo_t *prevInfo, XStatBuffer_t *xstatBuffer, XStatCustom_t *xstatCustom, uint32_t insertType, uint32_t bufferOffset);

uint32_t CustomStatsCorrelate(CLI_PARSE_INFO *pInfo, uint64_t *counter, uint64_t *clearCounter, uint32_t entryType, uint32_t mode);

uint32_t PidDisplayColGet(CLI_PARSE_INFO *info);
uint32_t PidDisplayRowGet(CLI_PARSE_INFO *info);
void PidStatReset(CLI_PARSE_INFO *info);
uint32_t PidFilterGet(CLI_PARSE_INFO *info);
uint32_t PidModeGet(CLI_PARSE_INFO *info);
uint32_t PidIdleMaxGet(CLI_PARSE_INFO *info);
uint32_t PidSilentGet(CLI_PARSE_INFO *info);

int Skip(CLI_PARSE_INFO *info, char *line);
int SkipTime(CLI_PARSE_INFO *info, PidProcess_t *pidProcess);

void PidstatPurge(CLI_PARSE_INFO *info);
void OneHitUpdateUptime(CLI_PARSE_INFO *pInfo, PidProcess_t *pidProcess);
void OneHitUpdateIdle(CLI_PARSE_INFO *pInfo, PidProcess_t *pidProcess);
void OneHitUpdateIdle2(CLI_PARSE_INFO *pInfo, PidProcess_t *pidProcess, uint32_t totalTicks);
void AllPidstatOneHit(CLI_PARSE_INFO *pInfo, uint32_t mode, uint32_t silent);

void MemCpyPid(PidProcess_t *pidTo, PidProcess_t *pidFrom);

uint32_t XstatShowSarGet(CLI_PARSE_INFO *info);
void SarDisplay(CLI_PARSE_INFO *pInfo);
void SarDisplaySingle(CLI_PARSE_INFO *pInfo, uint32_t sarQueue, uint32_t pfeEngine);
uint32_t SarQueueGet (CLI_PARSE_INFO *info);
uint32_t NameLenMaxGet(CLI_PARSE_INFO *info);
void PidStatInit(CLI_PARSE_INFO *pInfo);
uint32_t MaxRowsGet(CLI_PARSE_INFO *info);
double CpuThresholdGet(CLI_PARSE_INFO *info);
uint32_t diffColorGet(CLI_PARSE_INFO *info);
uint32_t TopGet (CLI_PARSE_INFO *info);
uint32_t SplitGet (CLI_PARSE_INFO *info);
uint32_t PfeEngineGet (CLI_PARSE_INFO *info);

void PidstatCommandGet(char *buffer);

extern void CollectPidstat(CLI_PARSE_INFO *pInfo, uint32_t all, char *fname);
extern PidProcess_t *PidProcessNew(CLI_PARSE_INFO *pInfo);
extern void PidProcessFree(CLI_PARSE_INFO *pInfo, LinkList_t *list, PidProcess_t *pidProcess);
extern void PidStatInitCmd(CLI_PARSE_INFO *pInfo);

/* defines */

#define CMD_BUF_LEN (4096)

#define UP_CMD_COUNT        (1)
#define UP_CMD_INDEX        (2)
#define UP_CMD_TOP          (3)
#define UP_CMD_BOTTOM       (4) 
#define UP_CMD_FIND_STR     (5)
#define UP_CMD_FIND_LABEL   (6)

#define DOWN_CMD_COUNT      (7)
#define DOWN_CMD_INDEX      (8)
#define DOWN_CMD_TOP        (9)
#define DOWN_CMD_BOTTOM     (10)
#define DOWN_CMD_FIND_STR   (11)
#define DOWN_CMD_FIND_LABEL (12)

#define FIRST_TIME_YES           (1)
#define FIRST_TIME_NO            (2)

#define COUNTER_ID_SSM           (1)

#define XSTAT_CPU_MODE_HIGHEST   (1)
#define XSTAT_CPU_MODE_NORMAL    (2)

#define COLUMN_SET_MODE_ALL      (1)
#define COLUMN_SET_MODE_EVERY_X  (2)
#define COLUMN_SET_MODE_SELECT   (3)
#define COLUMN_SET_MODE_RANGE    (4)

#define XSTAT_FILTER_SSM         (0x00000001)
                                      
#define XSTAT_FILTER_ALL         (XSTAT_FILTER_SSM)

#define MAX_COMMAND_LENGTH (1024)
#define MAX_LINE_LENGTH    (1024)

typedef struct XStatCmdControl_s
{
    uint32_t xstatMask;
    uint32_t numberOfXStatInfoBlocks;
    uint32_t xstatInfoLength;
    uint32_t truncateLength;
    
    pthread_mutex_t lock;
} XStatCmdControl_t;

#define COUNTER_MODE_OFF (0)
#define COUNTER_MODE_ON (1)

typedef enum XStatCustomProcNetDevId_e
{
    CUSTOM_SSM = 0,
    CUSTOM_SSM_TOTALS,
} XStatCustomProcNetDevId_t;
                      
typedef enum XStatCustomSsmEntryId_e
{                     
    SSM_IOCTL_CAVIUM = 0,
    SSM_IOCTL_TCP,
    SSM_PACKET_TCP,
    SSM_AES_CRYPTO_ENCRYPT,
    SSM_AES_CRYPTO_DECRYPT,
    SSM_CRYPTO_DEV_ID_0,
    SSM_CRYPTO_DEV_ID_1,
    SSM_CRYPTO_DEV_ID_2,
    SSM_CRYPTO_DEV_ID_3,
    SSM_ENQUEUE_RATE_ACTIVE_PERIOD,
    SSM_ENQUEUE_RATE_HIGH_PERIOD,  
    SSM_ENQUEUE_RATE_TOTAL_PERIOD, 
    SSM_ENQUEUE_RATE_TOTAL_LIFETIME,
    SSM_ENQUEUE_RATE_PERMAX_LIFETIME,
    SSM_ENQUEUE_RATE_HIGH_LIFETIME,   
    SSM_DEQUEUE_RATE_ACTIVE_PERIOD,   
    SSM_DEQUEUE_RATE_HIGH_PERIOD,  
    SSM_DEQUEUE_RATE_TOTAL_PERIOD, 
    SSM_DEQUEUE_RATE_TOTAL_LIFETIME,
    SSM_DEQUEUE_RATE_PERMAX_LIFETIME,
    SSM_DEQUEUE_RATE_HIGH_LIFETIME,   
    SSM_ENQUEUE_ACTIVE_PERIOD,        
    SSM_ENQUEUE_HIGH_PERIOD,  
    SSM_ENQUEUE_TOTAL_PERIOD,
    SSM_ENQUEUE_TOTAL_LIFETIME,
    SSM_ENQUEUE_PERMAX_LIFETIME,
    SSM_ENQUEUE_HIGH_LIFETIME,   
    SSM_ENQUEUE_DROP_ELIGIBLE_ACTIVE_PERIOD,
    SSM_ENQUEUE_DROP_ELIGIBLE_HIGH_PERIOD,   
    SSM_ENQUEUE_DROP_ELIGIBLE_TOTAL_PERIOD,   
    SSM_ENQUEUE_DROP_ELIGIBLE_TOTAL_LIFETIME,  
    SSM_ENQUEUE_DROP_ELIGIBLE_PERMAX_LIFETIME, 
    SSM_ENQUEUE_DROP_ELIGIBLE_HIGH_LIFETIME,   
    SSM_ENQUEUE_DROP_INELIGIBLE_ACTIVE_PERIOD, 
    SSM_ENQUEUE_DROP_INELIGIBLE_HIGH_PERIOD,   
    SSM_ENQUEUE_DROP_INELIGIBLE_TOTAL_PERIOD,
    SSM_ENQUEUE_DROP_INELIGIBLE_TOTAL_LIFETIME,
    SSM_ENQUEUE_DROP_INELIGIBLE_PERMAX_LIFETIME,
    SSM_ENQUEUE_DROP_INELIGIBLE_HIGH_LIFETIME,  
    SSM_ENQUEUE_DROPPED_ACTIVE_PERIOD,          
    SSM_ENQUEUE_DROPPED_HIGH_PERIOD,  
    SSM_ENQUEUE_DROPPED_TOTAL_PERIOD,
    SSM_ENQUEUE_DROPPED_TOTAL_LIFETIME,
    SSM_ENQUEUE_DROPPED_PERMAX_LIFETIME,
    SSM_ENQUEUE_DROPPED_HIGH_LIFETIME,  
    SSM_DEQUEUE_ACTIVE_PERIOD,          
    SSM_DEQUEUE_HIGH_PERIOD,            
    SSM_DEQUEUE_TOTAL_PERIOD,           
    SSM_DEQUEUE_TOTAL_LIFETIME,         
    SSM_DEQUEUE_PERMAX_LIFETIME,
    SSM_DEQUEUE_HIGH_LIFETIME,   
    SSM_DEQUEUE_DROP_ELIGIBLE_ACTIVE_PERIOD,
    SSM_DEQUEUE_DROP_ELIGIBLE_HIGH_PERIOD,  
    SSM_DEQUEUE_DROP_ELIGIBLE_TOTAL_PERIOD, 
    SSM_DEQUEUE_DROP_ELIGIBLE_TOTAL_LIFETIME,
    SSM_DEQUEUE_DROP_ELIGIBLE_PERMAX_LIFETIME,
    SSM_DEQUEUE_DROP_ELIGIBLE_HIGH_LIFETIME,   
    SSM_DEQUEUE_DROP_INELIGIBLE_ACTIVE_PERIOD, 
    SSM_DEQUEUE_DROP_INELIGIBLE_HIGH_PERIOD,   
    SSM_DEQUEUE_DROP_INELIGIBLE_TOTAL_PERIOD,  
    SSM_DEQUEUE_DROP_INELIGIBLE_TOTAL_LIFETIME, 
    SSM_DEQUEUE_DROP_INELIGIBLE_PERMAX_LIFETIME,
    SSM_DEQUEUE_DROP_INELIGIBLE_HIGH_LIFETIME,  
    SSM_DEQUEUE_DROPPED_ACTIVE_PERIOD,          
    SSM_DEQUEUE_DROPPED_HIGH_PERIOD,  
    SSM_DEQUEUE_DROPPED_TOTAL_PERIOD, 
    SSM_DEQUEUE_DROPPED_TOTAL_LIFETIME,
    SSM_DEQUEUE_DROPPED_PERMAX_LIFETIME,
    SSM_DEQUEUE_DROPPED_HIGH_LIFETIME,  
    SSM_PER_QUEUE_DEPTH_EVENTS,         
    SSM_PER_QUEUE_DEPTH_AVERAGE,    
    SSM_PER_QUEUE_DEPTH_MAXIMUM,    
    SSM_PER_LATENCY_PENDING_EVENTS, 
    SSM_PER_LATENCY_PENDING_AVERAGE,
    SSM_PER_LATENCY_PENDING_MAXIMUM,                 
    SSM_PER_LATENCY_PROCESSING_EVENTS, 
    SSM_PER_LATENCY_PROCESSING_AVERAGE,
    SSM_PER_LATENCY_PROCESSING_MAXIMUM,                 
    SSM_CPU_USAGE,                                   
    SSM_CPU_OVERLOAD,               
    SSM_MAX          
} XStatCustomSsmEntryId_t;

extern uint32_t showZero;

#ifdef __cplusplus
extern "C" {
#endif

extern void SsmCounterIncrement(uint32_t dsp_num, uint32_t counterId);
extern void SsmCounterDecrement(uint32_t dsp_num, uint32_t counterId);
extern void SsmCounterSet(uint32_t dsp_num, uint32_t counterId, uint64_t val);

XStatInfo_t *XStatShowCmdCounters(CLI_PARSE_INFO *pInfo, uint32_t counterId, uint32_t combo);
void XStatShowCombo(CLI_PARSE_INFO *pInfo, uint32_t counterId);
void XStatShowCmdRates(CLI_PARSE_INFO *pInfo, uint32_t counterId);
void XStatShowVportCmd( CLI_PARSE_INFO *pInfo);

extern uint32_t ProcNetDevGet(CLI_PARSE_INFO *pInfo, XStatCustomProcNetDevId_t internalId, XStatCustom_t **xstatCustom);
extern uint32_t ProcNetDevColumnHide(XStatCustomProcNetDevId_t tableId, uint32_t state);
extern uint32_t ProcNetDevColumnHeaderUpdate(XStatCustomProcNetDevId_t tableId, uint32_t col, char *name);
extern uint32_t ProcNetDevColumnSet(XStatCustomProcNetDevId_t tableId, uint32_t columnStart, uint32_t columnEnd, uint32_t state, uint32_t mode);

extern void ProcNetDevInit(void);
extern uint64_t ProcNetDevIncrement(uint32_t dsp_num, uint32_t counterId);
extern uint64_t ProcNetDevDecrement(uint32_t dsp_num, uint32_t counterId);
extern uint64_t ProcNetDevSet(XStatCustomProcNetDevId_t devId, uint32_t dsp_num, uint32_t counterId, uint64_t val);
extern uint64_t ProcNetDevAdd(XStatCustomProcNetDevId_t devId, uint32_t dsp_num, uint32_t counterId, uint64_t val);

extern uint32_t ProcNetDevStatsGet(XStatCustomProcNetDevId_t devId, uint32_t entryId, uint32_t columnNum, uint64_t *regData64);
extern uint32_t ProcNetDevStatsGetTotals(XStatCustomProcNetDevId_t devId, uint32_t entryId, uint32_t columnNum, uint64_t *regData64);

extern void XStatShowCounterCmd( CLI_PARSE_INFO *pInfo);
extern void XStatShowRateCmd( CLI_PARSE_INFO *pInfo);
extern void XStatShowComboCmd(CLI_PARSE_INFO *pInfo);
extern void XStatShowPacketCombo( CLI_PARSE_INFO *pInfo);

#ifdef __cplusplus
}
#endif

#endif
