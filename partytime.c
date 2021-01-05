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

//#define CONSOLE_OUTPUT (1)

static volatile int gdbStop = 1;

#define COLUMN_SPACING (2)

#define PRINT_MODE_DISPLAY (1)

#define GROUP_A       (1)
#define GROUP_B       (2)
#define GROUP_ALL     (3)
#define GROUP_UNKNOWN (4)

#define MAX_MY_COLUMNS (1024)

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

typedef struct Columns_s
{
   int width;
} Columns_t;

Columns_t columns[MAX_MY_COLUMNS];

#define MAX_ENTRIES  (1024)

typedef struct PrintEntry_s
{
   int enabled;
   char entry[MAX_STRING_SIZE];
} PrintEntry_t;

PrintEntry_t printTable[MAX_ENTRIES];

/* +++owen - hardcoding to assuming ONLY (4) races taking part in points series */
static int totalRacesIncluded = 4;
static int maxCount = 0;

LinkList_t *listTimeLine = NULL;
LinkList_t *listTimeOrder = NULL;

TimeLinePool_t *timeLinePool;

typedef struct PartyControl_s
{
   int max;
   int bestOf;
   int bonus;
   int doubleUp;
   int ageBonus;
   int debug;
   int groupId;
   int lockDown;
   int clip;
} PartyControl_t;

PartyControl_t partyControl;

//#define DEBUG_LEVEL_1 (1)

#define BEST_OF_ONE   (1)
#define BEST_OF_TWO   (2)
#define BEST_OF_THREE (3)
#define BEST_OF_FOUR  (4)

#define TYPE_USBC    (1)
#define TYPE_USDP    (2)
#define TYPE_UNKNOWN (3)

#define NL_REMOVE (1)
#define NL_KEEP   (2)

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

   timeLineInfo->team[0] = '\0';

   timeLineInfo->name[0] = '\0';
   timeLineInfo->first[0] = '\0';
   timeLineInfo->last[0] = '\0';

   timeLineInfo->points = 0;
   timeLineInfo->groupId = GROUP_UNKNOWN;

   for (i = 0; i < MAX_RACES; i++)
   {
      timeLineInfo->race[i].place = -1;
      timeLineInfo->race[i].points = 0;
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

TimeLineInfo_t *
TimeOrderInfoInsert(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, LinkList_t *list, int race)
{
   Link_t *link;
   Link_t *ptr;
   TimeLineInfo_t *currInfo;
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

            if (currInfo->points <= timeLineInfo->points)
            {
               LinkBefore(list, ptr, link);
               nameFound = 1;
               break;
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

void ReplaceTabs(char *out, char *in)
{
  char *ptrIn = in;
  char *ptrOut = out;
  int firstChar = 1;

  while (*ptrIn != '\0')
  {
     if ((*ptrIn == '\t') && (firstChar == 1))
     {
        /* If <tab> found as first character, just replace with <space> */
        firstChar = 0;
        *ptrOut = ' ';
     }
     else if (*ptrIn == '\t')
     {
        *ptrOut++ = ' ';
        *ptrOut++ = ' ';
        *ptrOut++ = ' ';
        *ptrOut = ' ';
     }
     else
     {
       *ptrOut = *ptrIn;
     }

     ptrIn++;
     ptrOut++;
  }

  *ptrOut = '\0';
}

void AddSpace(char *out, char *in)
{
  char *ptrIn = in;
  char *ptrOut = out;
  char *ptrUpdate = in;
  char left;
  char right;
  char *tmp2;
  int spaceFound = 0;

  while (*ptrIn != '\0')
  {
     if ((*ptrIn == '\t') || (*ptrIn == ' '))
     {
        *ptrOut++ = ' ';
        *ptrOut = ' ';
     }
     else
     {
       *ptrOut = *ptrIn;
     }

     ptrIn++;
     ptrOut++;
  }

  *ptrOut = '\0';
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

void RemoveSpaceAtEnd(char *ptr)
{
   char *ptr2;

   /* Go to end, go back, make sure no spaces at end of name */
   ptr2 = --ptr;
   if (*ptr2 == ' ')
   {
      while (1)
      {
         if (*ptr2 == ' ')
         {
            ptr2--;
         }
         else
         {
            ptr2++;
            *ptr2 = '\0';
            break;
         }
      }
   }
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
   else if ((strstr(tmp, "Korea") != 0) && (strstr(timeLineInfo->name, "Marc") != 0))
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Korea Marc");
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
   else if (strstr(tmp, "Dan Nelson") != 0)
   {
      teamFound = 1;
      strcpy(timeLineInfo->name, "Dan Nelson");
      strcpy(timeLineInfo->team, "¡DUX! TPA-FLA HERD");
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
      strcpy(timeLineInfo->team, "¡DUX! TPA-FLA HERD");
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
      strcpy(timeLineInfo->team, "¡DUX! TPA-FLA - HERD");
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
      strcpy(timeLineInfo->team, "¡DUX! TPA-FLA HERD");
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
   else if ((strstr(timeLineInfo->name, "Bergenzaun") != 0) && (strstr(timeLineInfo->name, "ns ") != 0))
   {
      strcpy(timeLineInfo->name, "Mans Bergenzaun");
      strcpy(timeLineInfo->team, "TEAM BTC");
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
      strcpy(timeLineInfo->team, "¡DUX! TPA-FLA HERD");
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
      strcpy(timeLineInfo->team, "¡DUX! TPA-FLA - HERD");
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

   if (partyControl.max != -1)
   {
      if (count > partyControl.max)
      {
         (pInfo->print_fp)("WARNING!!!!! WARNING!!!! you set max = %d, found count = %d\n", partyControl.max, count);
         partyControl.max = count;
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
         timeLineInfo->race[raceId].points = (count - timeLineInfo->race[raceId].place);
         timeLineInfo->race[raceId].points = (int)((float)timeLineInfo->race[raceId].points * fudge);

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

   if (partyControl.groupId == GROUP_A)
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

#if 0
         if ((strstr(athleteAll[i].name, "McGonagle") != 0) && (partyControl.lockDown == 1))
         {
            if (strstr(timeLineInfo->name, "McGonagle") != 0)
            {
               (pInfo->print_fp)("FYI: last name matches = %s vs. %s\n",
                                 athleteAll[i].name, timeLineInfo->name);
            }
         }
#endif

//         if ((strstr(athleteAll[i].name, timeLineInfo->last) != 0) && (strlen(athleteAll[i].name) == strlen(timeLineInfo->last)))
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
#if 0
            if (partyControl.lockDown == 1)
            {
               (pInfo->print_fp)("INFO: NOT SKIPPING %s\n", timeLineInfo->name);
            }
#endif
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


void ColumnStore(CLI_PARSE_INFO *pInfo, char *str)
{
   int i, j;
   int idx = 0;
   int count = 0;
   int endCount = 0;
   char *ptr;

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

void ColumnPrintfHeader(CLI_PARSE_INFO *pInfo, char *str)
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
   char outStr[MAX_STRING_SIZE];

   ptr = &str[0];
   i = 0;
   columnId = 0;
   columnWidth = 0;

   while (*ptr != '\0')
   {
      if (columnId == 0)
      {
         outStr[outIdx++] = '+';
         outStr[outIdx++] = '-';
      }

      idx = 0;
      while ((*ptr != ' ') && (*ptr != '\0'))
      {
         outStr[outIdx++] = '-';
         ptr++;
         columnWidth += 1;
      }

      if (*ptr == '\0')
      {
         outStr[outIdx++] = '-';
         endWidth = columnWidth;
         break;
      }

      outStr[outIdx++] = '-';
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
            outStr[outIdx++] = '-';
            endWidth = columnWidth;
            break;
         }

         /* FOUND two spaces! */
         if ((columns[columnId].width + COLUMN_SPACING) > columnWidth)
         {
            fill = ((columns[columnId].width + COLUMN_SPACING) - columnWidth);
            for (i = 0; i < fill; i++)
            {
               outStr[outIdx++] = '-';
            }
         }

         outStr[outIdx++] = '+';
         outStr[outIdx++] = '-';

         if (*ptr != '\0')
         {
            columnWidth = 0;
            columnId += 1;
         }
      }
   }

#if 1
   if (*ptr != '\0')
   {
      (pInfo->print_fp)("INTERNAL ERROR: end of str not found!!!\n");
   }

   if (partyControl.debug == 1)
   {
      (pInfo->print_fp)("columnId = %d, columns[%d].width = %d, endWidth = %d\n", columnId,
                        columnId, columns[columnId].width, endWidth);
   }

   if (1) /* columns[columnId].width > COLUMN_SPACING) */
   {
      if ((columns[columnId].width) > endWidth)
      {
         fill = (columns[columnId].width - endWidth);
         for (i = 0; i < fill; i++)
         {
            outStr[outIdx++] = '-';
         }
      }
   }
#endif

#if 1
   outIdx -= 1;
   outStr[outIdx++] = '|';
   outStr[outIdx++] = '\0';
#endif

   (pInfo->print_fp)("%s\n", outStr);

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
   char outStr[MAX_STRING_SIZE];

   ptr = &str[0];
   i = 0;
   columnId = 0;
   columnWidth = 0;

   while (*ptr != '\0')
   {
      if (columnId == 0)
      {
         outStr[outIdx++] = '|';
         outStr[outIdx++] = ' ';
      }

      idx = 0;
      while ((*ptr != ' ') && (*ptr != '\0'))
      {
         outStr[outIdx++] = *ptr;
         ptr++;
         columnWidth += 1;
      }

      if (*ptr == '\0')
      {
         outStr[outIdx++] = ' ';
         endWidth = columnWidth;
         break;
      }

      outStr[outIdx++] = *ptr;
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
            outStr[outIdx++] = ' ';
            endWidth = columnWidth;
            break;
         }

         /* FOUND two spaces! */
         if ((columns[columnId].width + COLUMN_SPACING) > columnWidth)
         {
            fill = ((columns[columnId].width + COLUMN_SPACING) - columnWidth);
            for (i = 0; i < fill; i++)
            {
               outStr[outIdx++] = ' ';
            }
         }

         outStr[outIdx++] = '|';
         outStr[outIdx++] = ' ';

         if (*ptr != '\0')
         {
            columnWidth = 0;
            columnId += 1;
         }
      }
   }

#if 1
   if (*ptr != '\0')
   {
      (pInfo->print_fp)("INTERNAL ERROR: end of str not found!!!\n");
   }

   if (partyControl.debug == 1)
   {
      (pInfo->print_fp)("columnId = %d, columns[%d].width = %d, endWidth = %d\n", columnId,
                        columnId, columns[columnId].width, endWidth);
   }

   if (1) /* columns[columnId].width > COLUMN_SPACING) */
   {
      if ((columns[columnId].width) > endWidth)
      {
         fill = (columns[columnId].width - endWidth);
         for (i = 0; i < fill; i++)
         {
            outStr[outIdx++] = ' ';
         }
      }
   }
#endif

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
   int idx = 0;

   if (printTable[1].enabled != 0)
   {
      ColumnPrintf(pInfo, printTable[idx].entry);
      idx += 1;
   }

   if ((printTable[idx].enabled != 0) && (strstr(printTable[idx].entry, "--") != 0))
   {
      ColumnPrintfHeader(pInfo, printTable[1].entry);
      idx += 1;
   }

   for (i = idx; i < MAX_ENTRIES; i++)
   {
      if (printTable[i].enabled == 0)
      {
         break;
      }

      ColumnPrintf(pInfo, printTable[i].entry);
   }
}


void RemoveWpkg(CLI_PARSE_INFO *pInfo, char *str)
{
   char *p;
   if ((p = strstr(str, "wkg")) != NULL)
   {
      *p = '\0';
   }
}

void FixLine(CLI_PARSE_INFO *pInfo, char *out, char *in, char *team)
{
   char *ptr, *ptr2;
   int idx;
   char *ptrIn = in;
   char *ptrOut = out;
   char tmp[MAX_STRING_SIZE];
   char teamName[MAX_STRING_SIZE];
   int teamFound = 0;
   int ret = 0;
   int nameFound = 0;
   
#if 0
   while (*ptrIn != '\0')
   {
      ptrIn++;
      ptrOut++;
   }
#endif

   /*
    * Format is:
    *
    * <first> <last> <date>
    */

   /* If we find anything other, fix it
    * 1   Andrew Toftoy   Jan 1, 2021     12.5mi/h    -   425W    1,929.8     6:09
    * 2   Antonio Ferrari     Jan 1, 2021     11.7mi/h    178bpm  404W    1,802.7     6:35
    * 3   Gabriel Mathisen Ⓥ  Jan 1, 2021     11.6mi/h    174bpm  370W    1,784.7     6:39
    * 42  Rolf Smit | FWC     Jan 1, 2021     8.9mi/h     164bpm  403W    1,377.3     8:37
    * 48  Marian von Rappard | SUPER VISION LAB   Jan 1, 2021     8.8mi/h     -   317W    1,364.1     8:42
    * 61  まさる みうら   Jan 1, 2021     8.5mi/h     170bpm  271W    1,313.8     9:02
    * 68  Geir Erling Nilsberg    Jan 1, 2021     8.3mi/h     152bpm  307W    1,280.7     9:16
    * 68  Luís Carvalhinho 🇵🇹     Jan 1, 2021     8.3mi/h     -   290W    1,280.7     9:16
    * 77  Wouter Jan Kleinlugtenbelt  Jan 1, 2021     8.1mi/h     157bpm  339W    1,255.9     9:27
    * 83  Vito Peló Team Loda Millennium 🇮🇹   Jan 1, 2021     8.0mi/h     162bpm  252W    1,232.0     9:38
    * 94  Stéphane Negreche(SNC⚡) 🇫🇷     Jan 1, 2021     7.9mi/h     169bpm  293W    1,211.0     9:48
    * 102     George Cooper (Sherwood Pines Cycles Forme)     Jan 1, 2021     7.7mi/h     199bpm  157W    1,192.8     9:57
    * 117     Åsa Fast-Berglund [SZ] Team Ljungskog   Jan 1, 2021     7.5mi/h     153bpm  242W    1,156.0     10:16
    * 130     🇪🇸 Miguel Ángel Lora 🇪🇸     Jan 1, 2021     7.4mi/h     149bpm  285W    1,141.2     10:24
    * 133     Mike Park(BeeTriathlonCoaching.com)     Jan 1, 2021     7.4mi/h     162bpm  275W    1,135.7     10:27
    * 145     Bike ironmike COX   Jan 1, 2021     7.2mi/h     157bpm  288W    1,105.7     10:44
    * 155     大野 洸     Jan 1, 2021     7.1mi/h     -   208W    1,087.1     10:55
    * 168     Eiji T  Jan 1, 2021     6.9mi/h     128bpm  282W    1,056.5     11:14
    * 192     Pedro Abio Ruiz 🇪🇦  Jan 1, 2021     6.4mi/h     164bpm  262W    986.3   12:02
    * 215     David Miles (DIRT)  Jan 1, 2021     6.1mi/h     183bpm  215W    934.5   12:42
    * 234     Lee Mardon (BIKE DOCTOR CC)     Jan 1, 2021     5.3mi/h     163bpm  244W    813.8   14:35
    * 254     Tami Baca (she,her,hers)    Jan 1, 2021     4.2mi/h     -   141W    643.3   18:27
    */
 
   /* Clean it up a bit */
   strcpy(tmp, in);
   ptr = &tmp[0];

   /* Convert non-printable characters */
   while (*ptr != '\0')
   {
      if (!isprint(*ptr) && (*ptr != '\0') && (*ptr != '\r') && (*ptr != '\t') && (*ptr != '\n'))
      {
         // *ptr = ' ';
         *ptr = '_';
      }
      else if (!isprint(*ptr) && ((*ptr == '\0') || (*ptr == '\r') || (*ptr == '\t') || (*ptr != '\n')))
      {
         (pInfo->print_fp)("ERROR: crazy line?: %s\n", in);
      }
      else if  ((*ptr == '\r') || (*ptr == '\t') || (*ptr == '|') || (*ptr == '(') || (*ptr == ')') || (*ptr == '[') || (*ptr == ']') ||
                (*ptr == '@') || (*ptr == '#'))
      {
         *ptr = ' ';
      }
      ptr++;
   }

   /* Find first digit */
   ptr = &tmp[0];
   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      if (isdigit(*ptr))
      {
         break;
      }
      ptr++;
   }

   if (*ptr == '\0')
   {
      (pInfo->print_fp)("ERROR: could not find first index: %s\n", in);
   }

   /* Get rest of digits */
   ptr++;
   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      if (*ptr == ' ')
      {
         break;
      }
      ptr++;
   }

   *ptrOut++ = ' ';

   if (*ptr == '\0')
   {
      (pInfo->print_fp)("ERROR: could not find <space> after first index: %s\n", in);
   }

   /* 3   Gabriel Mathisen __ Jan 1, 2021     11.6mi/h    174bpm  370W    1,784.7     6:39 */

   /* If third token is not a Month (Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec) */

   /* Get to start of first name */
   ptr++;
   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      if (*ptr != ' ')
      {
         break;
      }
      ptr++;
   }

   if (*ptr == '\0')
   {
      (pInfo->print_fp)("ERROR: could not find <first name> after first index: %s\n", in);
   }
   
   /* Get first name */
   ptr++;
   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      if (*ptr == ' ')
      {
         break;
      }
      ptr++;
   }

   if (*ptr == '\0')
   {
      (pInfo->print_fp)("ERROR: could not find <first name> after first index: %s\n", in);
   }

   /* if more than a space, ERROR */
   ptr++;
   if (*ptr == ' ')
   {
      (pInfo->print_fp)("ERROR: too many spaces after first name: %s\n", in);
   }

   *ptrOut++ = *ptr;

   /* Get last name */
   ptr++;
   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      if (*ptr == ' ')
      {
         break;
      }
      ptr++;
   }

   /* Get to start of team */
   ptr++;
   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      if (*ptr != ' ')
      {
         break;
      }
      ptr++;
   }

   if (*ptr == '\0')
   {
      (pInfo->print_fp)("ERROR: could not find <first name> after first index: %s\n", in);
   }

   sprintf(team, "%s", "-");

   /* Find month - then clear in between */
   if (((ptr2 = strstr(ptr, "Jan")) != NULL) ||
       ((ptr2 = strstr(ptr, "Feb")) != NULL) ||
       ((ptr2 = strstr(ptr, "Mar")) != NULL) ||
       ((ptr2 = strstr(ptr, "Apr")) != NULL) ||
       ((ptr2 = strstr(ptr, "May")) != NULL) ||
       ((ptr2 = strstr(ptr, "Jun")) != NULL) ||
       ((ptr2 = strstr(ptr, "Jul")) != NULL) ||
       ((ptr2 = strstr(ptr, "Aug")) != NULL) ||
       ((ptr2 = strstr(ptr, "Sep")) != NULL) ||
       ((ptr2 = strstr(ptr, "Oct")) != NULL) ||
       ((ptr2 = strstr(ptr, "Nov")) != NULL) ||
       ((ptr2 = strstr(ptr, "Dec")) != NULL))
   {
      if (ptr != ptr2)
      {
         teamFound = 1;

         /* Fix initial char copied */
         ptrOut--;
         *ptrOut++ = ' ';
         *ptrOut++ = ' ';

         /* Set all in between to space - attach to a team attempt */
         idx = 0;
         while (ptr != ptr2)
         {
            teamName[idx++] = *ptr;
            *ptr = ' ';
            ptr++;
         }
         idx--;
         teamName[idx] = '\0';
         strcpy(team, teamName);
      }
      else
      {
         ptrOut--;
         *ptrOut++ = ' ';
         *ptrOut++ = ' ';
      }
   }
   else
   {
      (pInfo->print_fp)("ERROR: COULD not find month!!! %s\n", in);
   }


   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      ptr++;
   }

#if 1
   ptrOut--;
   *ptrOut = '\0';
#else
   *ptrOut = '\0';
#endif
}

void FixTeamName(CLI_PARSE_INFO *pInfo, char *out, char *in)
{
   char *ptr, *ptr2;
   int idx;
   char *ptrIn = in;
   char *ptrOut = out;
   char tmp[MAX_STRING_SIZE];
   char teamName[MAX_STRING_SIZE];
   int teamFound = 0;
   int ret = 0;
   int nameFound = 0;
   
#if 0
   while (*ptrIn != '\0')
   {
      ptrIn++;
      ptrOut++;
   }
#endif

   /*
    * Format is:
    *
    * <first> <last> <date>
    */

   /* If we find anything other, fix it
    * 1   Andrew Toftoy   Jan 1, 2021     12.5mi/h    -   425W    1,929.8     6:09
    * 2   Antonio Ferrari     Jan 1, 2021     11.7mi/h    178bpm  404W    1,802.7     6:35
    * 3   Gabriel Mathisen Ⓥ  Jan 1, 2021     11.6mi/h    174bpm  370W    1,784.7     6:39
    * 42  Rolf Smit | FWC     Jan 1, 2021     8.9mi/h     164bpm  403W    1,377.3     8:37
    * 48  Marian von Rappard | SUPER VISION LAB   Jan 1, 2021     8.8mi/h     -   317W    1,364.1     8:42
    * 61  まさる みうら   Jan 1, 2021     8.5mi/h     170bpm  271W    1,313.8     9:02
    * 68  Geir Erling Nilsberg    Jan 1, 2021     8.3mi/h     152bpm  307W    1,280.7     9:16
    * 68  Luís Carvalhinho 🇵🇹     Jan 1, 2021     8.3mi/h     -   290W    1,280.7     9:16
    * 77  Wouter Jan Kleinlugtenbelt  Jan 1, 2021     8.1mi/h     157bpm  339W    1,255.9     9:27
    * 83  Vito Peló Team Loda Millennium 🇮🇹   Jan 1, 2021     8.0mi/h     162bpm  252W    1,232.0     9:38
    * 94  Stéphane Negreche(SNC⚡) 🇫🇷     Jan 1, 2021     7.9mi/h     169bpm  293W    1,211.0     9:48
    * 102     George Cooper (Sherwood Pines Cycles Forme)     Jan 1, 2021     7.7mi/h     199bpm  157W    1,192.8     9:57
    * 117     Åsa Fast-Berglund [SZ] Team Ljungskog   Jan 1, 2021     7.5mi/h     153bpm  242W    1,156.0     10:16
    * 130     🇪🇸 Miguel Ángel Lora 🇪🇸     Jan 1, 2021     7.4mi/h     149bpm  285W    1,141.2     10:24
    * 133     Mike Park(BeeTriathlonCoaching.com)     Jan 1, 2021     7.4mi/h     162bpm  275W    1,135.7     10:27
    * 145     Bike ironmike COX   Jan 1, 2021     7.2mi/h     157bpm  288W    1,105.7     10:44
    * 155     大野 洸     Jan 1, 2021     7.1mi/h     -   208W    1,087.1     10:55
    * 168     Eiji T  Jan 1, 2021     6.9mi/h     128bpm  282W    1,056.5     11:14
    * 192     Pedro Abio Ruiz 🇪🇦  Jan 1, 2021     6.4mi/h     164bpm  262W    986.3   12:02
    * 215     David Miles (DIRT)  Jan 1, 2021     6.1mi/h     183bpm  215W    934.5   12:42
    * 234     Lee Mardon (BIKE DOCTOR CC)     Jan 1, 2021     5.3mi/h     163bpm  244W    813.8   14:35
    * 254     Tami Baca (she,her,hers)    Jan 1, 2021     4.2mi/h     -   141W    643.3   18:27
    */
 
   /* Clean it up a bit */
   strcpy(tmp, in);
   ptr = &tmp[0];

   if (*ptr == '-')
   {
      *ptrOut++ = '-';
      *ptrOut = '\0';
      return;
   }

   /* Convert non-printable characters */
   while (*ptr != '\0')
   {
      if (!isprint(*ptr) && (*ptr != '\0') && (*ptr != '\r') && (*ptr != '\t') && (*ptr != '\n'))
      {
         // *ptr = ' ';
         *ptr = '_';
      }
      else if (!isprint(*ptr) && ((*ptr == '\0') || (*ptr == '\r') || (*ptr == '\t') || (*ptr != '\n')))
      {
         (pInfo->print_fp)("ERROR: crazy line?: %s\n", in);
      }
      else if  ((*ptr == '\r') || (*ptr == '\t') || (*ptr == '|') || (*ptr == '(') || (*ptr == ')') || (*ptr == '[') || (*ptr == ']') ||
                (*ptr == '@') || (*ptr == '#'))
      {
         *ptr = ' ';
      }
      ptr++;
   }

   /* Fix duplicate spaces */
   ptr = &tmp[0];
   while (*ptr != '\0')
   {
      *ptrOut++ = *ptr;
      if (*ptr == ' ')
      {
         ptr++;
         while (*ptr != '\0')
         {
            if (*ptr != ' ')
            {
               break;
            }
            ptr++;
         }
      }
      else if (*ptr == '_')
      {
         ptr++;
         while (*ptr != '\0')
         {
            if (*ptr != '_')
            {
               break;
            }
            ptr++;
         }
      }

      if (*ptr != '\0')
      {
         ptr++;
      }
   }

#if 1
   ptrOut--;
   *ptrOut = '\0';
#else
   *ptrOut = '\0';
#endif
}

void cmd_party_common(CLI_PARSE_INFO *pInfo, int partyMode)
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
   int raceId = 0;
   int nlMode = NL_REMOVE;
   int goIntoLoop = 0;
   int partyAlreadyOneTime = 0;
   int continueInLoop = 1;

   FILE *fp_in;
   FILE *fp_out;
   char infile[MAX_STRING_SIZE];
   char outfile[MAX_STRING_SIZE];

   char tmp[MAX_STRING_SIZE];
   char tmp2[MAX_STRING_SIZE];
   char tmp3[MAX_STRING_SIZE];
   char raceName[MAX_STRING_SIZE];
   char tmpName[MAX_STRING_SIZE];
   char updatedLine[MAX_STRING_SIZE];

   char timeLine[MAX_STRING_SIZE];

   Link_t *ptr, *ptr2;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *timeLineInfo2;
   int groupId = GROUP_ALL;

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

   int outIdx = 0;
   char outStr[MAX_STRING_SIZE];

   ColumnReset(pInfo);

   // fp_in = fopen("zwiftpower.txt", "r");
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
   sprintf(header, "%s", "#  name   team   time   watts  wpkg");
   ColumnStore(pInfo, header);

   sprintf(header, "%s", "--  ----   ----   ----   -----  ----");
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
      if ((partyMode & PARTY_KOM_SPRINTS) ||
          (partyMode & PARTY_RESULTS) ||
          (partyMode & PARTY_RESULTS_2))
      {
         if (((timeLine[0] == 'A') || (timeLine[0] == 'B') || (timeLine[0] == 'C') || (timeLine[0] == 'D')) &&
             (((timeLine[1] == ' ') && (timeLine[2] == ' ')) || (timeLine[1] == '\n')) &&
             (strstr(timeLine, "Distance") == NULL))
         {
            if (partyControl.groupId == GROUP_ALL)
            {
               groupId = GROUP_ALL;
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
         groupId = GROUP_ALL;
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
                  ((strstr(timeLine, ", 2021") != NULL) || (strstr(timeLine, ", 2021") != NULL)))
         {
            if (partyAlreadyOneTime != 1)
            {
               (pInfo->print_fp)("INTERNAL ERROR: going back in, but for first time?\n");
            }

            /* We have Already been in ONE TIME, stay in there */
            groupId = GROUP_ALL;
            goIntoLoop = 1;
         }
      }
      else
      {
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
            if (!StringGet(tmp, fp_in, nlMode))
            {
               /* Done reading file */
               break;
            }

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

               NameInsert(timeLineInfo, name);

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

               if (partyControl.lockDown == 1)
               {
                  if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                  {
                     continueInLoop = 1;
                     continue;
                  }
               }

#if 1
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
               count++;
#endif

               TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

               outIdx += sprintf(&outStr[outIdx], "%s   %s", fixed2, teamName2);
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
                  if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_ALL))
                  {
                     timeLineInfo->groupId = groupId;
                  }
               }

#if 1
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
#endif

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

                     if (partyControl.lockDown == 1)
                     {
                        if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                        {
                           continueInLoop = 1;
                           continue;
                        }
                     }

                     NameInsert(timeLineInfo, name);

                     if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_ALL))
                     {
                        timeLineInfo->groupId = groupId;
                     }

                     if (RacedToday(name, teamName))
                     {
                        TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

                        sprintf(timeLineInfo->team, "%s", teamName);
                        sprintf(place, "%d ", count);
                        count++;

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

                     if ((timeLineInfo->groupId == GROUP_UNKNOWN) || (timeLineInfo->groupId == GROUP_ALL))
                     {
                        timeLineInfo->groupId = groupId;
                     }

                     if (RacedToday(name, teamName))
                     {
                        TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

                        sprintf(timeLineInfo->team, "%s", teamName);
                        sprintf(place, "%d", count);
                        count++;

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

                     TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);

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

                     if (partyControl.lockDown == 1)
                     {
                        if (AthleteSkip(pInfo, listTimeLine, timeLineInfo, groupId))
                        {
                           continueInLoop = 0;
                           break;
                        }
                     }

                     TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);
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

                  AddSpace(tmp2, tmp);
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("%s", tmp2);
#else
                  outIdx += sprintf(&outStr[outIdx], "%s", tmp2);
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

                  AddSpace(tmp2, tmp);
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
                  outIdx += sprintf(&outStr[outIdx], "%s", tmp2);
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

int BestOf(TimeLineInfo_t *timeLineInfo, int count, int *total)
{
   int i;
   int id0, id1, id2, id3;
   int order[4];
   int ret;
   int numRaces = 0;

   id0 = timeLineInfo->race[0].points;
   id1 = timeLineInfo->race[1].points;
   id2 = timeLineInfo->race[2].points;
   id3 = timeLineInfo->race[3].points;

   order[0] = id0;
   order[1] = id1;
   order[2] = id2;
   order[3] = id3;

   for (i = 0; i < totalRacesIncluded; i++)
   {
      if (timeLineInfo->race[i].place > 0)
      {
         numRaces += 1;
      }
   }

   *total = numRaces;

   order[0] = id0;
   if (id1 > id0)
   {
      order[1] = id0;
      order[0] = id1;
   }
   else
   {
      order[1] = id1;
   }

   if (id2 > order[0])
   {
      order[2] = order[1];
      order[1] = order[0];
      order[0] = id2;
   }
   else if (id2 > order[1])
   {
      order[2] = order[1];
      order[1] = id2;
   }
   else if (id2 > order[2])
   {
      order[3] = order[2];
      order[2] = id2;
   }

   if (id3 > order[0])
   {
      order[3] = order[2];
      order[2] = order[1];
      order[1] = order[0];
      order[0] = id3;
   }
   else if (id3 > order[1])
   {
      order[3] = order[2];
      order[2] = order[1];
      order[1] = id3;
   }
   else if (id3 > order[2])
   {
      order[3] = order[2];
      order[2] = id3;
   }

   if (count == 1)
   {
      ret = order[0];
   }
   else if (count == 2)
   {
      ret = (order[0] + order[1]);
   }
   else if (count == 3)
   {
      ret = (order[0] + order[1] + order[2]);
   }
   else if (count == 4)
   {
      ret = (order[0] + order[1] + order[2] + order[3]);
   }

   return (ret);
}


void cmd_party_show(CLI_PARSE_INFO *pInfo)
{
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *currInfo;
   Link_t *link;
   Link_t *ptr;
   int i;
   int total;
   char placePoints[MAX_STRING_SIZE];
   char outStr[MAX_STRING_SIZE];
   char header[MAX_STRING_SIZE];

   int outIdx = 0;
   int count = 1;

   ColumnReset(pInfo);

   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;

      currInfo = TimeLineInfoNew(pInfo, TYPE_USBC);
      // currInfo->points = timeLineInfo->points;
      if (partyControl.bestOf == BEST_OF_ONE)
      {
         currInfo->points = BestOf(timeLineInfo, 1, &total);
      }
      else if (partyControl.bestOf == BEST_OF_TWO)
      {
         currInfo->points = BestOf(timeLineInfo, 2, &total);
      }
      else if (partyControl.bestOf == BEST_OF_THREE)
      {
         currInfo->points = BestOf(timeLineInfo, 3, &total);
      }
      else if (partyControl.bestOf == BEST_OF_FOUR)
      {
         currInfo->points = BestOf(timeLineInfo, 4, &total);
      }
      else
      {
         (pInfo->print_fp)("WARNING!!! FORCING best-of-2\n");
         currInfo->points = BestOf(timeLineInfo, 2, &total);
      }

      if (partyControl.ageBonus == 1)
      {
         if (strstr(timeLineInfo->name, "McGonagle") != 0)
         {
            currInfo->points += 5;
         }
      }

      if (partyControl.bonus == 1)
      {
         if (total == 4)
         {
            currInfo->points += 10;
         }
         else if (total == 3)
         {
            currInfo->points += 5;
         }
      }

      currInfo->me = timeLineInfo;

      TimeOrderInfoInsert(pInfo, currInfo, listTimeOrder, RACE_NO);
      ptr = ptr->next;
   }

#ifndef CONSOLE_OUTPUT
   sprintf(header, "%s", "#  name   team   race1  race2  race3  race4  plc/pts");
   ColumnStore(pInfo, header);

   sprintf(header, "%s", "-  ----   ----   ---  ----  -----  ----  -------");
   ColumnStore(pInfo, header);
#endif

   ptr = (Link_t *)listTimeOrder->head;
   while (ptr->next != NULL)
   {
      outIdx = 0;
      currInfo = (TimeLineInfo_t *)ptr->currentObject;

      timeLineInfo = (TimeLineInfo_t *)currInfo->me;

      if ((partyControl.clip == 1) && (timeLineInfo->numRaces < partyControl.bestOf))
      {
         ptr = ptr->next;
         continue;
      }

#if 0
      if (partyControl.groupId != timeLineInfo->groupId)
      {
            ptr = ptr->next;
            continue;
      }
#endif

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

      for (i = 0; i < totalRacesIncluded; i++)
      {
         if (timeLineInfo->race[i].place != -1)
         {
            sprintf(placePoints, "%2d/%-2d", timeLineInfo->race[i].place, timeLineInfo->race[i].points);
#ifdef CONSOLE_OUTPUT
            (pInfo->print_fp)(" %-10s ", placePoints);
#else
            outIdx += sprintf(&outStr[outIdx], " %-10s ", placePoints);
#endif
         }
         else
         {
            sprintf(placePoints, "%2d/%-2d", 0, 0);
#ifdef CONSOLE_OUTPUT
            (pInfo->print_fp)(" %-10s ", placePoints);
#else
            outIdx += sprintf(&outStr[outIdx], " %-10s ", placePoints);
#endif
         }
      }


#ifdef CONSOLE_OUTPUT
      (pInfo->print_fp)(" %-10d ", currInfo->points);
      (pInfo->print_fp)("\n");
#else
      outIdx += sprintf(&outStr[outIdx], " %-10d", currInfo->points);
      ColumnStore(pInfo, outStr);
#endif
      ptr = ptr->next;
   }

#ifdef CONSOLE_OUTPUT
   (pInfo->print_fp)("\n\n");
#else
   PrintTable(pInfo);
   (pInfo->print_fp)("\n\n");
#endif

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
      (pInfo->print_fp)("USAGE: %s {A|B|All}}\n", pInfo->argv[0]);
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
    else
    {
        (pInfo->print_fp)("USAGE: %s {A|B|All}\n", pInfo->argv[0]);
        return;
    }
}

void PartyInit(CLI_PARSE_INFO *pInfo)
{
   int i;

   partyControl.max = -1;
   partyControl.bestOf = BEST_OF_ONE;
   partyControl.bonus = 0;
   partyControl.doubleUp = 0;
   partyControl.ageBonus = 0;
   partyControl.debug = 0;
   partyControl.groupId = GROUP_UNKNOWN;
   partyControl.lockDown = 0;
   partyControl.clip = 0;

   ListReset(pInfo, listTimeLine);
   ListReset(pInfo, listTimeOrder);

   ColumnReset(pInfo);
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

//   PartyForceShow(pInfo);
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

void PartySetSnapReset(CLI_PARSE_INFO *pInfo)
{
   int i;

   for (i = 0; i < MAX_ATHLETES; i++)
   {
      athleteA[i].enabled = 0;
      athleteB[i].enabled = 0;
      athleteAll[i].enabled = 0;
   }
}

static const CLI_PARSE_CMD party_set_snap_cmd[] =
{
    { "A",      PartySetSnapA,    "snap A atheletes"},
    { "B",      PartySetSnapB,    "snap B atheletes"},
    { "All",    PartySetSnapAll,  "snap All atheletes"},
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
    { "bonus",     PartySetBonus,     "on|off - 5 pts for 3 races, 10 pts for 4 races"},
    { "double",    PartySetDouble,    "on|off - DOUBLE points for Alpe race (warranted)"},
    { "ageBonus",  PartySetAgeBonus,  "on|off - extra 5 points for being 55+"},
    { "debug",     PartySetDebug,     "on|off"},
    { "group",     PartySetGroup,     "A|B|All (unknown is default)"},
    { "lockdown",  PartySetLockdown,  "on|off"},
    { "clip",      PartySetClip,      "on|off"},
    { "snap",      PartySetSnap,      "snap athletes"},
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
   }

   strcpy(athleteA[i-1].name, "ATHLETE_END");
   strcpy(athleteB[i-1].name, "ATHLETE_END");
   strcpy(athleteAll[i-1].name, "ATHLETE_END");
}

void ColumnReset(CLI_PARSE_INFO *pInfo)
{
   int i;

   for (i = 0; i < MAX_MY_COLUMNS; i++)
   {
      columns[i].width = -1;
   }

   for (i = 0; i < MAX_ENTRIES; i++)
   {
      printTable[i].enabled = 0;
   }
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
   /* Reset things each time through */
   ListReset(info, listTimeLine);
   ListReset(info, listTimeOrder);
   PartyInit(info);
   ColumnReset(info);
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

}

static const CLI_PARSE_CMD party_force_cmd[] =
{
   { "show",    PartyForceShow, "show force athlete lists"},
   { "A",       PartyForceA,    "{athlete}"},
   { "B",       PartyForceB,    "{athlete}"},
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

#else

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

static const CLI_PARSE_CMD cmd_party_commands[] =
{
   { "start",           cmd_party_start,                "start party"},
   { "stop",            cmd_party_stop,                 "stop party"},
   { "reset",           cmd_party_reset,                "reset party"},
   { "show",            cmd_party_show,                 "show party status"},
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
      PartyInit(info);
      TimeLineInit(info);
      firstTime = 0;
   }

   cliDefaultHandler( info, cmd_party_commands );
}
