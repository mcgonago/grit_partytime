#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"
#include <time.h>
#include <sys/time.h>
#include "pthread.h"
#include "string.h"
#include "fcntl.h"

#include "link.h"
#include "linklist.h"

#include "zcli.h"
#include "zcolor.h"
#include "tracebuffer.h"
#include "timeline.h"
#include "columnprintf.h"
#include "partycontrol.h"

//#define CONSOLE_OUTPUT (1)

static volatile int gdbStop = 1;

#define PRINT_MODE_DISPLAY (1)

#define HEADER_INDEX_FIRST_LINE (0)
#define HEADER_INDEX_DELIMITER  (1)
   
#define LOWEST_TIME (10000.0)

#define TIME_ORDER_POINTS     (1)
#define TIME_ORDER_TEMPOISH   (2)
#define TIME_ORDER_TIME       (3)
#define TIME_ORDER_RICHMOND   (4)

#define PARTY_KOM_SPRINTS     (0x00000001)
#define PARTY_FOLLOWING       (0x00000002)
#define PARTY_FOLLOWING_2     (0x00000004)
#define PARTY_RESULTS         (0x00000008)
#define PARTY_RESULTS_2       (0x00000010)
#define PARTY_KOM_SPRINTS_RAW (0x00000020)
#define PARTY_FOLLOWING_RAW   (0x00000040)
#define PARTY_FOLLOWING_2_RAW (0x00000080)
#define PARTY_RESULTS_RAW     (0x00000100)
#define PARTY_RESULTS_2_RAW   (0x00000200)

/* +++owen - hardcoding to assuming ONLY (4) races taking part in points series */
static int maxCount = 0;
static int numRacesG = 0;
static int raceIdxG = -1;
static TimeLineInfo_t *timeLineInfoG = NULL;

LinkList_t *listTimeLine = NULL;
LinkList_t *listTimeOrder = NULL;

TimeLinePool_t *timeLinePool;

//#define DEBUG_LEVEL_1 (1)

#define RACE_NO  (1)
#define RACE_YES (2)

TimeTable_t timeTable = {1, 0, -1};

static uint32_t firstTime = 1;

#define MESSAGE_CUTOFF (70)

#define MAX_ATHLETES (100)
typedef struct Athlete_s
{
    uint32_t enabled;
    char name[MAX_NAME];
} Athlete_t;

Athlete_t athleteA[MAX_ATHLETES] = {
    {0, "ATHLETE_END"},
};

Athlete_t athleteB[MAX_ATHLETES] = {
    {0, "ATHLETE_END"},
};

Athlete_t athleteAll[MAX_ATHLETES] = {
    {0, "ATHLETE_END"},
};


Athlete_t athleteGRIT[MAX_ATHLETES] = {
   {1, "Christian Kaldbar"},
   {1, "Gabriel Mathisen"},
   {1, "Enda Bagnall"},
   {1, "Rob Fullerton"},
   {1, "Steve Tappan"},
   {1, "R. Elvira"},
   {1, "Doug Johnson"},
   {1, "Luke Elton"},
   {1, "MMD Kase Juku"},
   {1, "Dominik Lugmair"},
   {1, "Iijima Daisuke"},
   {1, "JESUS PRIETO"},
   {1, "tarou nagae"},
   {1, "Ana Santos"},
   {1, "Miquel Morales"},
   {1, "Anders Danielsen"},
   {1, "John Jeffries"},
   {1, "Lara Middleton"},
   {1, "Owen McGonagle"},
   {1, "M_arc Lebel"},
   {1, "Lisandro Quebrada"},
   {0, "ATHLETE_END"},
};


   
Athlete_t athleteGRIT1[MAX_ATHLETES] = {
//   {1, "Sepp Kuss"},                  /* 18 */
//   {1, "James Piccoli"},              /* 19 */
   {1, "Lance Anderson"},             /* 1  */
   {1, "Gabriel Mathisen"},           /* 2  */
   {1, "Steve Tappan"},               /* 3  */
   {1, "Derek Sawyer"},               /* 4  */
   {1, "Tak Ina"},                    /* 5  */
   {1, "tak ina"},                    /* 5  */
   {1, "Seth G"},                     /* 6  */
   {1, "Rob Fullerton"},              /* 7  */
   {1, "Owen McGonagle"},             /* 8  */
   {1, "Luke Elton"},                 /* 9  */
   {1, "Paul Hutchins"},              /* 10 */
   {1, "Steve Peplinski"},            /* 11 */
   {1, "Maximilian Weniger"},         /* 12 */
   {1, "John Jeffries"},              /* 13 */
   {1, "Alan Brannan"},               /* 14 */
   {1, "Shaun Corbin"},               /* 15 */
   {1, "Ben Sisson"},                 /* 16 */
   {1, "Andrew Cohen"},               /* 17 */
   {1, "Christian Kaldbar"},          /* 18 */
   {0, "ATHLETE_END"}
};

Athlete_t athleteGRIT_try1[MAX_ATHLETES] = {
   {1, "Steve Tappan"},
   {1, "Stuart Glover"},
   {1, "Rob Fullerton"},
   {1, "Gabriel Mathisen"},
   {1, "Leon Pearson"},
   {1, "Owen McGonagle"},
   {1, "Luke Elton"},
   {1, "Daniel Preece"},
   {1, "Alan Brannan"},
   {1, "Rob Dapice"},
   {1, "Philip Hurst"},
   {1, "Lara Middleton"},
   {1, "Rob McKechney"},
   {1, "stijn ver"},
   {1, "Szuiram Tri"},
   {0, "ATHLETE_END"},
};
   
Athlete_t athleteGRIT2[MAX_ATHLETES] = {
   {1, "Matthias Mueller"},
   {1, "Joel Gibbel"},
   {1, "Philip Baronius"},
   {1, "Alan Brannan"},
   {1, "Rob Fullerton"},
   {1, "Christian Kaldbar"},
   {1, "Gabriel Mathisen"},
   {1, "John Jeffries"},
   {1, "Owen McGonagle"},
   {1, "Steve Tappan"},
   {1, "Tak Ina"},
   {1, "Ryuta Ito"},
   {1, "Claire Jessop"},
   {1, "Kelsey Smith"},
   {1, "Alexander Schicho"},
   {1, "Daniel Florez"},
   {1, "Nobbi"},
   {1, "A NakaoJPN"},
   {1, "Brian Deason7654"},
   {1, "Eric GONDERINGER"},
   {1, "Geoff Gibson"},
   {1, "Hotpot Neo"},
   {1, "James Bolze"},
   {1, "Kris Kenis"},
   {1, "li xiang"},
   {1, "Matt Wilks"},
   {1, "Michiel Roks"},
   {1, "Oskar Oskar"},
   {1, "Patton Coles"},
   {1, "peter errens"},
   {1, "richard jal."},
   {1, "Ricky Enø Thomassen"},
   {1, "Ron Mar"},
   {1, "Sandro Funchal"},
   {1, "tarou nagae"},
   {1, "Tom Wassink"},
   {0, "ATHLETE_END"},
};

typedef struct NickNameLibrary_s
{
    uint32_t enabled;
    char name[MAX_NAME];
    char nickName[MAX_NAME];
} NickNameLibrary_t;

NickNameLibrary_t nickNameLibrary[] = {
    {1,"Steve Tappan",       "30% better than Bjarne"},
    {1,"Shaun Corbin",       "scorbi"},
//    {1,"Owen McGonagle",     "freddythecat"},
    {1,"Owen McGonagle",     "Thomas Shelby"},
    {1,"Steve Peplinski",    "McFartonator"},
    {1,"John Jeffries",      "the turtle"},
    {1,"Luke Elton",         "lukeekton93"},
    {1,"Rob Fullerton",      "mOb1u5"},
    {1,"Gabriel Mathisen",   "Squanchy"},
    {1,"Rob McKechney",      "The Badger"},
    {1,"Greg Langman",       "CLAIM_1"},
    {1,"Christian Kaldbar",  "Norseman"},
    {1,"Ryan Ness",          "CLAIM_1"},
    {1,"Ike Janssen",        "CLAIM_2"},
    {1,"Ronnie Abell",       "CLAIM_3"},
    {0, "ATHLETE_END", "-"},
};


extern void ColumnizeResults(CLI_PARSE_INFO *pInfo, char *fname, int columnId, int mode, char *infile);
extern void PartyTimeFilterCmd( CLI_PARSE_INFO *pInfo);
extern int TeamGrit(char *name);
extern int RacedToday(char *name, char *teamName);
extern void RidersMissed(CLI_PARSE_INFO *info);

void TeamNameCleanup(TimeLineInfo_t *timeLineInfo, char *team);
void PartyAddAthleteA(CLI_PARSE_INFO *info, char *name);
void PartyAddAthleteB(CLI_PARSE_INFO *info, char *name);
void ColumnReset(CLI_PARSE_INFO *pInfo);
void ListReset(CLI_PARSE_INFO *pInfo, LinkList_t *list);
void AthleteReset(CLI_PARSE_INFO *pInfo);
void PartyAddAthleteAll(CLI_PARSE_INFO *info, char *name);

static void PartyForceShow(CLI_PARSE_INFO *info);
static void PartyForceB(CLI_PARSE_INFO *info);
static void PartyWriteResults_common(CLI_PARSE_INFO *pInfo, char *outfile);

int FormattedProper(char *buff)
{
#if 0
   if ((strstr(buff, "-----")) || (strstr(buff, "Name ")))
   {
      /* properly formatted line not found */
      return 0;
   }
#endif

   return 1;
}

TimeLineInfo_t *
TimeLineCreate(CLI_PARSE_INFO *pInfo)
{
   uint32_t i;

   TimeLineInfo_t *thiss = NULL;

   /* find next available */
   for (i = 0; i < MAX_TIMELINES; i++)
   {
      if (timeLinePool[i].timeLineInfo.taken == 0)
      {
         timeLinePool[i].timeLineInfo.taken = 1;
         timeLinePool[i].timeLineInfo.objId = i;

         thiss = (TimeLineInfo_t *)&timeLinePool[i].timeLineInfo;
         break;
      }
   }

   if (i == MAX_TIMELINES)
   {
      (pInfo->print_fp)("FAILURE: could not find a free timeLine block, max = %d\n", MAX_TIMELINES);
      return NULL;
   }

   timeLineEnv.numberOfTimeLineInfoBlocks += 1;

   return thiss ;
}

void
TimeLineInfoFree(CLI_PARSE_INFO *pInfo, LinkList_t *list, TimeLineInfo_t *timeLineInfo)
{
   if (timeLineInfo->objId == -1)
   {
      (pInfo->print_fp)("INTERNAL ERROR: objId is -1\n");
      return;
   }

   LinkRemove(list, timeLineInfo->link);
   timeLinePool[timeLineInfo->objId].timeLineInfo.taken = 0;
}

void
TimeLineRemove(TimeLineInfo_t *timeLineInfo)
{
   int objId = timeLineInfo->objId;

   /* write pattern to catch reuse problems */
   memset(timeLineInfo, 0xCC, sizeof(TimeLineInfo_t));

   timeLineEnv.numberOfTimeLineInfoBlocks -= 1;

   timeLineInfo->taken = 0;
   timeLineInfo->objId = objId;
   timeLineInfo->timeLine[0] = '\0';
   timeLineInfo->wattLine[0] = '\0';
   timeLineInfo->groupLine[0] = '\0';
   timeLineInfo->timeStamp = 0;
   timeLineInfo->fraction = 0;
   timeLineInfo->msecs = 0;
   timeLineInfo->type = TYPE_UNKNOWN;
   timeLineInfo->processed = 0;
   timeLineInfo->pair = NULL;
   timeLineInfo->paired = 0;
   timeLineInfo->numRaces = 0;
}


TimeLineInfo_t *
TimeLineInfoNew(CLI_PARSE_INFO *pInfo, int type)
{
   int i;
   TimeLineInfo_t *timeLineInfo;
   PLATFORM_POINTER_T deletedObject = 0;

   if (timeLineEnv.numberOfTimeLineInfoBlocks == timeLineEnv.timeLineInfoLength)
   {
      (pInfo->print_fp)("ERROR: number of current sessions = %d is greater than max = %d\n",
                        timeLineEnv.numberOfTimeLineInfoBlocks, timeLineEnv.timeLineInfoLength);
      return (TimeLineInfo_t *)NULL;
   }

   /* Create a session */
   timeLineInfo = TimeLineCreate(pInfo);

   timeLineInfo->taken = 1;
   timeLineInfo->timeLine[0] = '\0';
   timeLineInfo->timeStamp = 0;
   timeLineInfo->fraction = 0;
   timeLineInfo->msecs = 0;
   timeLineInfo->type = type;
   timeLineInfo->processed = 0;
   timeLineInfo->pair = NULL;
   timeLineInfo->paired = 0;

   timeLineInfo->av = 0;
   timeLineInfo->avOrig = 0;
   timeLineInfo->totalTime = 0;

   timeLineInfo->team[0] = '\0';

   timeLineInfo->name[0] = '\0';
   timeLineInfo->first[0] = '\0';
   timeLineInfo->last[0] = '\0';

   timeLineInfo->points = 0;
   timeLineInfo->totalRaces = 0;
   timeLineInfo->groupId = GROUP_UNKNOWN;

   timeLineInfo->prIdx.lastFound = 0;
   timeLineInfo->prIdx.improvement = 0;

   for (i = 0; i < MAX_RACES; i++)
   {
      timeLineInfo->raceOrder[i].pos = -1;
      timeLineInfo->race[i].raceName[0] = '\0';
      timeLineInfo->race[i].place = -1;
      timeLineInfo->race[i].points = 0;
      timeLineInfo->race[i].time.f = LOWEST_TIME;
      timeLineInfo->race[i].time.i = (int)LOWEST_TIME;
   }

   if ((PLATFORM_POINTER_T)timeLineInfo == deletedObject)
   {
      (pInfo->print_fp)("GOTCHA: broken early on\n");
   }

   if (timeLineInfo == NULL)
   {
      (pInfo->print_fp)("INTERNAL ERROR: could not create stat info structure successfully\n");
      return NULL;
   }

   return(timeLineInfo);

}

void RemoveBracketsAndClip(char *out, char *in)
{
   char *ptrIn = in;
   // char *ptrOut = out;
   char name[MAX_STRING_SIZE];
   char *ptrOut = &name[0];

   while (*ptrIn != '\0')
   {
      if (*ptrIn == '[')
      {
         // *ptrOut = ' ';
         ptrIn++;
         continue;
      }
      else if (*ptrIn == ']')
      {
         *ptrOut = '\0';
         break;
      }
      else
      {
         *ptrOut = *ptrIn;
      }

      ptrIn++;
      ptrOut++;
   }

   *ptrOut = '\0';

#if 0
   /* Now clip it */
   prtOut = &name[0];

   /* Find first non space */
   while (*ptrOut != '\0')
   {
      if (*ptrOut != ' ')
      {
         break;
      }
      ptrOut++;
   }

   if (*ptrOut == '\0')
   {
      /* No spaces in name */
      strcpy(out, name);
      return;
   }
#endif

   /* Shift it */
   strcpy(out, name);
}


TimeLineInfo_t *
TimeLineInfoInsert(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, LinkList_t *list, int race)
{
   Link_t *link;
   Link_t *ptr;
   TimeLineInfo_t *currInfo;
   int nameFound = 0;
   int i;
   char team1[MAX_STRING_SIZE];
   char team2[MAX_STRING_SIZE];

   link = LinkCreate((void *)timeLineInfo);

   if (link == NULL)
   {
      printf("ERROR: Could not allocate link for session block\n");
      return NULL;
   }

#if 0
   if (partyControl.lockDown == 1)
   {
      (pInfo->print_fp)("ADDED: %s\n", timeLineInfo->name);
   }
#endif

   /* See if name already exists */

    ptr = (Link_t *)list->head;
    currInfo = (TimeLineInfo_t *)ptr->currentObject;
    if (currInfo == NULL)
    {
        /* First one */
        LinkHeadAdd(list, link);
    }
    else
    {
        ptr = (Link_t *)list->head;
        while (ptr->next != NULL)
        {
            currInfo = (TimeLineInfo_t *)ptr->currentObject;

            // RemoveBracketsAndClip(team1, currInfo->team);
            // RemoveBracketsAndClip(team2, timeLineInfo->team);

            if (strcmp(timeLineInfo->name, currInfo->name) == 0)
            {
               nameFound = 1;

               /* If we are adding a race result, do so now */
               for (i = 0; i < MAX_RACES; i++)
               {
                  if (timeLineInfo->race[i].place != -1)
                  {
                     currInfo->race[i].place = timeLineInfo->race[i].place;
                     strcpy(currInfo->race[i].raceName, timeLineInfo->race[i].raceName);
                     currInfo->race[i].time = timeLineInfo->time;

                     currInfo->numRaces = (i+1);
                     break;
                  }
               }
               break;
            }

            ptr = ptr->next;
        }

        if (nameFound == 0)
        {
           LinkTailAdd(list, link);
        }
    }

   return (timeLineInfo);
}

void
TimeLineInfoUpdate(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, LinkList_t *list, char *raceName)
{
   int i;

   for (i = 0; i < MAX_RACES; i++)
   {
      if ((timeLineInfo->race[i].place != -1) && (strcmp(timeLineInfo->race[i].raceName, raceName) == 0))
      {
         timeLineInfo->race[i].time = timeLineInfo->time;
//         (pInfo->print_fp)("INFO1: %d: %s: %4.4f  %s \n", i, timeLineInfo->race[i].raceName, timeLineInfo->time.f, timeLineInfo->name);
         break;
      }
   }
}

TimeLineInfo_t *
TimeOrderInfoInsert(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, LinkList_t *list, int race, int mode)
{
   Link_t *link;
   Link_t *ptr;
   TimeLineInfo_t *currInfo;
   int orderIdx1;
   int orderIdx2;
   int nameFound = 0;
   int i;

   link = LinkCreate((void *)timeLineInfo);

   if (link == NULL)
   {
      printf("ERROR: Could not allocate link for session block\n");
      return NULL;
   }

    ptr = (Link_t *)list->head;
    currInfo = (TimeLineInfo_t *)ptr->currentObject;
    if (currInfo == NULL)
    {
        /* First one */
        timeLineInfo->link = link;
        LinkHeadAdd(list, link);
    }
    else
    {
        ptr = (Link_t *)list->head;
        while (ptr->next != NULL)
        {
            currInfo = (TimeLineInfo_t *)ptr->currentObject;

            if (mode == TIME_ORDER_POINTS)
            {
               if (currInfo->points <= timeLineInfo->points)
               {
                  LinkBefore(list, ptr, link);
                  nameFound = 1;
                  break;
               }
            }
            else if (mode == TIME_ORDER_TEMPOISH)
            {
               orderIdx1 = currInfo->raceOrder[0].pos;
               orderIdx2 = timeLineInfo->raceOrder[0].pos;

               // if (currInfo->av <= timeLineInfo->av)
               // if (timeLineInfo->race[orderIdx2].time.f <= currInfo->race[orderIdx1].time.f)
               if (timeLineInfo->av <= currInfo->av)
               {
                  LinkBefore(list, ptr, link);
                  nameFound = 1;
                  break;
               }
            }
            else if (mode == TIME_ORDER_RICHMOND)
            {
               orderIdx1 = currInfo->raceOrder[0].pos;
               orderIdx2 = timeLineInfo->raceOrder[0].pos;

               // if (currInfo->av <= timeLineInfo->av)
               // if (timeLineInfo->race[orderIdx2].time.f <= currInfo->race[orderIdx1].time.f)
               if (timeLineInfo->totalTime <= currInfo->totalTime)
               {
                  LinkBefore(list, ptr, link);
                  nameFound = 1;
                  break;
               }
            }
            else if (mode == TIME_ORDER_TIME)
            {
               // orderIdx1 = currInfo->raceOrder[0].pos;
               // orderIdx2 = timeLineInfo->raceOrder[0].pos;
               orderIdx1 = (numRacesG - 1);

               if (timeLineInfo->race[orderIdx1].time.f <= currInfo->race[orderIdx1].time.f)
               {
                  LinkBefore(list, ptr, link);
                  nameFound = 1;
                  break;
               }
            }

            ptr = ptr->next;
        }

        if (nameFound == 0)
        {
           timeLineInfo->link = link;
           LinkTailAdd(list, link);
        }
    }

   return (timeLineInfo);
}

void TimeLineInit(CLI_PARSE_INFO *pInfo)
{
   uint32_t size;
   Link_t *nil;

   size = MAX_TIMELINES * sizeof(TimeLineInfo_t);

   if (timeLinePool != NULL)
   {
      free(timeLinePool);
   }

   if (listTimeLine != NULL)
   {
      ListReset(pInfo, listTimeLine);
      listTimeLine = NULL;
   }

   if (listTimeOrder != NULL)
   {
      ListReset(pInfo, listTimeOrder);
      listTimeOrder = NULL;
   }

   timeLinePool = (TimeLinePool_t *)malloc(size);

   if (timeLinePool == NULL)
   {
      (pInfo->print_fp)("ERROR: Could not create timeLine Info Pool\n");
      exit(-1);
   }

   memset(timeLinePool, 0, size);

   timeLineEnv.numberOfTimeLineInfoBlocks = 0;
   timeLineEnv.timeLineInfoLength = MAX_TIMELINES;

   /* Create link list of step infomation entries */
   if (listTimeLine == NULL)
   {
      listTimeLine = LinkListCreate();
      nil = LinkCreate(NULL);
      LinkTailAdd(listTimeLine, nil);
   }

   /* Create link list of PidProcess entries */
   if (listTimeOrder == NULL)
   {
      listTimeOrder = LinkListCreate();
      nil = LinkCreate(NULL);
      LinkTailAdd(listTimeOrder, nil);
   }
}

static void cmd_party_start(CLI_PARSE_INFO *info)
{
   timeTable.on = 1;
}

static void cmd_party_stop(CLI_PARSE_INFO *info)
{
   timeTable.on = 0;
}

char *StringGet(char *out, FILE *fp, int nl)
{
   char *ret;
   int len;
   char tmp[MAX_STRING_SIZE];

   ret = fgets(tmp, MAX_STRING_SIZE, fp);

   if (ret != NULL)
   {
      ReplaceTabs(out, tmp);
   }

   if (nl == NL_REMOVE)
   {
      len = strlen(out);

      /* Add in xtra space at the end, just in case */
      out[len-1] = ' ';
      out[len] = ' ';
      out[len+1] = '\0';
   }

   return(ret);
}

void NameInsert(TimeLineInfo_t *timeLineInfo, char *name)
{
   char *ptr;
   char tmp[MAX_STRING_SIZE];
   char team[MAX_STRING_SIZE];
   int teamFound = 0;
   int ret = 0;

   if ((ptr = strstr(name, "LEADER:")) != NULL)
   {
      strcpy(tmp, &name[8]);
   }
   else if ((ptr = strstr(name, "SWEEPER:")) != NULL)
   {
      strcpy(tmp, &name[9]);
   }
   else
   {
      strcpy(tmp, name);
   }

   strcpy(timeLineInfo->name, tmp);

   ptr = timeLineInfo->name;
   while (*ptr != '\0')
   {
      if (!isprint(*ptr))
      {
         // *ptr = ' ';
         *ptr = '_';
      }
      ptr++;
   }

   RemoveSpaceAtEnd(ptr);

#if 1
   /* Split team from name */
   ptr = tmp;

   if (strstr(tmp, "MHA_ KOH") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "MHA KOH");
      strcpy(timeLineInfo->team, "[JETT]");
   }
   else if (strstr(tmp, "Derek Sawyer") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Derek Sawyer");
      strcpy(timeLineInfo->team, "[GRIT][Rippers]");
   }
   else if (strstr(tmp, "conrad moss") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Conrad Moss");
      strcpy(timeLineInfo->team, "Velopower PC, P-TJ RT");
   }
   else if (strstr(tmp, "Oleg Semeko") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Oleg Semeko");
      strcpy(timeLineInfo->team, "Veter.cc");
   }
   else if (strstr(tmp, "Gabriel Mathisen") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Gabriel Mathisen");
      strcpy(timeLineInfo->team, "[VEGAN][GRIT]");
   }
   else if ((strstr(tmp, "Korea") != 0) && (strstr(tmp, "Marc") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Korea Marc");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(tmp, "Ren") != 0) && (strstr(tmp, "Jeppesen") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Rene Jeppesen");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(tmp, "tak ina") != 0) || (strstr(tmp, "Tak Ina") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Tak Ina");
      strcpy(timeLineInfo->team, "[JETT][GRIT]");
   }
   else if (strstr(tmp, "VV Cucumber") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "VV Cucumber");
      strcpy(timeLineInfo->team, "[LOOKCYCLE CHN]");
   }
   else if (strstr(tmp, "Ricardo Abela") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Ricardo Abela");
      strcpy(timeLineInfo->team, "TEAM BMTR");
   }
   else if (strstr(tmp, "Dan Nelson") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Dan Nelson");
      strcpy(timeLineInfo->team, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(tmp, "Kelly Toth") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Kelly Toth");
      strcpy(timeLineInfo->team, "CRCA/NYCC");
   }
   else if (strstr(tmp, "John Jeffries") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "John Jeffries");
      strcpy(timeLineInfo->team, "[AA Bikes][GRIT]");
   }
   else if (strstr(tmp, "Dan Mitchell") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Dan Mitchell");
      strcpy(timeLineInfo->team, "MDV, IG");
   }
   else if (strstr(tmp, "James Bolze") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "James Bolze");
      strcpy(timeLineInfo->team, "CVC RACING");
   }
   else if (strstr(tmp, "Eric Andr") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Eric Andre");
      strcpy(timeLineInfo->team, "-");
   }
   else if (strstr(tmp, "Matt A.") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Matt A. P.");
      strcpy(timeLineInfo->team, "DIRT");
   }
   else if (strstr(tmp, "Shaun Corbin") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Shaun Corbin");
      strcpy(timeLineInfo->team, "[GRIT][DIRT]");
   }
   else if (strstr(tmp, "Guido Kramer Leymen") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Guido Kramer Leymen");
      strcpy(timeLineInfo->team, "-");
   }
   else if (strstr(tmp, "Paul Roy") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Paul Roy");
      strcpy(timeLineInfo->team, "RETO!");
   }
   else if (strstr(tmp, "Guillaume Lafleur") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Guillaume Lafleur");
      strcpy(timeLineInfo->team, "leMETIER");
   }
   else if (strstr(tmp, "Bruch Wu") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Bruch Wu");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(tmp, "Bergenzaun") != 0) && (strstr(tmp, "ns ") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Mans Bergenzaun");
      strcpy(timeLineInfo->team, "TEAM BTC");
   }
   else if ((strstr(tmp, "Egon") != 0) && (strstr(tmp, "Ivan") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Egon Ivanjsek");
      strcpy(timeLineInfo->team, "PPRteam");
   }
   else if ((strstr(tmp, "Thomas") != 0) && (strstr(tmp, "Schr") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Thomas Schrader");
      strcpy(timeLineInfo->team, "ZRG");
   }
   else if (strstr(tmp, "C hris M") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Chris M");
      strcpy(timeLineInfo->team, "B4H");
   }
   else if (strstr(tmp, "Seth G ") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Seth G");
      strcpy(timeLineInfo->team, "[GRIT]");
   }
   else if (strstr(tmp, "Renfrew Bunch") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Renfrew Bunch");
      strcpy(timeLineInfo->team, "[Scotland]");
   }
   else if (!isprint(tmp[0]) && !isprint(tmp[1]) && !isprint(tmp[2]))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "__STAR__");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(tmp, "Bergenzaun") != 0) && (strstr(tmp, "M") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Mans Bergenzaun");
      strcpy(timeLineInfo->team, "Team BTC");
   }
   else if (strstr(tmp, "Dax Nelson") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Dax Nelson");
      strcpy(timeLineInfo->team, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(tmp, "Greg Langman") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Greg Langman");
      strcpy(timeLineInfo->team, "DIRT");
   }
   else if ((strstr(tmp, "Hiroki") != 0) && (strstr(tmp, "Tanaka") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Hiroki Tanaka");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(tmp, "Hans") != 0) && (strstr(tmp, "rgen") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Hans-Jurgen G");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(tmp, "J B") != 0) && (strstr(tmp, "SWOZR") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "J B #1");
      strcpy(timeLineInfo->team, "SWOZR");
   }
   else if ((strstr(tmp, "J B") != 0) && (strstr(tmp, "Bees") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "J B #2");
      strcpy(timeLineInfo->team, "BCC - Bees");
   }
#if 0
   else if (strstr(tmp, "J B") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "J B");
      strcpy(timeLineInfo->team, "-");
   }
#endif
   else if (strstr(tmp, "Sebastien Loir") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Sebastien Loir");
      strcpy(timeLineInfo->team, "BIKES-FR");
   }
   else if (strstr(tmp, "Greg Rashford") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Greg Rashford");
      strcpy(timeLineInfo->team, "#NOMAD");
   }
   else if (strstr(tmp, "Dan Newton") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Dan Newton");
      strcpy(timeLineInfo->team, "WGT - Bynea CC");
   }
   else if (strstr(tmp, "Dan Nelson") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Dan Nelson");
      strcpy(timeLineInfo->team, "!DUX! TPA-FLA - HERD");
   }
   else if ((strstr(tmp, "Rafa") != 0) && (strstr(tmp, "Dzieko") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Rafa Dzieko");
      strcpy(timeLineInfo->team, "ski ZTPL.CC");
   }
#if 0
   else if (strstr(tmp, "Denis Frolov") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Denis Frolov");
      strcpy(timeLineInfo->team, "UTT");
   }
   else if (strstr(tmp->name, "Roy Woodland") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Roy Woodland");
      strcpy(timeLineInfo->team, "PACK");
   }
#endif
   else if (strstr(tmp, "Steve Tappan") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Steve Tappan");
      strcpy(timeLineInfo->team, "[GRIT]");
   }
   else
   {
      if ((timeLineInfo->team[0] == '\0') || (timeLineInfo->team[0] == '-'))
      {
         while (*ptr != '\0')
         {
            if (isdigit(*ptr))
            {
               /* If we hit a number at any point before a true team name - assume it is ZwiftPower nametag */
               strcpy(timeLineInfo->team, "-");
               teamFound = 1;

               ptr--;
               while (*ptr == ' ')
               {
                  ptr--;
               }

               ptr++;
               *ptr = '\0';
               strcpy(timeLineInfo->name, tmp);
               break;
            }
            else if ((*ptr == '[') || (*ptr == '(') || (*ptr == '_'))
            {
               teamFound = 1;
               strcpy(timeLineInfo->team, ptr);
               // ptrName = ptr;

               ptr--;
               while (*ptr == ' ')
               {
                  ptr--;
               }

               ptr++;
               *ptr = '\0';

               strcpy(timeLineInfo->name, tmp);

               // TeamNameCleanup(timeLineInfo, ptrName);
               break;
            }

            ptr++;
         }
      }
   }

   if (teamFound == 0)
   {
       /* See if name shows up in our team list */
       if ((ret = TeamGrit(timeLineInfo->name)) == 1)
       {
          strcpy(timeLineInfo->team, "[GRIT]");
       }
       else
       {
          /* Just put in a dash for now */
          strcpy(timeLineInfo->team, "-");
       }
   }
#endif

}

void NameDouble(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo)
{
   TimeLineInfo_t *currInfo;
   Link_t *ptr;

   if (partyControl.debug == 1)
   {
      /* First, spit out error, if DEBUG is TURNED ON!!!, if we see DOUBLE names */
      ptr = (Link_t *)listTimeLine->head;
      while (ptr->next != NULL)
      {
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

         if ((strcmp(currInfo->name, timeLineInfo->name) == 0) &&
             (strcmp(currInfo->team, timeLineInfo->team) != 0))
         {
            (pInfo->print_fp)("INFO: name = **%s**, team = **%s** DIFFERENT than name = **%s**, team = **%s** \n",
                              currInfo->name, currInfo->team, timeLineInfo->name, timeLineInfo->team);
         }

         ptr = ptr->next;
      }
   }

   if (partyControl.debug != 1)
   {
      if ((strstr(timeLineInfo->name, "J B") != 0) && (strstr(timeLineInfo->team, "SWOZR") != 0))
      {
         strcpy(timeLineInfo->name, "J B #1");
         strcpy(timeLineInfo->team, "SWOZR");
      }
      else if ((strstr(timeLineInfo->name, "J B") != 0) && (strstr(timeLineInfo->team, "Bees") != 0))
      {
         strcpy(timeLineInfo->name, "J B #2");
         strcpy(timeLineInfo->team, "BCC - Bees");
      }
   }
}

void TeamNameCleanup(TimeLineInfo_t *timeLineInfo, char *team)
{
   int len;

   if (strstr(timeLineInfo->name, "MHA_ KOH") != 0)
   {
      strcpy(timeLineInfo->name, "MHA KOH");
      strcpy(timeLineInfo->team, "[JETT]");
   }
   else if (strstr(timeLineInfo->name, "Derek Sawyer") != 0)
   {
      strcpy(timeLineInfo->name, "Derek Sawyer");
      strcpy(timeLineInfo->team, "[GRIT][Rippers]");
   }
   else if (strstr(timeLineInfo->name, "Dan Nelson") != 0)
   {
      strcpy(timeLineInfo->name, "Dan Nelson");
      strcpy(timeLineInfo->team, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(timeLineInfo->name, "Kelly Toth") != 0)
   {
      strcpy(timeLineInfo->name, "Kelly Toth");
      strcpy(timeLineInfo->team, "CRCA/NYCC");
   }
   else if (strstr(timeLineInfo->name, "Bruch Wu") != 0)
   {
      strcpy(timeLineInfo->name, "Bruch Wu");
      strcpy(timeLineInfo->team, "-");
   }
   else if (strstr(timeLineInfo->name, "John Jeffries") != 0)
   {
      strcpy(timeLineInfo->name, "John Jeffries");
      strcpy(timeLineInfo->team, "[AA Bikes][GRIT]");
   }
   else if (strstr(timeLineInfo->name, "Dan Mitchell") != 0)
   {
      strcpy(timeLineInfo->name, "Dan Mitchell");
      strcpy(timeLineInfo->team, "MDV, IG");
   }
   else if (strstr(timeLineInfo->name, "James Bolze") != 0)
   {
      strcpy(timeLineInfo->name, "James Bolze");
      strcpy(timeLineInfo->team, "CVC RACING");
   }
   else if (strstr(timeLineInfo->team, "Eric Andr") != 0)
   {
      strcpy(timeLineInfo->name, "Eric Andre");
      strcpy(timeLineInfo->team, "-");
   }
   else if (strstr(timeLineInfo->name, "Matt A.") != 0)
   {
      strcpy(timeLineInfo->name, "Matt A. P.");
      strcpy(timeLineInfo->team, "DIRT");
   }
   else if (strstr(timeLineInfo->name, "Shaun Corbin") != 0)
   {
      strcpy(timeLineInfo->name, "Shaun Corbin");
      strcpy(timeLineInfo->team, "[GRIT][DIRT]");
   }
   else if (strstr(timeLineInfo->name, "Guido Kramer Leymen") != 0)
   {
      strcpy(timeLineInfo->name, "Guido Kramer Leymen");
      strcpy(timeLineInfo->team, "-");
   }
   else if (strstr(timeLineInfo->name, "Paul Roy") != 0)
   {
      strcpy(timeLineInfo->name, "Paul Roy");
      strcpy(timeLineInfo->team, "RETO!");
   }
   else if (strstr(timeLineInfo->name, "Guillaume Lafleur") != 0)
   {
      strcpy(timeLineInfo->name, "Guillaume Lafleur");
      strcpy(timeLineInfo->team, "leMETIER");
   }
   else if ((strstr(timeLineInfo->name, "Bergenzaun") != 0) && (strstr(timeLineInfo->name, "ns ") != 0))
   {
      strcpy(timeLineInfo->name, "Mans Bergenzaun");
      strcpy(timeLineInfo->team, "TEAM BTC");
   }
   else if ((strstr(timeLineInfo->name, "Egon") != 0) && (strstr(timeLineInfo->name, "Ivan") != 0))
   {
      strcpy(timeLineInfo->name, "Egon Ivanjsek");
      strcpy(timeLineInfo->team, "PPRteam");
   }
   else if ((strstr(timeLineInfo->name, "Thomas") != 0) && (strstr(timeLineInfo->name, "Schr") != 0))
   {
      strcpy(timeLineInfo->name, "Thomas Schrader");
      strcpy(timeLineInfo->team, "ZRG");
   }
   else if (strstr(timeLineInfo->name, "C hris M") != 0)
   {
      strcpy(timeLineInfo->name, "Chris M");
      strcpy(timeLineInfo->team, "B4H");
   }
   else if (strstr(timeLineInfo->name, "Seth G ") != 0)
   {
      strcpy(timeLineInfo->name, "Seth G");
      strcpy(timeLineInfo->team, "[GRIT]");
   }
   else if (strstr(timeLineInfo->name, "Renfrew Bunch") != 0)
   {
      strcpy(timeLineInfo->name, "Renfrew Bunch");
      strcpy(timeLineInfo->team, "[Scotland]");
   }
   else if (!isprint(timeLineInfo->name[0]) && !isprint(timeLineInfo->name[1]) && !isprint(timeLineInfo->name[2]))
   {
      strcpy(timeLineInfo->name, "__STAR__");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(timeLineInfo->name, "Bergenzaun") != 0) && (strstr(timeLineInfo->name, "M") != 0))
   {
      strcpy(timeLineInfo->name, "Mans Bergenzaun");
      strcpy(timeLineInfo->team, "Team BTC");
   }
   else if (strstr(timeLineInfo->name, "Dax Nelson") != 0)
   {
      strcpy(timeLineInfo->name, "Dax Nelson");
      strcpy(timeLineInfo->team, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(timeLineInfo->name, "Greg Langman") != 0)
   {
      strcpy(timeLineInfo->name, "Greg Langman");
      strcpy(timeLineInfo->team, "DIRT");
   }
   else if ((strstr(timeLineInfo->name, "Hiroki") != 0) && (strstr(timeLineInfo->name, "Tanaka") != 0))
   {
      strcpy(timeLineInfo->name, "Hiroki Tanaka");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(timeLineInfo->name, "Hans") != 0) && (strstr(timeLineInfo->name, "rgen") != 0))
   {
      strcpy(timeLineInfo->name, "Hans-Jurgen G");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(timeLineInfo->name, "J B") != 0) && (strstr(team, "SWOZR") != 0))
   {
      strcpy(timeLineInfo->name, "J B #1");
      strcpy(timeLineInfo->team, "SWOZR");
   }
   else if ((strstr(timeLineInfo->name, "J B") != 0) && (strstr(team, "Bees") != 0))
   {
      strcpy(timeLineInfo->name, "J B #2");
      strcpy(timeLineInfo->team, "BCC - Bees");
   }
#if 0
   else if (strstr(timeLineInfo->name, "J B") != 0)
   {
      strcpy(timeLineInfo->name, "J B");
      strcpy(timeLineInfo->team, "-");
   }
#endif
   else if (strstr(timeLineInfo->name, "Sebastien Loir") != 0)
   {
      strcpy(timeLineInfo->name, "Sebastien Loir");
      strcpy(timeLineInfo->team, "BIKES-FR");
   }
   else if (strstr(timeLineInfo->name, "Greg Rashford") != 0)
   {
      strcpy(timeLineInfo->name, "Greg Rashford");
      strcpy(timeLineInfo->team, "#NOMAD");
   }
   else if (strstr(timeLineInfo->name, "Dan Newton") != 0)
   {
      strcpy(timeLineInfo->name, "Dan Newton");
      strcpy(timeLineInfo->team, "WGT - Bynea CC");
   }
   else if (strstr(timeLineInfo->name, "Dan Nelson") != 0)
   {
      strcpy(timeLineInfo->name, "Dan Nelson");
      strcpy(timeLineInfo->team, "!DUX! TPA-FLA - HERD");
   }
   else if ((strstr(timeLineInfo->name, "Rafa") != 0) && (strstr(timeLineInfo->name, "Dzieko") != 0))
   {
      strcpy(timeLineInfo->name, "Rafa Dzieko");
      strcpy(timeLineInfo->team, "ski ZTPL.CC");
   }
   else if (strstr(timeLineInfo->name, "conrad moss") != 0)
   {
      strcpy(timeLineInfo->name, "Conrad Moss");
      strcpy(timeLineInfo->team, "Velopower PC, P-TJ RT");
   }
   else if (strstr(timeLineInfo->name, "Oleg Semeko") != 0)
   {
      strcpy(timeLineInfo->name, "Oleg Semeko");
      strcpy(timeLineInfo->team, "Veter.cc");
   }
   else if (strstr(timeLineInfo->name, "Gabriel Mathisen") != 0)
   {
      strcpy(timeLineInfo->name, "Gabriel Mathisen");
      strcpy(timeLineInfo->team, "[VEGAN][GRIT]");
   }
   else if ((strstr(timeLineInfo->name, "Korea") != 0) && (strstr(timeLineInfo->name, "Marc") != 0))
   {
      strcpy(timeLineInfo->name, "Korea Marc");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(timeLineInfo->name, "Ren") != 0) && (strstr(timeLineInfo->name, "Jeppesen") != 0))
   {
      strcpy(timeLineInfo->name, "Rene Jeppesen");
      strcpy(timeLineInfo->team, "-");
   }
   else if ((strstr(timeLineInfo->name, "tak ina") != 0) || (strstr(timeLineInfo->name, "Tak Ina") != 0))
   {
      strcpy(timeLineInfo->name, "Tak Ina");
      strcpy(timeLineInfo->team, "[JETT][GRIT]");
   }
   else if (strstr(timeLineInfo->name, "VV Cucumber") != 0)
   {
      strcpy(timeLineInfo->name, "VV Cucumber");
      strcpy(timeLineInfo->team, "[LOOKCYCLE CHN]");
   }
   else if (strstr(timeLineInfo->name, "Ricardo Abela") != 0)
   {
      strcpy(timeLineInfo->name, "Ricardo Abela");
      strcpy(timeLineInfo->team, "TEAM BMTR");
   }
#if 0
   else if (strstr(timeLineInfo->name, "Denis Frolov") != 0)
   {
      strcpy(timeLineInfo->name, "Denis Frolov");
      strcpy(timeLineInfo->team, "UTT");
   }
   else if (strstr(timeLineInfo->name, "Roy Woodland") != 0)
   {
      strcpy(timeLineInfo->name, "Roy Woodland");
      strcpy(timeLineInfo->team, "PACK");
   }
#endif
   else if (strstr(timeLineInfo->name, "Steve Tappan") != 0)
   {
      strcpy(timeLineInfo->name, "Steve Tappan");
      strcpy(timeLineInfo->team, "[GRIT]");
   }
   else
   {
      /* Remove newline */
      len = strlen(team);
      if (team[len-1] == '\n');
      {
#if 0
         /* Keep space after team for now? */
         team[len-1] = ' ';
         team[len+1] = '\0';
#else
         team[len-1] = '\0';
#endif
      }

      /* If name shows up starting with a number - assume it is ZwiftPower nametag */
      if (isdigit(team[0]))
      {
         strcpy(timeLineInfo->team, "-");
      }
      else
      {
         strcpy(timeLineInfo->team, team);
      }
   }
}

void AddUpPoints(CLI_PARSE_INFO *pInfo, int count, int raceId)
{
   float fudge = 1.0;
   Link_t *ptr;
   TimeLineInfo_t *timeLineInfo;
   int newCount = 0;

   if (partyControl.max != -1)
   {
      if (count > partyControl.max)
      {
         (pInfo->print_fp)("WARNING!!!!! WARNING!!!! you set max = %d, found count = %d\n", partyControl.max, count);
         partyControl.max = count;
         newCount = count;
      }
      else
      {
         newCount = partyControl.max;
      }

      fudge = (float)partyControl.max/(float)count;
   }
   else
   {
      fudge = 1.0;
   }

   (pInfo->print_fp)("partyControl.max = %d, count = %d, fudge = %4.4f\n", partyControl.max, count, fudge);

   /* Now, go through each one - if the race is set, add in points based on total in race */
   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

      if (timeLineInfo->race[raceId].place != -1)
      {
#ifdef WEIGHT_TRY1
         if (partyControl.max != -1)
         {
            timeLineInfo->race[raceId].points = (newCount - timeLineInfo->race[raceId].place);
         }
         else
         {
            timeLineInfo->race[raceId].points = (count - timeLineInfo->race[raceId].place);
         }
#else
         timeLineInfo->race[raceId].points = (count - timeLineInfo->race[raceId].place);
         timeLineInfo->race[raceId].points = (int)((float)timeLineInfo->race[raceId].points * fudge);
#endif
         
         // +++owen - fix hardcode to a must of raceId == 3
         if ((raceId == 2) && (partyControl.doubleUp == 1))
         {
            timeLineInfo->race[raceId].points *= 2;
         }

         timeLineInfo->points += timeLineInfo->race[raceId].points;
      }

      // (pInfo->print_fp)("%s", timeLineInfo->timeLine);
      ptr = ptr->next;
   }
}

int AthleteSkip(CLI_PARSE_INFO *pInfo, LinkList_t *list, TimeLineInfo_t *timeLineInfo, int groupId)
{
   int i, j;
   Link_t *ptr;

   if (partyControl.nickName == 1)
   {
      /* NO - for A check - if we ALSO do NOT see in B - we throw in A pool !!! */
      for (j = 0; j < MAX_ATHLETES; j++)
      {
         if (strstr(nickNameLibrary[j].name, "ATHLETE_END") != 0)
         {
            break;
         }

         if (strcmp(nickNameLibrary[j].name, timeLineInfo->name) == 0)
         {
            /* Swap with nickName - all good */
            strcpy(timeLineInfo->name, nickNameLibrary[j].nickName);
            return 0;
         }
      }

      if (j == MAX_ATHLETES)
      {
         (pInfo->print_fp)("INTERNAL ERROR: Could not ignore - too many athletes!!!\n");
      }

      return 1;
   }
   else if (partyControl.groupId == GROUP_A)
   {
      for (i = 0; i < MAX_ATHLETES; i++)
      {
         if (athleteA[i].enabled == 0)
         {
            /* SKIP - did not find A Athlete */

            /* NO - for A check - if we ALSO do NOT see in B - we throw in A pool !!! */
            for (j = 0; j < MAX_ATHLETES; j++)
            {
               if (athleteB[j].enabled == 0)
               {
                  /* KEEP - not in A, not in B */
                  return 0;
               }

               if (strcmp(athleteB[j].name, timeLineInfo->name) == 0)
               {
                  /* NOT an A, but is a B - therefor, SKIP!!! */
                  return 1;
               }
            }


            if (j == MAX_ATHLETES)
            {
               (pInfo->print_fp)("INTERNAL ERROR: Could not ignore - too many athletes!!!\n");
            }

            return 1;
         }

         if (strcmp(athleteA[i].name, timeLineInfo->name) == 0)
         {
            /* Found A athlete - DO NOT SKIP */
            return 0;
         }
      }
   }
   else if (partyControl.groupId == GROUP_B)
   {
      for (i = 0; i < MAX_ATHLETES; i++)
      {
         if (athleteB[i].enabled == 0)
         {
            return 1;
         }

         if (strcmp(athleteB[i].name, timeLineInfo->name) == 0)
         {
            /* Found B athlete - DO NOT SKIP */
            return 0;
         }
      }
   }
   else if (partyControl.groupId == GROUP_ALL)
   {
      for (i = 0; i < MAX_ATHLETES; i++)
      {
         if (athleteAll[i].enabled == 0)
         {
            return 1;
         }

         if ((strstr(athleteAll[i].name, timeLineInfo->last) != 0) && (strlen(timeLineInfo->last) > 2))
         {
            if (strstr(athleteAll[i].name, timeLineInfo->first) == 0)
            {
               (pInfo->print_fp)(" MISS: name = %s, last = %s, first = %s\n",
                                 athleteAll[i].name, timeLineInfo->last, timeLineInfo->first);
            }
            else
            {
               (pInfo->print_fp)("MATCH: name = %s, last = %s, first = %s\n",
                                 athleteAll[i].name, timeLineInfo->last, timeLineInfo->first);
            }

            /* Found athlete - DO NOT SKIP */
            return 0;
         }

         if (strcmp(athleteAll[i].name, timeLineInfo->name) == 0)
         {
            (pInfo->print_fp)("  HOW: name = %s, last = %s, first = %s\n",
                              athleteAll[i].name, timeLineInfo->last, timeLineInfo->first);

            /* Found athlete - DO NOT SKIP */
            return 0;
         }
      }
   }
   else if (partyControl.groupId == GROUP_GRIT)
   {
      for (i = 0; i < MAX_ATHLETES; i++)
      {
         if (athleteGRIT[i].enabled == 0)
         {
            return 1;
         }

         if ((strstr(athleteGRIT[i].name, timeLineInfo->last) != 0) && (strlen(timeLineInfo->last) > 2))
         {
            if (strstr(athleteGRIT[i].name, timeLineInfo->first) == 0)
            {
               (pInfo->print_fp)(" MISS: name = %s, last = %s, first = %s\n",
                                 athleteGRIT[i].name, timeLineInfo->last, timeLineInfo->first);
            }
            else
            {
               (pInfo->print_fp)("MATCH: name = %s, last = %s, first = %s\n",
                                 athleteGRIT[i].name, timeLineInfo->last, timeLineInfo->first);
            }

            /* Found athlete - DO NOT SKIP */
            return 0;
         }

         if (strcmp(athleteGRIT[i].name, timeLineInfo->name) == 0)
         {
            (pInfo->print_fp)("  HOW: name = %s, last = %s, first = %s\n",
                              athleteGRIT[i].name, timeLineInfo->last, timeLineInfo->first);

            /* Found athlete - DO NOT SKIP */
            return 0;
         }
      }
   }

   if (i == MAX_ATHLETES)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not ignore - too many athletes!!!\n");
   }

   return 1;
}

#if 0
void ColumnStore(CLI_PARSE_INFO *pInfo, char *str)
{
   int i, j;
   int idx = 0;
   int count = 0;
   int endCount = 0;
   char *ptr;

   RemoveTrailingAdd2Inline(str);

   if (partyControl.debug == 1)
   {
      (pInfo->print_fp)("CC: %s\n", str);
   }

   for (i = 0; i < MAX_MY_COLUMNS; i++)
   {
      if (columns[i].width == -1)
      {
         break;
      }

      if (partyControl.debug == 1)
      {
         (pInfo->print_fp)("CF1: %2d: L = %2d    %s\n", i, columns[i].width, str);
      }

   }

   if (i == MAX_MY_COLUMNS)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Too many columns!!! max = %d\n", MAX_MY_COLUMNS);
      return;
   }

#if 0
   if ((str[0] == ' ') || (str[0] == '\t') || (str[0] == '\n') || (str[0] == '\r'))
   {
      return;
   }
#endif

   for (i = 0; i < MAX_ENTRIES; i++)
   {
      if (printTable[i].enabled == 0)
      {
         sprintf(printTable[i].entry, "%s", str);
         printTable[i].enabled = 1;
         break;
      }
   }

   /* Find column lengths */
   ptr = &str[0];
   i = 0;
   while (*ptr != '\0')
   {
      while ((*ptr != ' ') && (*ptr != '\0'))
      {
         count += 1;
         ptr++;
      }

      if (*ptr == '\0')
      {
         if (count > columns[i].width)
         {
            columns[i].width = count;
         }

         if (partyControl.debug == 1)
         {
            (pInfo->print_fp)("\n\n");
         }

         for (j = 0; j < MAX_MY_COLUMNS; j++)
         {
            if (columns[j].width == -1)
            {
               break;
            }

            if (partyControl.debug == 1)
            {
               (pInfo->print_fp)("CF2: %2d: L = %2d    %s\n", j, columns[j].width, str);
            }

         }

         return;
      }

      count += 1;
      ptr++;

      if (*ptr == ' ')
      {
         /* FOUND two spaces! */
         if (count > columns[i].width)
         {

            if (count <= 0)
            {
               (pInfo->print_fp)("INTERNAL ERROR!!!!!!! count = %d\n", count);
            }

            columns[i].width = count;

            if (partyControl.debug == 1)
            {
               (pInfo->print_fp)("CD1: %2d: L = %2d    %s\n", i, columns[i].width, ptr);
            }
         }

         /* Get past one or more spaces */
         endCount = count;
         while ((*ptr == ' ') && (*ptr != '\0'))
         {
            count += 1;
            ptr++;
         }

         if (*ptr == '\0')
         {
            if (count <= 0)
            {
               (pInfo->print_fp)("INTERNAL ERROR!!!!!!! count = %d\n", count);
            }

            if (endCount > columns[i].width)
            {
               columns[i].width = endCount;

               if (partyControl.debug == 1)
               {
                  (pInfo->print_fp)("CD2: %2d: L = %2d    %s\n", i, columns[i].width, ptr);
               }
            }

            for (j = 0; j < MAX_MY_COLUMNS; j++)
            {
               if (columns[j].width == -1)
               {
                  break;
               }

               if (partyControl.debug == 1)
               {
                  (pInfo->print_fp)("CF3: %2d: L = %2d    %s\n", j, columns[j].width, str);
               }

            }

            return;
         }

         /* Start over - not at a column delimiter quite yet */
         count = 1;
         i++;
      }
   }

}

void ColumnPrintf(CLI_PARSE_INFO *pInfo, char *str)
{
   int i;
   int fill = 0;
   int idx = 0;
   char *ptr;

   int columnWidth = 0;
   int endWidth = 0;
   int charIdx = 0;
   int columnId = 0;
   int outIdx = 0;
   int inIdx = 0;
   int space = ' ';
   char outStr[MAX_STRING_SIZE];

   ptr = &str[0];
   i = 0;
   columnId = 0;
   columnWidth = 0;

   /*
    * GLOBAL assumption - if 2nd line, header separator is used - it
    * must have at LEAST two dashes "--"
    */
   if (strstr(ptr, "--") != 0)
   {
      space = '-';
   }

   while (*ptr != '\0')
   {
      if (columnId == 0)
      {
         outStr[outIdx++] = '|';
         outStr[outIdx++] = space;
      }

      idx = 0;
      while ((*ptr != ' ') && (*ptr != '\0'))
      {
         if (space == '-')
         {
            outStr[outIdx++] = space;
         }
         else
         {
            outStr[outIdx++] = *ptr;
         }

         ptr++;
         columnWidth += 1;
      }

      if (*ptr == '\0')
      {
         outStr[outIdx++] = space;
         endWidth = columnWidth;
         break;
      }

      if (space == '-')
      {
         outStr[outIdx++] = space;
      }
      else
      {
         outStr[outIdx++] = *ptr;
      }

      ptr++;
      columnWidth += 1;
      endWidth = columnWidth;

      if (*ptr == ' ')
      {
         ptr++;

         endWidth = columnWidth;
         while ((*ptr == ' ') && (*ptr != '\0'))
         {
            endWidth += 1;
            ptr++;
         }

         if (*ptr == '\0')
         {
            break;
         }

         /* FOUND two spaces! */
         if ((columns[columnId].width + COLUMN_SPACING) > columnWidth)
         {
            fill = ((columns[columnId].width + COLUMN_SPACING) - columnWidth);
            for (i = 0; i < fill; i++)
            {
               outStr[outIdx++] = space;
            }
         }

         if (space == '-')
         {
            outStr[outIdx++] = '+';
         }
         else
         {
            outStr[outIdx++] = '|';
         }

         outStr[outIdx++] = space;

         if (*ptr != '\0')
         {
            columnWidth = 0;
            columnId += 1;
         }
      }
   }

   if (*ptr != '\0')
   {
      (pInfo->print_fp)("INTERNAL ERROR: end of str not found!!!\n");
   }

   if (partyControl.debug == 1)
   {
      (pInfo->print_fp)("columnId = %d, columns[%d].width = %d, endWidth = %d\n", columnId,
                        columnId, columns[columnId].width, endWidth);
   }

   if (columns[columnId].width > COLUMN_SPACING)
   {
      if ((columns[columnId].width) > endWidth)
      {
         fill = (columns[columnId].width - endWidth);
         for (i = 0; i < fill; i++)
         {
            outStr[outIdx++] = space;
         }
      }
   }


#if 1
   outIdx -= 1;
   outStr[outIdx++] = '|';
   outStr[outIdx++] = '\0';
#endif

   (pInfo->print_fp)("%s\n", outStr);

}

void PrintTable(CLI_PARSE_INFO *pInfo)
{
   int i;

   for (i = 0; i < MAX_ENTRIES; i++)
   {
#if 0
      if (printTable[i+1].enabled == 0)
      {
         /* +++owen - crazy, to keep supporting this CONSOLE_OUTPUT, fudge it */
         /* +++owen - remove tailing spaces ? */
      }
      else
#else
      {
         if (printTable[i].enabled == 0)
         {
            break;
         }
      }
#endif

      ColumnPrintf(pInfo, printTable[i].entry);
   }
}
#endif

void RemoveWpkg(CLI_PARSE_INFO *pInfo, char *str)
{
   char *p;
   if ((p = strstr(str, "wkg")) != NULL)
   {
      *p = '\0';
   }
}


void SaveTime(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, char *tmp)
{
   int t;
   char *ptr;
   char tstr[MAX_STRING_SIZE];
   
   strcpy(tstr, tmp);

   sprintf(timeLineInfo->time.str, "%s", tmp);

   if ((ptr = strstr(tstr, ":")) != NULL)
   {
      /* Get minutes */
      *ptr = '\0';
      t = atoi(tstr);

      t *= 60;
      
      /* get seconds */
      ptr++;
      t += atoi(ptr);

      /* Store it as a flot total seconds */
      timeLineInfo->time.f = (float)t;
   }
   else
   {
      sscanf(tmp, "%f", &timeLineInfo->time.f);
   }
}

float GetTime(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, int id)
{
   return(timeLineInfo->race[id].time.f);
}

float FormatTimeCommon(CLI_PARSE_INFO *pInfo, float f, char *out, char *in)
{
   int t;
   char *ptr;
   int min;
   int secs;
   int tenths;
   float tenthsf;
   
   if (((ptr = strstr(in, ":")) != NULL) && ((ptr = strstr(in, ".")) == NULL))
   {
      if (f >= LOWEST_TIME)
      {
         sprintf(out, "0:00");
      }
      else if (f >= 0.0)
      {
         min = (int)(f / 60.0);
         secs = (int)f - (min * 60);
         sprintf(out, "%d:%02d", min, secs);
      }
      else
      {
         f = -1.0 * f;
         min = (int)(f / 60.0);
         secs = (int)f - (min * 60);
         sprintf(out, "-%d:%02d", min, secs);
      }
   }
   else
   {
#if 0
      sprintf(out, "%5.3f", f);
#else
      if (f >= 60.0)
      {
         if (f >= LOWEST_TIME)
         {
            sprintf(out, "0:00");
         }
         else if (f >= 0.0)
         {
//            min = (int)(f / 60.0);
            min = (int)(f / 60.0);
            secs = (int)f - (min * 60);
            tenths = f - (float)((min * 60) + secs);
//         (pInfo->print_fp)("tenths = %f, (%f - ((%d * 60) + %d)\n", tenths, f, min, secs );
            tenths *= 1000;
            sprintf(out, "%d:%02d.%03d", min, secs, (int)tenths);
         }
         else
         {
            f = -1.0 * f;
            min = (int)(f / 60.0);
            secs = (int)f - (min * 60);
            tenths = f - (float)((min * 60) + secs);
//         (pInfo->print_fp)("tenths = %f, (%f - ((%d * 60) + %d)\n", tenths, f, min, secs );
            tenths *= 1000;
            sprintf(out, "-%d:%02d.%03d", min, secs, (int)tenths);
         }
      }
      else
      {
         secs = (int)f;
         tenthsf = f - (float)secs;
         tenthsf *= 1000;
         tenths = (int)tenthsf;

         // sprintf(out, "00:%02d:%3d", secs, tenths);
         // sprintf(out, "%5.3f", f);
         sprintf(out, "%d.%03d", secs, (int)tenths);
      }
#endif
   }
}


float FormatTime(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, int id, char *out)
{
   float f = timeLineInfo->race[id].time.f;

   FormatTimeCommon(pInfo, f, out, timeLineInfo->race[id].time.str);
}

float FormatTime2(CLI_PARSE_INFO *pInfo, float f, char *out, char *in)
{
   FormatTimeCommon(pInfo, f, out, in);
}

float FormatTime3(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, int id, char *out, int clip)
{
   int t;
   char *ptr;
   int min;
   int secs;
   int tenths;
   float tenthsf;
   float f = timeLineInfo->race[id].time.f;

#if 0
   sprintf(out, "%5.3f", f);
#else
   if (f >= 60.0)
   {
      if (f >= LOWEST_TIME)
      {
         sprintf(out, "0:00");
      }
      else if (f >= 0.0)
      {
//            min = (int)(f / 60.0);
         min = (int)(f / 60.0);
         secs = (int)f - (min * 60);
         tenthsf = f - (float)((min * 60) + secs);
//         (pInfo->print_fp)("tenths = %f, (%f - ((%d * 60) + %d)\n", tenths, f, min, secs );
         tenthsf *= 1000;
         tenths = (int)tenthsf;
         if (clip == 0)
         {
            sprintf(out, "%d:%02d.%03d", min, secs, tenths);
         }
         else
         {
            sprintf(out, "%d:%02d", min, secs);
         }
      }
      else
      {
         f = -1.0 * f;
         min = (int)(f / 60.0);
         secs = (int)f - (min * 60);
         tenthsf = f - (float)((min * 60) + secs);
//         (pInfo->print_fp)("tenths = %f, (%f - ((%d * 60) + %d)\n", tenths, f, min, secs );
         tenthsf *= 1000;
         tenths = (int)tenthsf;
         if (clip == 0)
         {
            sprintf(out, "%d:%02d.%03d", min, secs, tenthsf);
         }
         else
         {
            sprintf(out, "%d:%02d", min, secs);
         }
      }
   }
   else
   {
#if 0
      if ((f == LOWEST_TIME) || (f == 0.0))
      {
         secs = 0;
         tenths = 0;
      }
      else
#endif
      {
         secs = (int)f;
         tenthsf = f - (float)secs;
         tenthsf *= 1000;
         tenths = (int)tenthsf;
      }

      // sprintf(out, "00:%02d:%3d", secs, tenths);
      // sprintf(out, "%5.3f", f);
      sprintf(out, "%d.%03d", secs, (int)tenths);
   }
#endif
}

void cmd_party_common_inline(CLI_PARSE_INFO *pInfo, int partyMode, char *infile, char *outfile, char *raceName, int raceId)
{
   int ret;
   char buff[180];
   char time_details1[80];
   int idx;
   int si;
   char *c;
   int len;
   int count = 1;
   int firstAthlete = 1;
   int includeVAM = 0;
   int nlMode = NL_REMOVE;
   int goIntoLoop = 0;
   int partyAlreadyOneTime = 0;
   int continueInLoop = 1;

   FILE *fp_in;
   FILE *fp_out;

   char tmp[MAX_STRING_SIZE];
   char tmp2[MAX_STRING_SIZE];
   char tmp3[MAX_STRING_SIZE];
   char tmpName[MAX_STRING_SIZE];
   char updatedLine[MAX_STRING_SIZE];

   char timeLine[MAX_STRING_SIZE];

   Link_t *ptr, *ptr2;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *timeLineInfo2;
   int groupId = GROUP_ALL;
   int skipRead = 0;

   Link_t *ptrI;
   Link_t *ptrJ;

   TimeLineInfo_t *timeLineInfoI;
   TimeLineInfo_t *timeLineInfoJ;
   float fudge;

   char place[MAX_STRING_SIZE];
   char first[MAX_STRING_SIZE];
   char last[MAX_STRING_SIZE];
   char name[MAX_STRING_SIZE];
   char month[MAX_STRING_SIZE];
   char day[MAX_STRING_SIZE];
   char year[MAX_STRING_SIZE];
   char speed[MAX_STRING_SIZE];
   char watts[MAX_STRING_SIZE];
   char vam[MAX_STRING_SIZE];
   char bpm[MAX_STRING_SIZE];
   char vid[MAX_STRING_SIZE];
   char time[MAX_STRING_SIZE];
   char test[MAX_STRING_SIZE];
   char teamName[MAX_STRING_SIZE];
   char teamName2[MAX_STRING_SIZE];
   char header[MAX_STRING_SIZE];
   char fixed[MAX_STRING_SIZE];
   char fixed2[MAX_STRING_SIZE];
   char groupLine[MAX_STRING_SIZE];

   int outIdx = 0;
   char outStr[MAX_STRING_SIZE];

   ColumnReset(pInfo);

   fp_in = fopen(infile, "r");

   if (!fp_in)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not open %s\n", infile);
      exit(-1);
   }

   fp_out = fopen(outfile, "w");

   if (!fp_out)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not open %s\n", outfile);
      exit(-1);
   }


#ifndef CONSOLE_OUTPUT
   sprintf(header, "%s", "#  name   team   time   watts  wpkg  ");
   ColumnStore(pInfo, header);

   sprintf(header, "%s", "--  ----   ----   ----   -----  ----  ");
   ColumnStore(pInfo, header);
#endif

   idx = 0;
   while (1)
   {
      if (!StringGet(tmp, fp_in, NL_REMOVE))
      {
         /* Done reading file */
         break;
      }

      strcpy(timeLine, tmp);

      if (!FormattedProper(timeLine))
      {
         /* prperly formatted line not found */
         continue;
      }

      goIntoLoop = 0;
      continueInLoop = 1;
      skipRead = 0;
      if ((partyMode & PARTY_KOM_SPRINTS) ||
          (partyMode & PARTY_RESULTS) ||
          (partyMode & PARTY_RESULTS_2))
      {
         if (((timeLine[0] == 'A') || (timeLine[0] == 'B') || (timeLine[0] == 'C') || (timeLine[0] == 'D')) &&
             (((timeLine[1] == ' ') && (timeLine[2] == ' ')) || (timeLine[1] == '\n')) &&
             (strstr(timeLine, "Distance") == NULL))
         {
            sprintf(groupLine, "%s", timeLine);

            if (partyControl.groupId == GROUP_ALL)
            {
               groupId = GROUP_ALL;
            }
            else if (partyControl.groupId == GROUP_GRIT)
            {
               groupId = GROUP_GRIT;
            }
            else if ((timeLine[0] == 'B') || (timeLine[0] == 'C') || (timeLine[0] == 'D'))
            {
               groupId = GROUP_B;
            }
            else if (timeLine[0] == 'A')
            {
               groupId = GROUP_A;
            }
            else
            {
               groupId = GROUP_UNKNOWN;
               (pInfo->print_fp)("INTERNAL ERROR: DID not recognize GroupID\n");
            }

            if (partyControl.lockDown == 0)
            {
               if (((timeLine[0] == 'B') || (timeLine[0] == 'C') || (timeLine[0] == 'D')) &&
                   (partyControl.groupId == GROUP_A))
               {
                  goIntoLoop = 0;
               }
               else if ((timeLine[0] == 'A') &&
                        (partyControl.groupId == GROUP_B))
               {
                  goIntoLoop = 0;
               }
               else
               {
                  goIntoLoop = 1;
               }
            }
            else
            {
               goIntoLoop = 1;
            }
         }
      }
      else if ((partyMode & PARTY_FOLLOWING) || (partyMode & PARTY_FOLLOWING_2))
      {
         // groupId = GROUP_ALL;
         groupId = partyControl.groupId;
         sprintf(groupLine, "%s", timeLine);

         if (strstr(timeLine, "Rank ") != NULL)
         {
            partyAlreadyOneTime = 1;
            goIntoLoop = 1;
            if (strstr(timeLine, "VAM ") != NULL)
            {
               includeVAM = 1;
            }
         }
         else if ((strstr(timeLine, "mi/h") != NULL) && (strstr(timeLine, "W ") != NULL) &&
                  (strstr(timeLine, ", 20") != NULL))
         {
            if (partyAlreadyOneTime != 1)
            {
               (pInfo->print_fp)("INTERNAL ERROR: going back in, but for first time?\n");
            }

            /* We have Already been in ONE TIME, stay in there */
            goIntoLoop = 1;

            /* We already have athlete - SKIP next read as we go on BACK IN */
            skipRead = 1;
            strcpy(tmp, timeLine);
         }
      }
      else
      {
         sprintf(groupLine, "%s", timeLine);
         groupId = GROUP_ALL;
         goIntoLoop = 1;
      }

      if (goIntoLoop == 1)
      {
         while (continueInLoop == 1)
         {
            outIdx = 0;
            if ((partyMode & PARTY_FOLLOWING) || (partyMode & PARTY_FOLLOWING_2))
            {
               nlMode = NL_KEEP;
            }
            else
            {
               nlMode = NL_REMOVE;
            }

            // Athletes name
            if (skipRead == 0)
            {
               if (!StringGet(tmp, fp_in, nlMode))
               {
                  /* Done reading file */
                  break;
               }
            }
            skipRead = 0;

            if (partyMode & PARTY_FOLLOWING)
            {
               if ((tmp[0] == '\n') || (tmp[0] == ' ') || (tmp[0] == '\r') || (tmp[0] == '\t'))
               {
                  continueInLoop = 0;
                  break;
               }

               strcpy(fixed, tmp);

               FixLine(pInfo, fixed2, fixed, teamName);
               FixTeamName(pInfo, teamName2, teamName);

               strcpy(tmp, fixed2);

               if (includeVAM == 1)
               {
                  /* 1    Lance Anderson  Sep 8, 2020     21.2mi/h    170   431W    1,345.5     4:42 */
                  sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s %s",
                         place, first, last, month, day, year, speed, bpm, watts, vid, time);
               }
               else
               {
                  sprintf(vid, "%s", "0");
                  /* 1       Steve Peplinski 	Feb 28, 2019 	46.1mi/h 	169bpm 	736W 	9s */
                  sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s",
                         place, first, last, month, day, year, speed, bpm, watts, time);
               }

               sprintf(name, "%s ", first);
               strcat(name, last);

               timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);
               strcpy(timeLineInfo->timeLine, tmp);

               strcpy(timeLineInfo->last, last);
               strcpy(timeLineInfo->first, first);
               strcpy(timeLineInfo->team, teamName2);
               timeLineInfo->groupId = groupId;

               NameInsert(timeLineInfo, name);

//               if ((partyControl.lockDown == 1) || (partyControl.nickName == 1))
               if (partyControl.lockDown == 1)
               {
//                     (pInfo->print_fp)("%s\n", name);
               }


//             if ((partyControl.lockDown == 1) && (strstr(name, "McGonagle") != NULL))
//             if ((partyControl.lockDown == 1) && ((strstr(name, "Owen") != NULL) || (strstr(tmp, "Owen") != NULL)))
               if ((strstr(name, "Owen") != NULL) || (strstr(tmp, "Owen") != NULL))
               {
                  gdbStop = 1;
               }

//               if ((partyControl.lockDown == 1) || (partyControl.nickName == 1))
               if (partyControl.lockDown == 1)
               {
                  if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                  {
                     continueInLoop = 1;
                     continue;
                  }
               }

               if (raceId != -1)
               {
//                  raceId -= 1;
                  timeLineInfo->race[raceId].place = count;
                  sprintf(timeLineInfo->race[raceId].raceName, "%s", raceName);
               }
               else
               {
                  if (strcmp(raceName, "race1") == 0)
                  {
                     raceId = 0;
                     timeLineInfo->race[0].place = count;
                     sprintf(timeLineInfo->race[0].raceName, "%s", raceName);
                  }
                  else if (strcmp(raceName, "race2") == 0)
                  {
                     raceId = 1;
                     timeLineInfo->race[1].place = count;
                     sprintf(timeLineInfo->race[1].raceName, "%s", raceName);
                  }
                  else if (strcmp(raceName, "race3") == 0)
                  {
                     raceId = 2;
                     timeLineInfo->race[2].place = count;
                     sprintf(timeLineInfo->race[2].raceName, "%s", raceName);
                  }
                  else if (strcmp(raceName, "race4") == 0)
                  {
                     raceId = 3;
                     timeLineInfo->race[3].place = count;
                     sprintf(timeLineInfo->race[3].raceName, "%s", raceName);
                  }
               }

               if ((raceId + 1) > numRacesG)
               {
                  numRacesG = (raceId + 1);
               }

               count++;

               TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

               outIdx += sprintf(&outStr[outIdx], "%s   %s  ", fixed2, teamName2);
               ColumnStore(pInfo, outStr);

#ifdef CONSOLE_OUTPUT
               (pInfo->print_fp)("%s   %s  %s %s %s   %s   %s\n",
                                 place, name, month, day, year, watts, time);
#endif

               fprintf(fp_out, "%s   %s   %s %s %s   %s   %s",
                       place, name, month, day, year, watts, time);
            }
            else
            {
               if (partyMode & PARTY_FOLLOWING_2)
               {
                  if ((tmp[0] == '\n') || (tmp[0] == ' ') || (tmp[0] == '\r') || (tmp[0] == '\t'))
                  {
                     continueInLoop = 0;
                     break;
                  }

                  timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);
                  strcpy(timeLineInfo->timeLine, timeLine);
               }
               else
               {
                  if (strstr(tmp, "Description") != NULL)
                  {
                     /* Ooops... Found the first B or A before athletes */
                     continueInLoop = 0;
                     break;
                  }

                  timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);
                  strcpy(timeLineInfo->timeLine, tmp);

                  // strcpy(timeLineInfo->name, tmp);
                  strcpy(tmpName, tmp);
                  NameInsert(timeLineInfo, tmpName);
                  if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_ALL) || (timeLineInfo->groupId == GROUP_GRIT))
                  {
                     timeLineInfo->groupId = groupId;
                  }
               }


               if (raceId != -1)
               {
//                  raceId -= 1;
                  timeLineInfo->race[raceId].place = count;
                  sprintf(timeLineInfo->race[raceId].raceName, "%s", raceName);
               }
               else
               {
                  if (strcmp(raceName, "race1") == 0)
                  {
                     raceId = 0;
                     timeLineInfo->race[0].place = count;
                     sprintf(timeLineInfo->race[0].raceName, "%s", raceName);
                  }
                  else if (strcmp(raceName, "race2") == 0)
                  {
                     raceId = 1;
                     timeLineInfo->race[1].place = count;
                     sprintf(timeLineInfo->race[1].raceName, "%s", raceName);
                  }
                  else if (strcmp(raceName, "race3") == 0)
                  {
                     raceId = 2;
                     timeLineInfo->race[2].place = count;
                     sprintf(timeLineInfo->race[2].raceName, "%s", raceName);
                  }
                  else if (strcmp(raceName, "race4") == 0)
                  {
                     raceId = 3;
                     timeLineInfo->race[3].place = count;
                     sprintf(timeLineInfo->race[3].raceName, "%s", raceName);
                  }
               }

               if ((raceId + 1) > numRacesG)
               {
                  numRacesG = (raceId + 1);
               }

               if (partyMode & PARTY_FOLLOWING_2)
               {
                  if (includeVAM == 1)
                  {
                     /* 1    Lance Anderson  Sep 8, 2020     21.2mi/h    170   431W    1,345.5     4:42 */
                     sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s %s",
                            place, first, last, month, day, year, speed, bpm, watts, vid, time);

                     sprintf(name, "%s ", first);
                     strcat(name, last);

                     strcpy(timeLineInfo->timeLine, tmp);

                     strcpy(timeLineInfo->last, last);
                     strcpy(timeLineInfo->first, first);
                     NameInsert(timeLineInfo, name);

//                   if ((partyControl.lockDown == 1) || (partyControl.nickName == 1))
                     if (partyControl.lockDown == 1)
                     {
                        if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                        {
                           continueInLoop = 1;
                           continue;
                        }
                     }

                     NameInsert(timeLineInfo, name);

                     if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_ALL) || (timeLineInfo->groupId == GROUP_GRIT))
                     {
                        timeLineInfo->groupId = groupId;
                     }

                     if (RacedToday(name, teamName))
                     {
                        TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

                        sprintf(timeLineInfo->team, "%s", teamName);
                        sprintf(place, "%d ", count);
                        count++;

                        if (count > maxCount)
                        {
                           raceIdxG = raceId;
                           timeLineInfoG = timeLineInfo;
                           maxCount = count;
                        }

                        (pInfo->print_fp)("%s   %s  %s  %s %s %s   %s   %s\n",
                                          place, name, teamName, month, day, year, watts, time);

                        fprintf(fp_out, "%s   %s  %s  %s %s %s   %s   %s\n",
                                place, name, teamName, month, day, year, watts, time);
                     }
                  }
                  else
                  {
                     /* 1       Steve Peplinski 	Feb 28, 2019 	46.1mi/h 	169bpm 	736W 	9s */
                     sprintf(vid, "%s", "0");

                     sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s",
                            place, first, last, month, day, year, speed, bpm, watts, time);

                     sprintf(name, "%s ", first);
                     strcat(name, last);

                     strcpy(timeLineInfo->timeLine, tmp);

                     strcpy(timeLineInfo->last, last);
                     strcpy(timeLineInfo->first, first);
                     NameInsert(timeLineInfo, name);

//                   if ((partyControl.lockDown == 1) || (partyControl.nickName == 1))
                     if (partyControl.lockDown == 1)
                     {
                        if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                        {
                           continueInLoop = 1;
                           continue;
                        }
                     }

                     NameInsert(timeLineInfo, name);
                     // TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

                     if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_ALL) || (timeLineInfo->groupId == GROUP_GRIT))
                     {
                        timeLineInfo->groupId = groupId;
                     }

                     if (RacedToday(name, teamName))
                     {
                        TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

                        sprintf(timeLineInfo->team, "%s", teamName);
                        sprintf(place, "%d", count);
                        count++;

                        if (count > maxCount)
                        {
                           raceIdxG = raceId;
                           timeLineInfoG = timeLineInfo;
                           maxCount = count;
                        }

                        (pInfo->print_fp)("%s   %s  %s  %s %s %s   %s   %s\n",
                                          place, name, teamName, month, day, year, watts, time);

                        fprintf(fp_out, "%s   %s  %s  %s %s %s   %s   %s\n",
                                place, name, teamName, month, day, year, watts, time);
                     }
                  }
               }
               else
               {

                  // Team or empty
                  if (!StringGet(tmp, fp_in, NL_KEEP))
                  {
                     /* Done reading file */
                     continueInLoop = 0;
                     break;
                  }

                  // If team, skip
                  if ((tmp[0] != '\n') && (tmp[0] != ' ') && (tmp[0] != '\r') && (tmp[0] != '\t'))
                  {
                     /* TEAM can be found in the name (Zwift does that) or on next line */
                     TeamNameCleanup(timeLineInfo, tmp);

//                   if ((partyControl.lockDown == 1) || (partyControl.nickName == 1))
                     if (partyControl.lockDown == 1)
                     {
                        if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                        {
                           continueInLoop = 0;
                           break;
                        }
                     }

                     /* CATCH if/when we have DUPLICATE names (i.e., P B) with different teams */
                     NameDouble(pInfo, timeLineInfo);

                     /* We have team name, thus skip next <empty> line */
                     if (!StringGet(tmp, fp_in, NL_KEEP))
                     {
                        /* Done reading file */
                        continueInLoop = 0;
                        break;
                     }
                  }
                  else
                  {
                     /* CATCH if/when we have DUPLICATE names (i.e., P B) with different teams */
                     NameDouble(pInfo, timeLineInfo);

//                   if ((partyControl.lockDown == 1) || (partyControl.nickName == 1))
                     if (partyControl.lockDown == 1)
                     {
                        if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                        {
                           continueInLoop = 0;
                           break;
                        }
                     }
                  }

                  /* TEAM can be found in the name (Zwift does that) or on next line */
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("%3d  %s  ", count, timeLineInfo->name);
#else
                  outIdx += sprintf(&outStr[outIdx], "%d  %s  ", count, timeLineInfo->name);
#endif
                  fprintf(fp_out, "%3d  %s  ", count, timeLineInfo->name);
                  count++;

                  if (count > maxCount)
                  {
                     raceIdxG = raceId;
                     timeLineInfoG = timeLineInfo;
                     maxCount = count;
                  }

                  /* TEAM can be found in the name (Zwift does that) or on next line */
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("%s  ", timeLineInfo->team);
#else
                  outIdx += sprintf(&outStr[outIdx], "%s  ", timeLineInfo->team);
#endif
                  fprintf(fp_out, "%s  ", timeLineInfo->team);

                  /* time */
                  if (!StringGet(tmp, fp_in, nlMode))
                  {
                     /* Done reading file */
                     continueInLoop = 0;
                     break;
                  }

                  /* SAVE time */
                  SaveTime(pInfo, timeLineInfo, tmp);
                  TimeLineInfoUpdate(pInfo, timeLineInfo, listTimeLine, raceName);

                  TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

                  AddSpace(tmp2, tmp);
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("%s", tmp2);
#else
                  outIdx += sprintf(&outStr[outIdx], "%s  ", tmp2);
#endif
                  fprintf(fp_out, "%s", tmp2);

                  if ((partyMode & PARTY_RESULTS) || (partyMode & PARTY_RESULTS_2))
                  {
                     /* If first athlete we DO NOT have a '+' difference */
                     if (firstAthlete != 1)
                     {
                        /* + */
                        if (!StringGet(tmp, fp_in, NL_REMOVE))
                        {
                           /* Done reading file */
                           continueInLoop = 0;
                           break;
                        }

                     }
                     firstAthlete = 0;
                  }

                  /* watts */
#ifdef CONSOLE_OUTPUT
                  if (!StringGet(tmp, fp_in, NL_KEEP))
#else
                  if (!StringGet(tmp, fp_in, nlMode))
#endif
                  {
                     /* Done reading file */
                     continueInLoop = 0;
                     break;
                  }

                  // AddSpace(tmp2, tmp);
                  RemoveTrailing(tmp2, tmp);
                  sprintf(timeLineInfo->wattLine, "%s", tmp2);;
                  strcpy(timeLineInfo->groupLine, groupLine);
                  RemoveWpkg(pInfo, tmp2);

#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("%s\n", tmp2);
                  if (partyMode & PARTY_RESULTS_2)
                  {
                     fprintf(fp_out, "%s", tmp2);
                  }
                  else
                  {
                     fprintf(fp_out, "%s\n", tmp2);
                  }
#else
                  outIdx += sprintf(&outStr[outIdx], "%s  ", tmp2);
                  ColumnStore(pInfo, outStr);
                  fprintf(fp_out, "%s\n", tmp2);
#endif

                  if (!(partyMode & PARTY_RESULTS_2))
                  {
                     /* TOTAL watts? */
                     if (!StringGet(tmp, fp_in, NL_KEEP))
                     {
                        /* Done reading file */
                        continueInLoop = 0;
                        break;
                     }

                     /* extended stats */
                     /* B <original place> */
                     if (!StringGet(tmp, fp_in, NL_KEEP))
                     {
                        /* Done reading file */
                        continueInLoop = 0;
                        break;
                     }
                  }

                  sprintf(groupLine, "%s", tmp);

                  // Break if we see garbage
                  if ((tmp[0] != 'A') && (tmp[0] != 'B') && (tmp[0] != 'C') && (tmp[0] != 'D'))
                  {
                     continueInLoop = 0;
                     break;
                  }

                  if (partyControl.lockDown == 0)
                  {
                     if (((tmp[0] == 'B') || (tmp[0] == 'C') || (tmp[0] == 'D')) &&
                         (partyControl.groupId == GROUP_A))
                     {
                        continueInLoop = 0;
                        break;
                     }
                     else if ((tmp[0] == 'A') &&
                              (partyControl.groupId == GROUP_B))
                     {
                        continueInLoop = 0;
                        break;
                     }
                  }

               }
            }
         }
      }
   }

   fclose(fp_in);

#ifndef CONSOLE_OUTPUT
   (pInfo->print_fp)("\n");
#endif

   PrintTable(pInfo);

   AddUpPoints(pInfo, count, raceId);

   if (partyMode & PARTY_FOLLOWING_2)
   {
      /* Did we find everyone? */
      RidersMissed(pInfo);
   }

#ifdef CONSOLE_OUTPUT
   (pInfo->print_fp)("\n\n");
#else
//   outIdx = sprintf(&outStr[outIdx], "%s", "\n\n");
//   (pInfo->print_fp)("%s", outStr);
   (pInfo->print_fp)("\n\n");
#endif

}

void cmd_party_common(CLI_PARSE_INFO *pInfo, int partyMode)
{
   char infile[MAX_STRING_SIZE];
   char outfile[MAX_STRING_SIZE];
   char raceName[MAX_STRING_SIZE];

   int raceId = -1;

   sprintf(infile,  "%s", "/home/omcgonag/Work/partytime/database/_posts/2020/12/04/Jungle-Circuit-On-A-MTB_results_zwift.txt");
   sprintf(outfile, "%s", "/home/omcgonag/Work/partytime/database/_posts/2020/12/04/Jungle-Circuit-On-A-MTB_results_partytime.txt");
   sprintf(raceName, "%s", "hooha");

   if ( pInfo->argc > 1)
   {
      sprintf(infile, "%s", pInfo->argv[1]);
   }

   if ( pInfo->argc > 2)
   {
      sprintf(outfile, "%s", pInfo->argv[2]);
   }

   if ( pInfo->argc > 3)
   {
      sprintf(raceName, "%s", pInfo->argv[3]);
   }

   if ( pInfo->argc > 4)
   {
      sscanf(pInfo->argv[4], "%d", &raceId);
   }

   cmd_party_common_inline(pInfo, partyMode, infile, outfile, raceName, raceId);

}


void cmd_party_kom_and_sprints(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, PARTY_KOM_SPRINTS);
}

void cmd_party_results(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, PARTY_RESULTS);
}

void cmd_party_results2(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, PARTY_RESULTS_2);
}

void cmd_party_following(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, PARTY_FOLLOWING);
}

void cmd_party_following2(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, PARTY_FOLLOWING_2);
}

int BestOfPoints(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, int count, int *total, int mode, PRIdx_t *prIdx)
{
   int i, j;
   int ret;
   int numRaces = 0;
   int finalHighest = 0;
   int currPoints = 0;
   int currHighest = 0;
   int highestIdx = 0;
   int orderIdx = 0;
   int mostRecentIdx = 0;

   for (i = 0; i < numRacesG; i++)
   {
      timeLineInfo->race[i].done = 0;
      if (timeLineInfo->race[i].place > 0)
      {
         numRaces += 1;
         mostRecentIdx = i;
      }
   }

   highestIdx = 0;
   while (1)
   {
      highestIdx = 0;
      currHighest = 0;

      for (i = 0; i < numRacesG; i++)
      {
         for (j = 0; j < numRacesG; j++)
         {
            if ((timeLineInfo->race[j].done == 0) &&
                (timeLineInfo->race[j].place > 0))
            {
               currPoints = timeLineInfo->race[j].points;
               if (currPoints >= currHighest)
               {
                  currHighest = currPoints;
                  highestIdx = j;
               }
            }
         }
      }

      if (currHighest == 0)
      {
         break;
      }

      timeLineInfo->race[highestIdx].done = 1;
      timeLineInfo->raceOrder[orderIdx].pos = highestIdx;
      orderIdx += 1;
   }

   for (i = 0; i < count; i++)
   {
      orderIdx = timeLineInfo->raceOrder[i].pos;
      ret += timeLineInfo->race[orderIdx].points;
   }

   *total = numRaces;

   return (ret);
}

float BestOfTime(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, int count, int *total, int mode, PRIdx_t *prIdx)
{
   int i, j;
   float ret;
   int numRaces = 0;
   int finalHighest = 0;
   int currPoints = 0;
   int lowestIdx = 0;
   int orderIdx = 0;
   int orderIdx2 = 0;
   int validNum = 0;
   int mostRecentIdx = 0;

   float currTime = 0.0;
   float currLowestTime = LOWEST_TIME;

   for (i = 0; i < numRacesG; i++)
   {
      timeLineInfo->race[i].done = 0;
      if (timeLineInfo->race[i].place > 0)
      {
         numRaces += 1;
         mostRecentIdx = i;
      }
   }

#if 0
   if ((strstr(timeLineInfo->name, "McGonagle") != 0) || (strstr(timeLineInfo->name, "Kaldbar") != 0))
   {
      (pInfo->print_fp)("\n\n");
      for (i = 0; i < numRacesG; i++)
      {
         (pInfo->print_fp)("INFO: %s:\n", timeLineInfo->name);
         for (j = 0; j < numRacesG; j++)
         {
            if ((timeLineInfo->race[j].done == 0) &&
                (timeLineInfo->race[j].place > 0))
            {
               // (pInfo->print_fp)("INFO: %d:  %d:  %4.4f\n", j, timeLineInfo->race[j].place, timeLineInfo->race[j].time.f);
               (pInfo->print_fp)("INFO: %d:  %d:  %s\n", j, timeLineInfo->race[j].place, timeLineInfo->race[j].time.str);
            }
         }
         break;
      }
      (pInfo->print_fp)("\n\n");
   }
#endif

   if (mode == SHOW_RICHMOND)
   {
      currLowestTime = 0.0;
      for (i = 0; i < numRacesG; i++)
      {
         currTime = GetTime(pInfo, timeLineInfo, i);
         currLowestTime += currTime;
      }

      timeLineInfo->race[i].done = 1;
      timeLineInfo->race[i].time.f = currLowestTime;
      sprintf(timeLineInfo->race[i].time.str, "%5.3f", timeLineInfo->race[i].time.f);

      return(timeLineInfo->race[i].time.f);
   }
   else
   {

      lowestIdx = 0;
      while (1)
      {
         lowestIdx = 0;
         currLowestTime = LOWEST_TIME;

         for (i = 0; i < numRacesG; i++)
         {
            for (j = 0; j < numRacesG; j++)
            {
               if ((timeLineInfo->race[j].done == 0) &&
                   (timeLineInfo->race[j].place > 0))
               {
                  if (mode == SHOW_TEMPOISH)
                  {
//                  currTime = timeLineInfo->race[j].time.f;
                     currTime = GetTime(pInfo, timeLineInfo, j);
                     if (currTime <= currLowestTime)
                     {
                        currLowestTime = currTime;
                        lowestIdx = j;
                     }
                  }
                  else if (mode == SHOW_RICHMOND)
                  {
//                  currTime = timeLineInfo->race[j].time.f;
                     currTime = GetTime(pInfo, timeLineInfo, j);
                     if (currTime <= currLowestTime)
                     {
                        currLowestTime = currTime;
                        lowestIdx = j;
                     }
                  }
               }
            }
         }

         if (currLowestTime == LOWEST_TIME)
         {
            break;
         }

         timeLineInfo->race[lowestIdx].done = 1;
         timeLineInfo->raceOrder[orderIdx].pos = lowestIdx;
         orderIdx += 1;
      }
   }

   prIdx->lastFound = 0;
   if ((numRaces > 1) && (mostRecentIdx != 0) && (numRacesG != 1) && (mostRecentIdx == (numRacesG - 1)))
   {
      prIdx->lastFound = 1;
   }

#if 0
   if ((strstr(timeLineInfo->name, "McGonagle") != 0) || (strstr(timeLineInfo->name, "Kaldbar") != 0))
   {
            (pInfo->print_fp)("INFO0: %s: numRaces=%d  numRacesG=%d  mostRecentIdx=%d order[0].pos=%d found=%d\n\n", 
                              timeLineInfo->name, numRaces, numRacesG, mostRecentIdx, timeLineInfo->raceOrder[0].pos, prIdx->lastFound);
   }
#endif
   
   if (prIdx->lastFound == 1)
   {
      /* If PR idx matches the most recent race, we have a NEW PR !!! */
      // if (timeLineInfo->raceOrder[0].pos == mostRecentIdx)
      if (timeLineInfo->raceOrder[0].pos == 0)
      {
#if 0
         if ((strstr(timeLineInfo->name, "McGonagle") != 0) || (strstr(timeLineInfo->name, "Kaldbar") != 0))
         {
            (pInfo->print_fp)("INFO2: %s:\n", timeLineInfo->name);
            (pInfo->print_fp)("INFO2: %d:  %d:  %s\n", 0, 
                              timeLineInfo->raceOrder[0].pos, 
                              timeLineInfo->race[timeLineInfo->raceOrder[0].pos].time.str);
            (pInfo->print_fp)("INFO2: %d:  %d:  %s\n", 1, 
                              timeLineInfo->raceOrder[1].pos, 
                              timeLineInfo->race[timeLineInfo->raceOrder[1].pos].time.str);
         }
#endif
         prIdx->idx = timeLineInfo->raceOrder[0].pos;

         /* Subtract the two positions to get improvement */
         orderIdx = timeLineInfo->raceOrder[0].pos;
         orderIdx2 = timeLineInfo->raceOrder[1].pos;
//         prIdx->improvement = timeLineInfo->race[orderIdx2].time.f - timeLineInfo->race[orderIdx].time.f;
         prIdx->improvement = GetTime(pInfo, timeLineInfo, orderIdx2) - GetTime(pInfo, timeLineInfo, orderIdx);
      }
      else
      {
         prIdx->idx = -1;
         orderIdx = timeLineInfo->raceOrder[0].pos;
//         prIdx->improvement = timeLineInfo->race[mostRecentIdx].time.f - timeLineInfo->race[orderIdx].time.f;
         prIdx->improvement = GetTime(pInfo, timeLineInfo, mostRecentIdx) - GetTime(pInfo, timeLineInfo, orderIdx);
      }
   }

   ret = 0.0;
   validNum = 0;
   for (i = 0; i < count; i++)
   {
      orderIdx = timeLineInfo->raceOrder[i].pos;
//      if ((int)timeLineInfo->race[orderIdx].time.f != 0)
      if ((int)GetTime(pInfo, timeLineInfo, orderIdx) != 0)
      {
         validNum += 1;
//         ret += timeLineInfo->race[orderIdx].time.f;
         ret += GetTime(pInfo, timeLineInfo, orderIdx);
      }
   }

   /* Average it */
   ret = (ret / (float)validNum);

   *total = numRaces;

   return (ret);
}


void cmd_party_show_common(CLI_PARSE_INFO *pInfo, int mode)
{
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *currInfo;
   Link_t *link;
   Link_t *ptr;
   int i;
   int len;
   float av;
   int orderIdx1;
   char placePoints[MAX_STRING_SIZE];
   char outStr[MAX_STRING_SIZE];
   char header[MAX_STRING_SIZE];
   char nameString[MAX_STRING_SIZE];
   char timeString[MAX_STRING_SIZE];
   char timeString2[MAX_STRING_SIZE];
   char timeString3[MAX_STRING_SIZE];
   char timeString4[MAX_STRING_SIZE];
   PRIdx_t prIdx;
   float improvement;
   int nameFound[MAX_RACES];
   int totalFound = 0;

   int outIdx = 0;
   int orderIdx = 0;
   int count = 1;

   ColumnReset(pInfo);


   if ((mode == SHOW_POINTS) || (mode == SHOW_TALLY) || (mode == SHOW_TIME))
   {
      ptr = (Link_t *)listTimeLine->head;
      while (ptr->next != NULL)
      {
         timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

         if (partyControl.nickName == 1)
         {
            if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, 0))
            {
               ptr = ptr->next;
               continue;
            }
         }

         currInfo = TimeLineInfoNew(pInfo, TYPE_USBC);

         currInfo->points = BestOfPoints(pInfo, timeLineInfo, partyControl.bestOf, &timeLineInfo->totalRaces, SHOW_POINTS, &timeLineInfo->prIdx);
         currInfo->av = BestOfTime(pInfo, timeLineInfo, partyControl.bestOf, &timeLineInfo->totalRaces, SHOW_TEMPOISH, &timeLineInfo->prIdx);
         currInfo->totalTime = BestOfTime(pInfo, timeLineInfo, partyControl.bestOf, &timeLineInfo->totalRaces, SHOW_RICHMOND, &timeLineInfo->prIdx);
         currInfo->avOrig = currInfo->av;

         if (partyControl.ageBonus == 1)
         {
            if (strstr(timeLineInfo->name, "McGonagle") != 0)
            {
               currInfo->points += 5;
            }
         }

         /* Fill in currInfo */
         currInfo->prIdx = timeLineInfo->prIdx;
         for (i = 0; i < numRacesG; i++)
         {
            currInfo->raceOrder[i].pos = timeLineInfo->raceOrder[i].pos;
            currInfo->race[i].time = timeLineInfo->race[i].time;
         }

         currInfo->me = timeLineInfo;
         currInfo->totalRaces = timeLineInfo->totalRaces;

         if (partyControl.bonus == 1)
         {
            if (timeLineInfo->totalRaces >= 4)
            {
               currInfo->points += 10;
            }
            else if (timeLineInfo->totalRaces >= 3)
            {
               currInfo->points += 5;
            }
         }

         if ((mode == SHOW_POINTS) || (mode == SHOW_TALLY))
         {
            TimeOrderInfoInsert(pInfo, currInfo, listTimeOrder, RACE_NO, TIME_ORDER_POINTS);
         }
         else
         {
            TimeOrderInfoInsert(pInfo, currInfo, listTimeOrder, RACE_NO, TIME_ORDER_TIME);
         }

         ptr = ptr->next;
      }
   }
   else if (mode == SHOW_TEMPOISH)
   {
      ptr = (Link_t *)listTimeLine->head;
      while (ptr->next != NULL)
      {
         timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

         if (partyControl.nickName == 1)
         {
            if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, 0))
            {
               ptr = ptr->next;
               continue;
            }
         }

         currInfo = TimeLineInfoNew(pInfo, TYPE_USBC);

         currInfo->av = BestOfTime(pInfo, timeLineInfo, partyControl.bestOf, &timeLineInfo->totalRaces, SHOW_TEMPOISH, &timeLineInfo->prIdx);
         currInfo->totalTime = BestOfTime(pInfo, timeLineInfo, partyControl.bestOf, &timeLineInfo->totalRaces, SHOW_RICHMOND, &timeLineInfo->prIdx);
         currInfo->avOrig = currInfo->av;

         if ((partyControl.clip == 1) && (partyControl.cat != 0))
         {
            if (partyControl.cat == 1)
            {
               if (currInfo->av >= partyControl.cat1)
               {
                  ptr = ptr->next;
                  continue;
               }
            }
            else if (partyControl.cat == 2)
            {
               if ((currInfo->av < partyControl.cat1) ||
                   (currInfo->av >= partyControl.cat2))
               {
                  ptr = ptr->next;
                  continue;
               }
            }
            else if (partyControl.cat == 3)
            {
               if (currInfo->av < partyControl.cat2)
               {
                  ptr = ptr->next;
                  continue;
               }
            }
         }

         if (partyControl.pr == 1) 
         {
#if 0
            timeLineInfo->prIdx.lastFound = 0;
#else
            if (timeLineInfo->prIdx.lastFound == 1)
            {
               if (timeLineInfo->prIdx.idx == -1)
               {
                  currInfo->av += (timeLineInfo->prIdx.improvement * (float)partyControl.adjust);
               }
               else
               {
                  currInfo->av -= (timeLineInfo->prIdx.improvement * (float)partyControl.adjust);
               }
            }
#endif
         }
         
         /* Fill in currInfo */
         for (i = 0; i < numRacesG; i++)
         {
            currInfo->raceOrder[i].pos = timeLineInfo->raceOrder[i].pos;
            currInfo->race[i].time = timeLineInfo->race[i].time;
         }

         currInfo->prIdx = timeLineInfo->prIdx;

         if (partyControl.ageBonus == 1)
         {
            if (strstr(timeLineInfo->name, "McGonagle") != 0)
            {
               currInfo->points += 5;
            }
         }

         currInfo->me = timeLineInfo;
         currInfo->totalRaces = timeLineInfo->totalRaces;

         TimeOrderInfoInsert(pInfo, currInfo, listTimeOrder, RACE_NO, TIME_ORDER_TEMPOISH);
         ptr = ptr->next;
      }
   }
   else if (mode == SHOW_RICHMOND)
   {
      ptr = (Link_t *)listTimeLine->head;
      while (ptr->next != NULL)
      {
         timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

         if (partyControl.nickName == 1)
         {
            if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, 0))
            {
               ptr = ptr->next;
               continue;
            }
         }

         currInfo = TimeLineInfoNew(pInfo, TYPE_USBC);

         currInfo->av = BestOfTime(pInfo, timeLineInfo, partyControl.bestOf, &timeLineInfo->totalRaces, SHOW_TEMPOISH, &timeLineInfo->prIdx);
         currInfo->totalTime = BestOfTime(pInfo, timeLineInfo, partyControl.bestOf, &timeLineInfo->totalRaces, SHOW_RICHMOND, &timeLineInfo->prIdx);
         currInfo->avOrig = currInfo->av;

         /* Fill in currInfo */
         for (i = 0; i < (numRacesG + 1); i++)
         {
            currInfo->raceOrder[i].pos = timeLineInfo->raceOrder[i].pos;
            currInfo->race[i].time = timeLineInfo->race[i].time;
         }

         currInfo->prIdx = timeLineInfo->prIdx;

         if (partyControl.ageBonus == 1)
         {
            if (strstr(timeLineInfo->name, "McGonagle") != 0)
            {
               currInfo->points += 5;
            }
         }

         currInfo->me = timeLineInfo;
         currInfo->totalRaces = timeLineInfo->totalRaces;

         TimeOrderInfoInsert(pInfo, currInfo, listTimeOrder, RACE_NO, TIME_ORDER_RICHMOND);
         ptr = ptr->next;
      }
   }

   if (mode == SHOW_POINTS)
   {
// +++owen - spend a little bit to debug - so that course names show up across columns!!!
#if 0 /* ndef CONSOLE_OUTPUT */
      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "#  name   team  ");

      ptr = (Link_t *)listTimeLine->head;
      while (ptr->next != NULL)
      {
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

         for (i = 0; i < numRacesG; i++)
         {
            if (currInfo->race[i].raceName[0] != '\0')
            {
               totalFound += 1;
            }
         }

         if (totalFound == numRacesG)
         {
            /* Save it up - since it will be in order that was loaded */
            for (i = 0; i < numRacesG; i++)
            {
               outIdx += sprintf(&header[outIdx], "%s  ", currInfo->race[i].raceName);
            }
            break;
         }

         ptr = ptr->next;
      }

      outIdx += sprintf(&header[outIdx], "best-of-%d  ", partyControl.bestOf);
      ColumnStore(pInfo, header);

      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "--  ----   ----  ");
      for (i = 0; i < numRacesG; i++)
      {
         outIdx += sprintf(&header[outIdx], "%s", "-----  ");
      }

      outIdx += sprintf(&header[outIdx], "%s", "-----  ");

#if 0
      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "#  name   team   race1  ");
      for (i = 1; i < numRacesG; i++)
      {
         outIdx += sprintf(&header[outIdx], "race%d  ", (i+1));
      }
      outIdx += sprintf(&header[outIdx], "%s", "plc/pts  ");
      ColumnStore(pInfo, header);

      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "-  ----   ----   -----  ");
      for (i = 1; i <= numRacesG; i++)
      {
         outIdx += sprintf(&header[outIdx], "%s", "------  ");
      }
#endif
      ColumnStore(pInfo, header);
#endif


#ifndef CONSOLE_OUTPUT
      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "#  name   team   race1  ");
      for (i = 1; i < numRacesG; i++)
      {
         outIdx += sprintf(&header[outIdx], "race%d  ", (i+1));
      }
      outIdx += sprintf(&header[outIdx], "%s", "plc/pts  ");
      ColumnStore(pInfo, header);

      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "-  ----   ----   -----  ");
      for (i = 1; i <= numRacesG; i++)
      {
         outIdx += sprintf(&header[outIdx], "%s", "------  ");
      }
      ColumnStore(pInfo, header);
#endif

      ptr = (Link_t *)listTimeOrder->head;
      while (ptr->next != NULL)
      {
         outIdx = 0;
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

#if 0
         if (partyControl.nickName == 1)
         {
            if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, 0))
            {
               ptr = ptr->next;
               continue;
            }
         }
#endif
         
         timeLineInfo = (TimeLineInfo_t *)currInfo->me;

         if ((partyControl.bestOf > 1) && (partyControl.clip == 1) && (timeLineInfo->numRaces < partyControl.bestOf))
         {
            ptr = ptr->next;
            continue;
         }

         if (timeLineInfo->team[0] != '\0')
         {
#ifdef CONSOLE_OUTPUT
            (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, timeLineInfo->team);
#else
            outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, timeLineInfo->name, timeLineInfo->team);
#endif
         }
         else
         {
#ifdef CONSOLE_OUTPUT
            (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#else
            outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#endif
         }
         count++;

         for (i = 0; i < numRacesG; i++)
         {
            if (timeLineInfo->race[i].place != -1)
            {
               sprintf(placePoints, "%2d/%-2d  ", timeLineInfo->race[i].place, timeLineInfo->race[i].points);
#ifdef CONSOLE_OUTPUT
               (pInfo->print_fp)(" %-10s ", placePoints);
#else
               outIdx += sprintf(&outStr[outIdx], " %-10s  ", placePoints);
#endif
            }
            else
            {
               sprintf(placePoints, "%2d/%-2d  ", 0, 0);
#ifdef CONSOLE_OUTPUT
               (pInfo->print_fp)(" %-10s ", placePoints);
#else
               outIdx += sprintf(&outStr[outIdx], " %-10s  ", placePoints);
#endif
            }
         }


#ifdef CONSOLE_OUTPUT
         (pInfo->print_fp)(" %-10d  ", currInfo->points);
         (pInfo->print_fp)("\n");
#else
         outIdx += sprintf(&outStr[outIdx], " %10d  ", currInfo->points);
         ColumnStore(pInfo, outStr);
#endif
         ptr = ptr->next;
      }
   }
   else if (mode == SHOW_TALLY)
   {

#ifndef CONSOLE_OUTPUT
      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "#  name   team   #    plc/pts  average");
      ColumnStore(pInfo, header);

      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "-  ----   ----   ---  -----    -------");
      ColumnStore(pInfo, header);
#endif

      count = 1;
      ptr = (Link_t *)listTimeOrder->head;
      while (ptr->next != NULL)
      {
         outIdx = 0;
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

         if (currInfo->totalRaces >= partyControl.tallyMin)
         {
            timeLineInfo = (TimeLineInfo_t *)currInfo->me;

            if ((partyControl.bestOf > 1) && (partyControl.clip == 1) && (timeLineInfo->numRaces < partyControl.bestOf))
            {
               ptr = ptr->next;
               continue;
            }

            nameString[0] = '*';
            nameString[1] = '*';
            strcpy(&nameString[2], timeLineInfo->name);

            len = strlen(nameString);

            nameString[len] = '*';
            nameString[len+1] = '*';
            nameString[len+2] = '\0';

            if (timeLineInfo->team[0] != '\0')
            {
               outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, nameString, timeLineInfo->team);
            }
            else
            {
               outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, nameString, " ");
            }
            count++;

            av = (float)currInfo->points / (float)currInfo->totalRaces;

            outIdx += sprintf(&outStr[outIdx], " %-5d  %-10d   %5.3f", currInfo->totalRaces, currInfo->points, av);
            ColumnStore(pInfo, outStr);
         }

         ptr = ptr->next;
      }
   }
   else if (mode == SHOW_TIME)
   {

#ifndef CONSOLE_OUTPUT
      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "#  name   team   time  pts");
      ColumnStore(pInfo, header);

      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "-  ----   ----   ----  ---");
      ColumnStore(pInfo, header);
#endif

      count = 1;
      ptr = (Link_t *)listTimeOrder->head;
      while (ptr->next != NULL)
      {
         outIdx = 0;
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

         timeLineInfo = (TimeLineInfo_t *)currInfo->me;

#if 0
         if (partyControl.nickName == 1)
         {
            if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, 0))
            {
               ptr = ptr->next;
               continue;
            }
         }
#endif
         
         if ((partyControl.bestOf > 1) && (partyControl.clip == 1) && (timeLineInfo->numRaces < partyControl.bestOf))
         {
            ptr = ptr->next;
            continue;
         }

         if ((partyControl.bestOf > 1) && (partyControl.clip == 1) && (partyControl.cat != 0))
         {
            orderIdx1 = (numRacesG - 1);
            if (partyControl.cat == 1)
            {
               if (timeLineInfo->race[orderIdx1].time.f >= partyControl.cat1)
               {
                  ptr = ptr->next;
                  continue;
               }
            }
            else if (partyControl.cat == 2)
            {
               if ((timeLineInfo->race[orderIdx1].time.f < partyControl.cat1) ||
                   (timeLineInfo->race[orderIdx1].time.f >= partyControl.cat2))
               {
                  ptr = ptr->next;
                  continue;
               }
            }
            else if (partyControl.cat == 3)
            {
               if (timeLineInfo->race[orderIdx1].time.f < partyControl.cat2)
               {
                  ptr = ptr->next;
                  continue;
               }
            }
         }

#if 1
         strcpy(nameString, timeLineInfo->name);
#else
         nameString[0] = '*';
         nameString[1] = '*';
         strcpy(&nameString[2], timeLineInfo->name);

         len = strlen(nameString);

         nameString[len] = '*';
         nameString[len+1] = '*';
         nameString[len+2] = '\0';
#endif

         if (timeLineInfo->team[0] != '\0')
         {
            outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, nameString, timeLineInfo->team);
         }
         else
         {
            outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, nameString, " ");
         }
         count++;

         orderIdx1 = (numRacesG - 1);
         outIdx += sprintf(&outStr[outIdx], " %s  %d", timeLineInfo->race[orderIdx1].time.str, currInfo->points);

         ColumnStore(pInfo, outStr);

         ptr = ptr->next;
      }
   }
   else if (mode == SHOW_TEMPOISH)
   {
#ifndef CONSOLE_OUTPUT
      if ((partyControl.pr == 0) && (partyControl.calendar == 0))
      {
         outIdx = 0;
         outIdx += sprintf(&header[outIdx], "%s", "#  name   team   PR  ");
         for (i = 1; i < numRacesG; i++)
         {
            if (i == 1)
            {
               outIdx += sprintf(&header[outIdx], "2nd  ", (i+1));
            }
            else if (i == 2)
            {
               outIdx += sprintf(&header[outIdx], "3rd  ", (i+1));
            }
            else
            {
               outIdx += sprintf(&header[outIdx], "%dth  ", (i+1));
            }
         }

         if (partyControl.bestOf == 1)
         {
            outIdx += sprintf(&header[outIdx], "%s", "PR");
         }
         else
         {
            outIdx += sprintf(&header[outIdx], "Best %d av.  ", partyControl.bestOf);
         }

         ColumnStore(pInfo, header);

         outIdx = 0;
         outIdx += sprintf(&header[outIdx], "%s", "-  ----   ----   -----  ");
         for (i = 1; i < numRacesG; i++)
         {
            outIdx += sprintf(&header[outIdx], "%s", "------  ");
         }

         /* improvement */
         outIdx += sprintf(&header[outIdx], "%s", "-------  ");
      }
      else if ((partyControl.pr == 0) && (partyControl.calendar == 1))
      {
         for (i = 0; i < MAX_RACES; i++)
         {
            nameFound[i] = 0;
         }

         outIdx = 0;
         outIdx += sprintf(&header[outIdx], "%s", "#  name   team  ");

         ptr = (Link_t *)listTimeLine->head;
         while (ptr->next != NULL)
         {
            currInfo = (TimeLineInfo_t *)ptr->currentObject;

            totalFound = 0;
            for (i = 0; i < numRacesG; i++)
            {
               if (currInfo->race[i].raceName[0] != '\0')
               {
                  totalFound += 1;
               }
            }

            if (totalFound == numRacesG)
            {
               /* Save it up - since it will be in order that was loaded */
               for (i = 0; i < numRacesG; i++)
               {
                  outIdx += sprintf(&header[outIdx], "%s  ", currInfo->race[i].raceName);
               }
               break;
            }

            ptr = ptr->next;
         }

         if (totalFound != numRacesG)
         {
            (pInfo->print_fp)("INTERNAL ERROR: did not find a rider that did all races - totalFound = %d, num = %d\n", totalFound, numRacesG);
            for (i = 0; i < (numRacesG - totalFound); i++)
            {
               outIdx += sprintf(&header[outIdx], "%s", "00-00-00  ");
            }
         }


         outIdx += sprintf(&header[outIdx], "Best %d av.  ", partyControl.bestOf);
         ColumnStore(pInfo, header);

         outIdx = 0;
         outIdx += sprintf(&header[outIdx], "%s", "--  ----   ----  ");
         for (i = 0; i < numRacesG; i++)
         {
            outIdx += sprintf(&header[outIdx], "%s", "-----  ");
         }

         outIdx += sprintf(&header[outIdx], "%s", "-----  ");
      }
      else
      {
         outIdx = 0;
         outIdx += sprintf(&header[outIdx], "%s", "#  name   team   PR  ");
         for (i = 1; i < numRacesG; i++)
         {
            if (i == 1)
            {
               outIdx += sprintf(&header[outIdx], "2nd  ", (i+1));
            }
            else if (i == 2)
            {
               outIdx += sprintf(&header[outIdx], "3rd  ", (i+1));
            }
            else
            {
               outIdx += sprintf(&header[outIdx], "%dth  ", (i+1));
            }
         }

         if (partyControl.bestOf == 1)
         {
            outIdx += sprintf(&header[outIdx], "%s", "PR");
         }
         else
         {
            outIdx += sprintf(&header[outIdx], "Bonus  Best %d   GRITified  ", partyControl.bestOf, partyControl.bestOf);
         }

         ColumnStore(pInfo, header);

         outIdx = 0;
         outIdx += sprintf(&header[outIdx], "%s", "-  ----   ----   -----  ");
         for (i = 1; i < numRacesG; i++)
         {
            outIdx += sprintf(&header[outIdx], "%s", "------  ");
         }

         /* improvement */
         outIdx += sprintf(&header[outIdx], "%s", " -------  ");

         /* X av. */
         outIdx += sprintf(&header[outIdx], "%s", " -------  ");

         /* X av. (adj) */
         outIdx += sprintf(&header[outIdx], "%s", " -------  ");
      }

      ColumnStore(pInfo, header);
#endif

      ptr = (Link_t *)listTimeOrder->head;
      while (ptr->next != NULL)
      {
         outIdx = 0;
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

         timeLineInfo = (TimeLineInfo_t *)currInfo->me;

         if ((partyControl.bestOf > 1) && (partyControl.clip == 1) && (timeLineInfo->numRaces < partyControl.bestOf))
         {
            ptr = ptr->next;
            continue;
         }

         if (timeLineInfo->team[0] != '\0')
         {
            if (timeLineInfo->prIdx.lastFound == 0)
            {
#ifdef CONSOLE_OUTPUT
               (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, timeLineInfo->team);
#else
               outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, timeLineInfo->name, timeLineInfo->team);
#endif
            }
            else
            {
               if (timeLineInfo->prIdx.idx != -1)
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d  **%-32s** %-30s ", count, timeLineInfo->name, timeLineInfo->team);
#else
// +++owen - fix later?
//                  outIdx += sprintf(&outStr[outIdx], "%d  **%s**  %s ", count, timeLineInfo->name, timeLineInfo->team);
                  outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, timeLineInfo->name, timeLineInfo->team);
#endif
               }
               else
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, timeLineInfo->team);
#else
                  outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, timeLineInfo->name, timeLineInfo->team);
#endif
               }
            }
         }
         else
         {
            if (timeLineInfo->prIdx.lastFound == 0)
            {
#ifdef CONSOLE_OUTPUT
               (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#else
               outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#endif
            }
            else
            {
               if (timeLineInfo->prIdx.idx != -1)
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d  **%-32s** %-30s ", count, timeLineInfo->name, " ");
#else
// +++owen - fix later?
//                  outIdx += sprintf(&outStr[outIdx], "  %-3d  **%-32s** %-30s ", count, timeLineInfo->name, " ");
                  outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#endif
               }
               else
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#else
                  outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#endif
               }
            }
         }
         count++;

         for (i = 0; i < numRacesG; i++)
         {
            if (partyControl.calendar == 0)
            {
               orderIdx = timeLineInfo->raceOrder[i].pos;
            }
            else
            {
               orderIdx = i;
            }

            if ((i == 0) && (timeLineInfo->prIdx.lastFound == 0))
            {
//               outIdx += sprintf(&outStr[outIdx], " %5.3f ", timeLineInfo->race[orderIdx].time.f);
#if 0
               outIdx += sprintf(&outStr[outIdx], " %5.3f ", GetTime(pInfo, timeLineInfo, orderIdx));
#else
               FormatTime(pInfo, timeLineInfo, orderIdx, timeString);
               if (strstr(timeString, "1000") == NULL)
               {
                  outIdx += sprintf(&outStr[outIdx], "  %s  ", timeString);
               }
               else
               {
                  outIdx += sprintf(&outStr[outIdx], "  0.00  ");
               }
#endif
            }
            else if ((i == 0) && (timeLineInfo->prIdx.lastFound == 1))
            {
               if (timeLineInfo->prIdx.idx != -1)
               {
//                  outIdx += sprintf(&outStr[outIdx], " **%5.3f** ", timeLineInfo->race[orderIdx].time.f);
#if 0
                  outIdx += sprintf(&outStr[outIdx], " **%5.3f** ", GetTime(pInfo, timeLineInfo, orderIdx));
#else
                  FormatTime(pInfo, timeLineInfo, orderIdx, timeString);
                  if (strstr(timeString, "1000") == NULL)
                  {
// +++owen - fix later?
//                     outIdx += sprintf(&outStr[outIdx], " **%s** ", timeString);
                     outIdx += sprintf(&outStr[outIdx], " %s ", timeString);
                  }
                  else
                  {
                     outIdx += sprintf(&outStr[outIdx], "  0.00  ");
                  }
#endif
               }
               else
               {
//                  outIdx += sprintf(&outStr[outIdx], " %5.3f ", timeLineInfo->race[orderIdx].time.f);
#if 0
                  outIdx += sprintf(&outStr[outIdx], " %5.3f ", GetTime(pInfo, timeLineInfo, orderIdx));
#else
                  FormatTime(pInfo, timeLineInfo, orderIdx, timeString);
                  if (strstr(timeString, "1000") == NULL)
                  {
                     outIdx += sprintf(&outStr[outIdx], "  %s  ", timeString);
                  }
                  else
                  {
                     outIdx += sprintf(&outStr[outIdx], "  0.00  ");
                  }
#endif
               }
            }
            else
            {
//               outIdx += sprintf(&outStr[outIdx], " %5.3f ", timeLineInfo->race[orderIdx].time.f);
#if 0
                  outIdx += sprintf(&outStr[outIdx], " %5.3f ", GetTime(pInfo, timeLineInfo, orderIdx));
#else
                  FormatTime(pInfo, timeLineInfo, orderIdx, timeString);
                  if (strstr(timeString, "1000") == NULL)
                  {
                     outIdx += sprintf(&outStr[outIdx], "   %s   ", timeString);
                  }
                  else
                  {
                     outIdx += sprintf(&outStr[outIdx], "  0.00  ");
                  }
#endif
            }
         }

         if (partyControl.pr == 0)
         {
            FormatTime2(pInfo, currInfo->avOrig, timeString2, timeString);
#ifdef CONSOLE_OUTPUT
//          (pInfo->print_fp)(" %5.3f ", currInfo->avOrig);
            (pInfo->print_fp)(" %s ", timeString2);
            (pInfo->print_fp)("\n");
#else
//          outIdx += sprintf(&outStr[outIdx], " %5.3f  ", currInfo->avOrig);
            outIdx += sprintf(&outStr[outIdx], " %s ", timeString2);
            ColumnStore(pInfo, outStr);
#endif
         }
         else
         {
            improvement = 0;
            if (timeLineInfo->prIdx.lastFound == 1)
            {
               if (currInfo->prIdx.idx == -1)
               {
                  improvement = currInfo->prIdx.improvement;
               }
               else
               {
                  improvement = -1.0 * currInfo->prIdx.improvement;
               }
            }

            FormatTime2(pInfo, improvement, timeString2, timeString);
            FormatTime2(pInfo, currInfo->avOrig, timeString3, timeString);
            FormatTime2(pInfo, currInfo->av, timeString4, timeString);
#ifdef CONSOLE_OUTPUT
//            (pInfo->print_fp)("  %5.3f  %5.3f  %5.3f  ", improvement, currInfo->avOrig, currInfo->av);
            (pInfo->print_fp)("  %s  %s  %s  ", timeString2, timeString3, timeString4);
            (pInfo->print_fp)("\n");
#else
//            outIdx += sprintf(&outStr[outIdx], "  %5.3f  %5.3f  %5.3f  ", improvement, currInfo->avOrig, currInfo->av);
            outIdx += sprintf(&outStr[outIdx], "  %s  %s  %s  ", timeString2, timeString3, timeString4);
            ColumnStore(pInfo, outStr);
#endif
         }

         ptr = ptr->next;
      }
   }
   else if (mode == SHOW_RICHMOND)
   {
#ifndef CONSOLE_OUTPUT
      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "#  name   team  ");

      ptr = (Link_t *)listTimeLine->head;
      while (ptr->next != NULL)
      {
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

         for (i = 0; i < numRacesG; i++)
         {
            if (currInfo->race[i].raceName[0] != '\0')
            {
               totalFound += 1;
            }
         }

         if (totalFound == numRacesG)
         {
            /* Save it up - since it will be in order that was loaded */
            for (i = 0; i < numRacesG; i++)
            {
               outIdx += sprintf(&header[outIdx], "%s  ", currInfo->race[i].raceName);
            }
            break;
         }

         ptr = ptr->next;
      }

      outIdx += sprintf(&header[outIdx], "Total Time  ", partyControl.bestOf);
      ColumnStore(pInfo, header);

      outIdx = 0;
      outIdx += sprintf(&header[outIdx], "%s", "--  ----   ----  ");
      for (i = 0; i < numRacesG; i++)
      {
         outIdx += sprintf(&header[outIdx], "%s", "-----  ");
      }

      outIdx += sprintf(&header[outIdx], "%s", "-----  ");
#endif

      ColumnStore(pInfo, header);

      ptr = (Link_t *)listTimeOrder->head;
      while (ptr->next != NULL)
      {
         outIdx = 0;
         currInfo = (TimeLineInfo_t *)ptr->currentObject;

         timeLineInfo = (TimeLineInfo_t *)currInfo->me;

         if ((partyControl.bestOf > 1) && (partyControl.clip == 1) && (timeLineInfo->numRaces < partyControl.bestOf))
         {
            ptr = ptr->next;
            continue;
         }

         if (timeLineInfo->team[0] != '\0')
         {
            if (timeLineInfo->prIdx.lastFound == 0)
            {
#ifdef CONSOLE_OUTPUT
               (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, timeLineInfo->team);
#else
               outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, timeLineInfo->name, timeLineInfo->team);
#endif
            }
            else
            {
               if (timeLineInfo->prIdx.idx != -1)
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d  **%-32s** %-30s ", count, timeLineInfo->name, timeLineInfo->team);
#else
                  outIdx += sprintf(&outStr[outIdx], "%d  **%s**  %s ", count, timeLineInfo->name, timeLineInfo->team);
#endif
               }
               else
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, timeLineInfo->team);
#else
                  outIdx += sprintf(&outStr[outIdx], "%d  %s  %s ", count, timeLineInfo->name, timeLineInfo->team);
#endif
               }
            }
         }
         else
         {
            if (timeLineInfo->prIdx.lastFound == 0)
            {
#ifdef CONSOLE_OUTPUT
               (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#else
               outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#endif
            }
            else
            {
               if (timeLineInfo->prIdx.idx != -1)
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d  **%-32s** %-30s ", count, timeLineInfo->name, " ");
#else
                  outIdx += sprintf(&outStr[outIdx], "  %-3d  **%-32s** %-30s ", count, timeLineInfo->name, " ");
#endif
               }
               else
               {
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#else
                  outIdx += sprintf(&outStr[outIdx], "  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
#endif
               }
            }
         }
         count++;

         for (i = 0; i < numRacesG; i++)
         {
            orderIdx = i;

            FormatTime(pInfo, timeLineInfo, orderIdx, timeString);
            if (strstr(timeString, "1000") == NULL)
            {
               outIdx += sprintf(&outStr[outIdx], "  %s  ", timeString);
            }
            else
            {
               outIdx += sprintf(&outStr[outIdx], "  0.00  ");
            }

         }

         FormatTime3(pInfo, timeLineInfo, i, timeString, 0);
//         FormatTime(pInfo, timeLineInfo, i, timeString);
         if (strstr(timeString, "1000") == NULL)
         {
            outIdx += sprintf(&outStr[outIdx], "  %s  ", timeString);
         }
         else
         {
            outIdx += sprintf(&outStr[outIdx], "  0.00  ");
         }

#if 0
         FormatTime2(pInfo, currInfo->avOrig, timeString2, timeString);
#ifdef CONSOLE_OUTPUT
//          (pInfo->print_fp)(" %5.3f ", currInfo->avOrig);
         (pInfo->print_fp)(" %s ", timeString2);
         (pInfo->print_fp)("\n");
#else
//          outIdx += sprintf(&outStr[outIdx], " %5.3f  ", currInfo->avOrig);
         outIdx += sprintf(&outStr[outIdx], " %s ", timeString2);
         ColumnStore(pInfo, outStr);
#endif
#endif
         ColumnStore(pInfo, outStr);

         ptr = ptr->next;
      }
   }

#ifdef CONSOLE_OUTPUT
   (pInfo->print_fp)("\n\n");
#else
   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");
#endif

}

void cmd_party_show(CLI_PARSE_INFO *pInfo)
{
   cmd_party_show_common(pInfo, SHOW_POINTS);
}

void cmd_party_tally(CLI_PARSE_INFO *pInfo)
{
   cmd_party_show_common(pInfo, SHOW_TALLY);
}

void cmd_party_tempoish(CLI_PARSE_INFO *pInfo)
{
   cmd_party_show_common(pInfo, SHOW_TEMPOISH);
}

void cmd_party_richmond(CLI_PARSE_INFO *pInfo)
{
   cmd_party_show_common(pInfo, SHOW_RICHMOND);
}

void cmd_party_time(CLI_PARSE_INFO *pInfo)
{
   cmd_party_show_common(pInfo, SHOW_TIME);
}

void PartySetMax(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {max-number-of-racers}\n", pInfo->argv[0]);
      return;
   }

   partyControl.max = cliCharToUnsignedLong(pInfo->argv[1]);
}

void PartySetBestOf(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {N-in-best-of-N-races}\n", pInfo->argv[0]);
      return;
   }

   partyControl.bestOf = cliCharToUnsignedLong(pInfo->argv[1]);
}

#define CUTOFF_CAT1 (1)
#define CUTOFF_CAT2 (2)
#define CUTOFF_CAT3 (3)

void PartySetCatCommon(CLI_PARSE_INFO *pInfo, float f, int mode)
{
   if (mode == CUTOFF_CAT1)
   {
      partyControl.cat1 = f;
   }
   else if (mode == CUTOFF_CAT2)
   {
      partyControl.cat2 = f;
   }
   else if (mode == CUTOFF_CAT3)
   {
      partyControl.cat3 = f;
   }
   else
   {
      (pInfo->print_fp)("INTERNAL ERROR: did not recognize mode = %d\n", mode);
   }
}

void PartySetCat1(CLI_PARSE_INFO *pInfo)
{
   float f;
   
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {cutoff-in-decimal}\n", pInfo->argv[0]);
      return;
   }

   sscanf(pInfo->argv[1], "%f", &f);

   (pInfo->print_fp)("f = %5.3f\n", f);

   PartySetCatCommon(pInfo, f, CUTOFF_CAT1);
}

void PartySetCat2(CLI_PARSE_INFO *pInfo)
{
   float f;
   
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {cutoff-in-decimal}\n", pInfo->argv[0]);
      return;
   }

   sscanf(pInfo->argv[1], "%f", &f);

   (pInfo->print_fp)("f = %5.3f\n", f);

   PartySetCatCommon(pInfo, f, CUTOFF_CAT2);
}

void PartySetCat3(CLI_PARSE_INFO *pInfo)
{
   float f;
   
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {cutoff-in-decimal}\n", pInfo->argv[0]);
      return;
   }

   sscanf(pInfo->argv[1], "%f", &f);

   (pInfo->print_fp)("f = %5.3f\n", f);

   PartySetCatCommon(pInfo, f, CUTOFF_CAT3);
}

void PartySetCat(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {0=off} | {1,2,3}\n", pInfo->argv[0]);
      return;
   }

   if (strcmp(pInfo->argv[1], "0") == 0)
   {
      /* off */
      partyControl.cat = 0;
   }
   else if (strcmp(pInfo->argv[1], "1") == 0)
   {
      partyControl.cat = 1;
   }
   else if (strcmp(pInfo->argv[1], "2") == 0)
   {
      partyControl.cat = 2;
   }
   else if (strcmp(pInfo->argv[1], "3") == 0)
   {
      partyControl.cat = 3;
   }
   else
   {
      (pInfo->print_fp)("USAGE: %s {0=off} | {1,2,3}\n", pInfo->argv[0]);
      return;
   }
}


void PartySetTallyMin(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {min-races-to-be-included}\n", pInfo->argv[0]);
      return;
   }

   partyControl.tallyMin = cliCharToUnsignedLong(pInfo->argv[1]);
}

void PartySetAdjust(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {min-races-to-be-included}\n", pInfo->argv[0]);
      return;
   }

   partyControl.adjust = cliCharToUnsignedLong(pInfo->argv[1]);
}

void PartySetCalendar(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.calendar = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.calendar = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetBonus(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.bonus = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.bonus = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetDouble(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.doubleUp = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.doubleUp = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetAgeBonus(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.ageBonus = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.ageBonus = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetDebug(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.debug = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.debug = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetGroup(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {A|B|All|GRIT}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "A") == 0)
    {
       partyControl.groupId = GROUP_A;
    }
    else if (strcmp(pInfo->argv[1], "B") == 0)
    {
       partyControl.groupId = GROUP_B;
    }
    else if (strcmp(pInfo->argv[1], "All") == 0)
    {
       partyControl.groupId = GROUP_ALL;
    }
    else if (strcmp(pInfo->argv[1], "GRIT") == 0)
    {
       partyControl.groupId = GROUP_GRIT;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {A|B|All|GRIT}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetPR(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.pr = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.pr = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetLockdown(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.lockDown = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.lockDown = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetClip(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.clip = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.clip = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetNickName(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {on|off}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "on") == 0)
    {
        partyControl.nickName = 1;
    }
    else if (strcmp(pInfo->argv[1], "off") == 0)
    {
        partyControl.nickName = 0;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {on|off}\n", pInfo->argv[0]);
        return;
    }
}

void PartySetSnapA(CLI_PARSE_INFO *pInfo)
{
   int i;
   Link_t *ptr;
   TimeLineInfo_t *timeLineInfo;

   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

      if (timeLineInfo->groupId == GROUP_A)
      {
         PartyAddAthleteA(pInfo, timeLineInfo->name);
      }

      ptr = ptr->next;
   }
}

void PartySetSnapB(CLI_PARSE_INFO *pInfo)
{
   int i;
   Link_t *ptr;
   TimeLineInfo_t *timeLineInfo;

   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

      if (timeLineInfo->groupId == GROUP_B)
      {
         PartyAddAthleteB(pInfo, timeLineInfo->name);
      }

      ptr = ptr->next;
   }

//   PartyForceShow(pInfo);
}

void PartySetSnapAll(CLI_PARSE_INFO *pInfo)
{
   int i;
   Link_t *ptr;
   TimeLineInfo_t *timeLineInfo;

   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

      if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_ALL))
      {
         PartyAddAthleteAll(pInfo, timeLineInfo->name);
      }

      ptr = ptr->next;
   }

//   PartyForceShow(pInfo);
}

void PartySetSnapGRIT(CLI_PARSE_INFO *pInfo)
{
#if 0
   int i;
   Link_t *ptr;
   TimeLineInfo_t *timeLineInfo;

   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

      if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_GRIT))
      {
         PartyAddAthleteGRIT(pInfo, timeLineInfo->name);
      }

      ptr = ptr->next;
   }

//   PartyForceShow(pInfo);
#endif
}

void PartySetSnapReset(CLI_PARSE_INFO *pInfo)
{
   int i;

   for (i = 0; i < MAX_ATHLETES; i++)
   {
      athleteA[i].enabled = 0;
      athleteB[i].enabled = 0;
      athleteAll[i].enabled = 0;
      athleteGRIT[i].enabled = 0;
   }
}

static const CLI_PARSE_CMD party_set_snap_cmd[] =
{
    { "A",      PartySetSnapA,    "snap A atheletes"},
    { "B",      PartySetSnapB,    "snap B atheletes"},
    { "All",    PartySetSnapAll,  "snap All atheletes"},
    { "GRIT",   PartySetSnapGRIT, "snap GRIT atheletes"},
    { "show",   PartyForceShow,   "show athlete tables"},
    { "reset",  AthleteReset,     "reset athlete tables"},
    { NULL, NULL, NULL }
};

static void PartySetSnap(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_set_snap_cmd);
}

static const CLI_PARSE_CMD party_set_cmd[] =
{
    { "max",       PartySetMax,       "max number of race entries"},
    { "bestof",    PartySetBestOf,    "N in best-of-N races"},
    { "cat",       PartySetCat,       "category splicing {0=off} | {1,2,3}"},
    { "cat1",      PartySetCat1,      "cat1 cutoff for sprints"},
    { "cat2",      PartySetCat2,      "cat2 cutoff for sprints"},
    { "cat3",      PartySetCat3,      "cat3 cutoff for sprints"},
    { "pr",        PartySetPR,        "turn PR weighting on|off"},
    { "adjust",    PartySetAdjust,    "adjust (multiply) improvement by X"},
    { "calendar",  PartySetCalendar,  "show tempoish races by calendar time"},
    { "tallymin",  PartySetTallyMin,  "minimum number of races to be included in tally (default 10)"},
    { "bonus",     PartySetBonus,     "on|off - 5 pts for 3 races, 10 pts for 4 races"},
    { "double",    PartySetDouble,    "on|off - DOUBLE points for Alpe race (warranted)"},
    { "ageBonus",  PartySetAgeBonus,  "on|off - extra 5 points for being 55+"},
    { "debug",     PartySetDebug,     "on|off"},
    { "group",     PartySetGroup,     "A|B|All|GRIT (unknown is default)"},
    { "lockdown",  PartySetLockdown,  "on|off"},
    { "clip",      PartySetClip,      "on|off"},
    { "snap",      PartySetSnap,      "snap athletes"},
    { "nickname",  PartySetNickName,  "on|off"},
    { NULL, NULL, NULL }
};

static void cmd_party_set(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_set_cmd );
}

void AthleteReset(CLI_PARSE_INFO *pInfo)
{
   int i;

   for (i = 0; i < MAX_ATHLETES; i++)
   {
      athleteA[i].enabled = 0;
      athleteB[i].enabled = 0;
      athleteAll[i].enabled = 0;
      athleteGRIT[i].enabled = 0;
   }

   strcpy(athleteA[i-1].name, "ATHLETE_END");
   strcpy(athleteB[i-1].name, "ATHLETE_END");
   strcpy(athleteAll[i-1].name, "ATHLETE_END");
   strcpy(athleteGRIT[i-1].name, "ATHLETE_END");
}

void ListReset(CLI_PARSE_INFO *pInfo, LinkList_t *list)
{
   int i;
   Link_t *ptr;
   Link_t *tmp;

   if (list != NULL)
   {
      /* See if there is a match */
      ptr = (Link_t *)list->head ;

      if (ptr->next != NULL)
      {
         while(ptr->next != NULL)
         {
            /* Free object */
            TimeLineRemove((TimeLineInfo_t *)ptr->currentObject);

            tmp = ptr->next;

            if (LinkRemove(list, ptr) != TRUE)
            {
               (pInfo->print_fp)("Could not remove item from list\n");
               break;
            }
            ptr = tmp;
         }
      }
   }

}

static void PartyResetAll(CLI_PARSE_INFO *info)
{
   Link_t *nil;

   PartyInit(info);

#if 0
   TimeLineInit(info);
   ListReset(info, listTimeLine);
   ListReset(info, listTimeOrder);
#endif
   
   ColumnReset(info);

   numRacesG = 0;
   raceIdxG = -1;
   timeLineInfoG = NULL;
}

static void ListResets(CLI_PARSE_INFO *info)
{
   ListReset(info, listTimeLine);
   ListReset(info, listTimeOrder);
}

static const CLI_PARSE_CMD party_reset_cmd[] =
{
    { "athlete",       AthleteReset,   "reset athlete table"},
    { "list",          ListResets,     "reset lists"},
    { "columns",       ColumnReset,    "reset columns"},
    { "all",           PartyResetAll,  "reset all"},
    { NULL, NULL, NULL }
};

static void cmd_party_reset(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_reset_cmd );
}

void PartyAddAthleteA(CLI_PARSE_INFO *info, char *name)
{
    int i,j;
    int idx = 0;
    char athlete[MAX_COMMAND_LENGTH];

    /* Find first available slot */
    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteA[i].enabled == 0)
       {
          break;
       }
    }

    if (i == MAX_ATHLETES)
    {
       (info->print_fp)("INTERNAL ERROR: Ran out of athletes - max = %d\n", MAX_ATHLETES);
       return;
    }

    strcpy(athleteA[i].name, name);
    athleteA[i].enabled = 1;
}

void PartyAddAthleteB(CLI_PARSE_INFO *info, char *name)
{
    int i,j;
    int idx = 0;
    char athlete[MAX_COMMAND_LENGTH];

    /* Find first available slot */
    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteB[i].enabled == 0)
       {
          break;
       }
    }

    if (i == MAX_ATHLETES)
    {
       (info->print_fp)("INTERNAL ERROR: Ran out of athletes - max = %d\n", MAX_ATHLETES);
       return;
    }

    strcpy(athleteB[i].name, name);
    athleteB[i].enabled = 1;

}


void PartyAddAthleteAll(CLI_PARSE_INFO *info, char *name)
{
    int i,j;
    int idx = 0;
    char athlete[MAX_COMMAND_LENGTH];

    /* Find first available slot */
    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteAll[i].enabled == 0)
       {
          break;
       }
    }

    if (i == MAX_ATHLETES)
    {
       (info->print_fp)("INTERNAL ERROR: Ran out of athletes - max = %d\n", MAX_ATHLETES);
       return;
    }

    strcpy(athleteAll[i].name, name);
    athleteAll[i].enabled = 1;

}

void PartyAddAthleteGRIT(CLI_PARSE_INFO *info, char *name)
{
    int i,j;
    int idx = 0;
    char athlete[MAX_COMMAND_LENGTH];

    /* Find first available slot */
    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteGRIT[i].enabled == 0)
       {
          break;
       }
    }

    if (i == MAX_ATHLETES)
    {
       (info->print_fp)("INTERNAL ERROR: Ran out of athletes - max = %d\n", MAX_ATHLETES);
       return;
    }

    strcpy(athleteGRIT[i].name, name);
    athleteGRIT[i].enabled = 1;

}

static void PartyForceCommon(CLI_PARSE_INFO *info, Athlete_t *athleteTable)
{
    int i,j;
    int idx = 0;
    char athlete[MAX_COMMAND_LENGTH];

    /* Find first available slot */
    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteTable[i].enabled == 0)
       {
          break;
       }
#if 0
        if (strstr(athleteTable[i].name, "ATHLETE_END") != 0)
        {
            break;
        }
#endif
    }

    if (i == MAX_ATHLETES)
    {
        (info->print_fp)("INFO: No more available athleteslots - maximum number = %d\n", MAX_ATHLETES);
    }


    /* package command line arguments into a single string */
    memset(athlete, 0, MAX_COMMAND_LENGTH);

    for (j = 0; j < (info->argc - 1); j++)
    {
       /* Package up argc and argv */
       idx += sprintf(&athlete[idx], "%s", (char *)info->argv[j+1]);

       athlete[idx++] = ' ';

       if (idx >= MAX_COMMAND_LENGTH)
       {
          (info->print_fp)("Athlete name exceeds maximum length of %d characters\n", MAX_COMMAND_LENGTH);
          return;
       }
    }

    athlete[idx-1] = '\0';

    strcpy(athleteTable[i].name, athlete);
    athleteTable[i].enabled = 1;

    /* Set new END */
    strcpy(athleteTable[i+1].name, "FILTER_END");
    athleteTable[i+1].enabled = 0;

    /* Show what we have so far */
    for (i = 0; i < MAX_ATHLETES ;i++)
    {
       if (athleteTable[i].enabled == 0)
       {
          break;
       }

       (info->print_fp)("%3d: [%d] %s\n", i, athleteTable[i].enabled, athleteTable[i].name);
    }
}

static void PartyForceA(CLI_PARSE_INFO *info)
{
    if ( info->argc < 2)
    {
        (info->print_fp)("USAGE: %s {athlete} \n", info->argv[0]);
        return;
    }

    PartyForceCommon(info, athleteA);
}

static void PartyForceB(CLI_PARSE_INFO *info)
{
    if ( info->argc < 2)
    {
        (info->print_fp)("USAGE: %s {athlete} \n", info->argv[0]);
        return;
    }

    PartyForceCommon(info, athleteB);
}

static void PartyForceGRIT(CLI_PARSE_INFO *info)
{
    if ( info->argc < 2)
    {
        (info->print_fp)("USAGE: %s {athlete} \n", info->argv[0]);
        return;
    }

    PartyForceCommon(info, athleteGRIT);
}

static void PartyForceShow(CLI_PARSE_INFO *info)
{
   int i;

   (info->print_fp)("A LIST:\n");

    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteA[i].enabled == 0)
       {
          break;
       }

       (info->print_fp)("%3d: [%d] %s\n", i, athleteA[i].enabled, athleteA[i].name);
    }

   (info->print_fp)("B LIST:\n");

    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteB[i].enabled == 0)
       {
          break;
       }

       (info->print_fp)("%3d: [%d] %s\n", i, athleteB[i].enabled, athleteB[i].name);
    }

   (info->print_fp)("All LIST:\n");

    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteAll[i].enabled == 0)
       {
          break;
       }

       (info->print_fp)("%3d: [%d] %s\n", i, athleteAll[i].enabled, athleteAll[i].name);
    }

   (info->print_fp)("GRIT LIST:\n");

    for (i = 0; i < MAX_ATHLETES; i++)
    {
       if (athleteGRIT[i].enabled == 0)
       {
          break;
       }

       (info->print_fp)("%3d: [%d] %s\n", i, athleteGRIT[i].enabled, athleteGRIT[i].name);
    }

}

static const CLI_PARSE_CMD party_force_cmd[] =
{
   { "show",    PartyForceShow, "show force athlete lists"},
   { "A",       PartyForceA,    "{athlete}"},
   { "B",       PartyForceB,    "{athlete}"},
   { "GRIT",    PartyForceGRIT, "{athlete}"},
   { NULL, NULL, NULL }
};

static void cmd_party_force(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_force_cmd );
}

void PartyRawKOM(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, (PARTY_KOM_SPRINTS | PARTY_KOM_SPRINTS_RAW));
}

void PartyRawSprints(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, (PARTY_KOM_SPRINTS | PARTY_KOM_SPRINTS_RAW));
}

void PartyRawFollowing(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, (PARTY_FOLLOWING | PARTY_FOLLOWING_RAW));
}

void PartyRawResults(CLI_PARSE_INFO *pInfo)
{
   cmd_party_common(pInfo, (PARTY_RESULTS | PARTY_RESULTS_RAW));
}


static const CLI_PARSE_CMD party_raw_cmd[] =
{
   { "kom",       PartyRawKOM,       "raw KOM results"},
   { "sprints",   PartyRawSprints,   "raw Sprint results"},
   { "following", PartyRawFollowing, "raw Sprint results"},
   { "results",   PartyRawResults,   "raw results"},
   { NULL, NULL, NULL }
};

static void cmd_party_raw(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_raw_cmd );
}

static void PartySelf(CLI_PARSE_INFO *pInfo)
{
   char header[MAX_STRING_SIZE];
   char s1[MAX_STRING_SIZE];
   char s2[MAX_STRING_SIZE];
   char s3[MAX_STRING_SIZE];
   char s4[MAX_STRING_SIZE];
   char s5[MAX_STRING_SIZE];
   char s6[MAX_STRING_SIZE];
   char s7[MAX_STRING_SIZE];
   char s8[MAX_STRING_SIZE];
   char s9[MAX_STRING_SIZE];
   char sA[MAX_STRING_SIZE];


#if 1
   sprintf(s1, "%s", "  1  Steve Tappan  [GRIT]  18:21    338w  5.0wkg");
   sprintf(s2, "%s", "  2  Derek Sawyer  [GRIT][Rippers]  18:43    347w  4.6wkg");
   sprintf(s3, "%s", "  3  Olivier PERRIN  Asvel Tri  19:20    297w  4.7wkg");

   ColumnReset(pInfo);
   (pInfo->print_fp)("TESTA:\n\n");

   ColumnStore(pInfo, s1);
   ColumnStore(pInfo, s2);
   ColumnStore(pInfo, s3);

   // ColumnStore(pInfo, outStr);
   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");

#endif
#if 1

   sprintf(s1, "%s", "3  Steve Tappan  [GRIT]  06:39    374w  5.5wkg");

   // sprintf(s2, "%s", "4  Thomas Woods  TeamODZ  06:41    332w  5.2wkg");
   sprintf(s2, "%s", "4  Thomas Woodsaa  TeamODZzzz  06:41    332wddddddd  5.2wkg              ");

   // sprintf(s3, "%s", "5  Yusuke Saeki  (SBC Vertex Racing Team)    06:42    329w  5.1wkg");
   sprintf(s3, "%s", "5  Yusuke Saeki  (SBC Vertex Racing Team)    06:42bbb    329wccc  5.1wkg");

#if 0
   (pInfo->print_fp)("STRINGS:\n");
   (pInfo->print_fp)("%s\n", s1);
   (pInfo->print_fp)("%s\n", s2);
   (pInfo->print_fp)("%s\n", s3);
   (pInfo->print_fp)("\n");
#endif

   ColumnReset(pInfo);
   (pInfo->print_fp)("TESTA:\n\n");

   ColumnStore(pInfo, s2);
   ColumnStore(pInfo, s3);

   // ColumnStore(pInfo, outStr);
   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");

   sprintf(s2, "%s", "4  Thomas Woodsaa  TeamODZzzz  06:41    332wddddddd  5.2wkg              ");
   sprintf(s3, "%s", "5  Yusuke Saeki  (SBC Vertex Racing Team)    06:42bbb    329wccc  5.1wkg               ");

   ColumnReset(pInfo);
   (pInfo->print_fp)("TESTA:\n\n");

   ColumnStore(pInfo, s2);
   ColumnStore(pInfo, s3);

   // ColumnStore(pInfo, outStr);
   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");

#if 1
   (pInfo->print_fp)("TEST2:\n\n");
   ColumnReset(pInfo);

   sprintf(s1, "%s", "3  Steve Tappan  [GRIT]bbbbbbbb  06:39    374w  5.5wkg");
   sprintf(s2, "%s", "4  Thomas Woodsaaa  TeamODZ  06:41    332wcccccccc  5.2wkgdddddddddddd");
   sprintf(s3, "%s", "5  Yusuke Saeki          (SBC Vertex Racing Team)                06:42           329w       5.1wkg");

   ColumnStore(pInfo, s1);
   ColumnStore(pInfo, s2);
   ColumnStore(pInfo, s3);

   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");

   (pInfo->print_fp)("TEST3:\n\n");

   ColumnReset(pInfo);

   sprintf(header, "%s", "#  a1  a2  a3  a4  a5  a6  a7  a8  a9  a10");
   sprintf(s1, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s2, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s3, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s4, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s5, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s6, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s7, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s8, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s9, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(sA, "%s", "3  1  2  3  4  5  6  7  8  9  10");

   ColumnStore(pInfo, header);
   ColumnStore(pInfo, s1);
   ColumnStore(pInfo, s2);
   ColumnStore(pInfo, s3);
   ColumnStore(pInfo, s4);
   ColumnStore(pInfo, s5);
   ColumnStore(pInfo, s6);
   ColumnStore(pInfo, s7);
   ColumnStore(pInfo, s8);
   ColumnStore(pInfo, s9);
   ColumnStore(pInfo, sA);

   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");

   ColumnReset(pInfo);

   sprintf(header, "%s", "#  a1  a2aaaaa  a3  a4  a5  a6  a7  a8  a9  a10");
   sprintf(s1, "%s", "3  1  2  3  4  5  6  7  8  9  10   ");
   sprintf(s2, "%s", "3  1  aaa2  3  4  5  6  7  8  9  10  ");
   sprintf(s3, "%s", "3  1  2  3  bbbb4  5  6  7  8  9  10           ");
   sprintf(s4, "%s", "3  1  2  3  4  5  6  7  8  9  10                 ");
   sprintf(s5, "%s", "3  1  2  3  4  5  bbbb6  7  8  9  10");
   sprintf(s6, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s7, "%s", "3  1  2  3  4  5  6  7  bbbb8  9  10");
   sprintf(s8, "%s", "3  1  2  3  4  5  6  7  8  9  10");
   sprintf(s9, "%s", "3  1  2  3  4  5  6  7  8  9  bbbb10");
   sprintf(sA, "%s", "3  1  2  3  4  5  6  7  8  9                          10                     ");

   ColumnStore(pInfo, header);
   ColumnStore(pInfo, s1);
   ColumnStore(pInfo, s2);
   ColumnStore(pInfo, s3);
   ColumnStore(pInfo, s4);
   ColumnStore(pInfo, s5);
   ColumnStore(pInfo, s6);
   ColumnStore(pInfo, s7);
   ColumnStore(pInfo, s8);
   ColumnStore(pInfo, s9);
   ColumnStore(pInfo, sA);

   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");
#endif
#endif
}

char *GroupId(char *out, int groupId, int place)
{
   if (groupId == GROUP_A)
   {
      sprintf(out, "A   %d", place);
   }
   else if (groupId == GROUP_B)
   {
      sprintf(out, "B   %d", place);
   }
   else
   {
      sprintf(out, "Z   %d", place);
   }
}


static void PartyWriteResults_common(CLI_PARSE_INFO *pInfo, char *outfile)
{
   Link_t *ptr;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *currInfo;
   char line[MAX_STRING_SIZE];
   char timeString[MAX_STRING_SIZE];
   FILE *fp_out;
   
   int count = 1;

   fp_out = fopen(outfile, "w");

   if (!fp_out)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not open %s\n", outfile);
      return;
   }

   ptr = (Link_t *)listTimeOrder->head;

   if (ptr->next == NULL)
   {
      (pInfo->print_fp)("INFO: Could not write results, results not yet calculated\n");
      return;
   }

   /*
    *
    * A       7         <===== WE MUST HAVE his category?                  timeLineInfo->group[0]
    * Gabriel Mathisen  <===== NAME                                        timeLineInfo->name[0]
    * GRIT              <===== TEAM                                        timeLineInfo->teamName[0]
    *         
    * 01:03          <====== TOTAL TIME                                    timeLineInfo->totalTime[0]
    * 558w 9.0wkg    <====== WE MUST have this? WE BETTER HAVE THIS!!!     timeLineInfo->wattLine[0];
    *         Power                                                        
    * A       4       
    * Daniel Sutherland (BMTR_A2)
    * TEAM BMTR
    *         
    * 01:08
    * 549w 7.8wkg
    *         Power
    *
    */
    
   while (ptr->next != NULL)
   {
      currInfo = (TimeLineInfo_t *)ptr->currentObject;

      timeLineInfo = (TimeLineInfo_t *)currInfo->me;

//      GroupId(line, timeLineInfo->groupId, count);
//      fprintf(fp_out, "%s\n", line);
      fprintf(fp_out, "%s\n", timeLineInfo->groupLine);

      count++;

      fprintf(fp_out, "%s\n", timeLineInfo->name);

      if (timeLineInfo->team[0] == '-')
      {
         fprintf(fp_out, "\n");
      }
      else
      {
         fprintf(fp_out, "%s\n", timeLineInfo->team);
         fprintf(fp_out, "\n");
      }

      FormatTime3(pInfo, timeLineInfo, numRacesG, timeString, 0);

      fprintf(fp_out, "%s \n", timeString);
      fprintf(fp_out, "%s \n", timeLineInfo->wattLine);

      fprintf(fp_out, "        Power\n");

      ptr = ptr->next;
   }

   fclose(fp_out);
}

static void PartyWriteResults(CLI_PARSE_INFO *pInfo)
{
   char outfile[MAX_STRING_SIZE];
   
   int count = 1;

   if (pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {filename} \n", pInfo->argv[0]);
      return;
   }

   sprintf(outfile, "%s", pInfo->argv[1]);

   PartyWriteResults_common(pInfo, outfile);
}

static const CLI_PARSE_CMD party_write_cmd[] =
{
   { "results", PartyWriteResults, "write results to a {file}"},
   { NULL, NULL, NULL }
};

static void cmd_party_write(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_write_cmd );
}

static void PartyConvertInline(CLI_PARSE_INFO *pInfo)
{
   Link_t *ptr;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *currInfo;
   char line[MAX_STRING_SIZE];
   char timeString[MAX_STRING_SIZE];
   char infile[MAX_STRING_SIZE];
   char outfile[MAX_STRING_SIZE];
   char tmpfile[MAX_STRING_SIZE];
   FILE *fp_out;
   FILE *fp_tmp;
   int columnId;

   int raceId = 0;
   int count = 1;

#if 1
   PartyInit(pInfo);
   ListReset(pInfo, listTimeLine);
   ListReset(pInfo, listTimeOrder);
   numRacesG = 0;
   ColumnReset(pInfo);
#endif

   if (pInfo->argc < 4)
   {
      (pInfo->print_fp)("USAGE: %s {in-file} {column-id} {out-file} \n", pInfo->argv[0]);
      return;
   }

   sprintf(infile, "%s", pInfo->argv[1]);

   columnId = cliCharToUnsignedLong(pInfo->argv[2]);

   sprintf(outfile, "%s", pInfo->argv[3]);

   /* Test it with an already formatted file */
   
//   ColumnizeResults(pInfo, infile, columnId, OUTPUT_MODE_CONVERT, fp_out);

   sprintf(tmpfile, "tmp.txt");
   ColumnizeResults(pInfo, infile, columnId, OUTPUT_MODE_CONVERT, tmpfile);

   /* Now, convert tmp.txt into an ORDERED list of results !!! */

   /* Do normal processing/ordering of "kom" results */

   cmd_party_common_inline(pInfo, PARTY_KOM_SPRINTS, "tmp.txt", "tmp.md", "race1", raceId);
   // cmd_party_common_inline(pInfo, PARTY_KOM_SPRINTS, tmpfile, tmpmd, race1, raceId);

   cmd_party_show_common(pInfo, SHOW_TIME);

   /* Now, write that out */
   PartyWriteResults_common(pInfo, outfile);

}


static const CLI_PARSE_CMD party_convert_cmd[] =
{
   { "inline", PartyConvertInline, "convert {in-file} {column-id} {out-file}"},
   { NULL, NULL, NULL }
};

static void cmd_party_convert(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_convert_cmd );
}


static const CLI_PARSE_CMD cmd_party_commands[] =
{
   { "start",           cmd_party_start,                "start party"},
   { "stop",            cmd_party_stop,                 "stop party"},
   { "reset",           cmd_party_reset,                "reset party"},
   { "show",            cmd_party_show,                 "show party status"},
   { "convert",         cmd_party_convert,              "convert results"},
   { "write",           cmd_party_write,                "write results"},
   { "tally",           cmd_party_tally,                "tally up all results"},
   { "tempoish",        cmd_party_tempoish,             "show average time sprint results"},
   { "richmond",        cmd_party_richmond,             "cummulative time across results"},
   { "time",            cmd_party_time,                 "show time sprint results"},
   { "raw",             cmd_party_raw,                  "party raw"},
   { "set",             cmd_party_set,                  "party set commands"},
   { "force",           cmd_party_force,                "force {athlete} {A|B} force an athlete to a category"},
   { "kom",             cmd_party_kom_and_sprints,      "kom [infile] [outfile]"},
   { "sprints",         cmd_party_kom_and_sprints,      "sprints [infile] [outfile]"},
   { "results",         cmd_party_results,              "results [infile] [outfile]"},
   { "results2",        cmd_party_results2,             "results2 [infile] [outfile]"},
   { "following",       cmd_party_following,            "following [infile] [outfile]"},
   { "following2",      cmd_party_following2,           "following2 [infile] [outfile]"},
   { "filter",          PartyTimeFilterCmd,             "filter commands"},
   { "self",            PartySelf,                      "self test"},
   { NULL, NULL, NULL }
};

void cmd_party( CLI_PARSE_INFO *info)
{
   if (firstTime == 1)
   {
//      PartyInit(info);
      TimeLineInit(info);

      ListReset(info, listTimeLine);
      ListReset(info, listTimeOrder);
      numRacesG = 0;

      firstTime = 0;
   }

   cliDefaultHandler( info, cmd_party_commands );
}
