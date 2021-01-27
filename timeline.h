#ifndef __TIMELINE_H__
#define __TIMELINE_H__

// #define MAX_TIMELINES (65536)
#define MAX_TIMELINES (8192)

#define MAX_RACES (100)

typedef struct Value_s
{
   char str[MAX_STRING_SIZE];
   int i;
   float f;
} Value_t;

typedef struct PRIdx_s
{
   int idx;
   float improvement;
   int lastFound;
} PRIdx_t;

typedef struct Race_s
{
   char raceName[MAX_STRING_SIZE];
   int place;
   int points;
   int done;
   Value_t time;
} Race_t;

typedef struct Order_s
{
   int pos;
} Order_t;

#if 0
typedef struct CTime_s
{
   int hour;
   int min;
   int sec;
   int msec;
} CTime_t;
#endif

typedef struct TimeLineInfo_s
{
   bool taken;
   int objId;
   char timeLine[MAX_STRING_SIZE];
   time_t timeStamp;

   int groupId;

   int points;
   int totalRaces;
   float av;
   float totalTime;
   float avOrig;
   PRIdx_t prIdx;

   char place[MAX_STRING_SIZE];
   char first[MAX_STRING_SIZE];
   char last[MAX_STRING_SIZE];
   char name[MAX_STRING_SIZE];
   char team[MAX_STRING_SIZE];
   char month[MAX_STRING_SIZE];
   char day[MAX_STRING_SIZE];
   char year[MAX_STRING_SIZE];
   char speed[MAX_STRING_SIZE];
   char watts[MAX_STRING_SIZE];
   char wpkg[MAX_STRING_SIZE];
   char bpm[MAX_STRING_SIZE];
   char vid[MAX_STRING_SIZE];
   char test[MAX_STRING_SIZE];

   Value_t time;
   
   int numRaces;
   Race_t race[MAX_RACES];
   Order_t raceOrder[MAX_RACES];
   
   int fraction;
   int msecs;
   int type;
   int paired;
   int processed;

   Link_t *link;

   Link_t *pair;
   struct TimeLineInfo_s *me;
   // TimeLineInfo_t *me;
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
TimeLineInfo_t *TimeLineInfoInsert(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, LinkList_t *list, int race);
void timeLineInit(CLI_PARSE_INFO *pInfo);
void SideBySideSingle(CLI_PARSE_INFO *pInfo, char *buff);
void ShowTimeLineList(CLI_PARSE_INFO *pInfo, char *msg, int iter);

#endif /* __TIMELINE_H__ */
