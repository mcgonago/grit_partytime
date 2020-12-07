#ifndef __TIMELINE_H__
#define __TIMELINE_H__

#define MAX_TIMELINES (65536)

typedef struct TimeLineInfo_s
{
   bool taken;
   char timeLine[MAX_STRING_SIZE];
   time_t timeStamp;
   char place[MAX_STRING_SIZE];
   char name[MAX_STRING_SIZE];
   char time[MAX_STRING_SIZE];
   char wpkg[MAX_STRING_SIZE];
   char watts[MAX_STRING_SIZE];

   int fraction;
   int msecs;
   int type;
   int paired;
   int processed;
  
   Link_t *pair;
} TimeLineInfo_t;

typedef struct TimeLineEnv_s
{
    uint32_t numberOfTimeLineInfoBlocks;
    uint32_t timeLineInfoLength;
} TimeLineEnv_t;

TimeLineEnv_t timeLineEnv;
       
typedef struct TimeLinePool_s
{
    TimeLineInfo_t timeLineInfo;
} TimeLinePool_t;

extern LinkList_t *listTimeLine;

TimeLineInfo_t *TimeLineCreate(CLI_PARSE_INFO *pInfo);
void TimeLineRemove(TimeLineInfo_t *timeLineInfo);
TimeLineInfo_t *TimeLineInfoNew(CLI_PARSE_INFO *pInfo, int type);
TimeLineInfo_t *TimeLineInfoInsert(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo);
void timeLineInit(CLI_PARSE_INFO *pInfo);
void SideBySideSingle(CLI_PARSE_INFO *pInfo, char *buff);
void ShowTimeLineList(CLI_PARSE_INFO *pInfo, char *msg, int iter);

#endif /* __TIMELINE_H__ */
