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

#define CONSOLE_OUTPUT (1)

#define GROUP_A    (1)
#define GROUP_B    (2)
#define GROUP_ALL  (3)   

#define PARTY_KOM_SPRINTS (1)
#define PARTY_FOLLOWING   (2)
#define PARTY_FOLLOWING_2 (3)
#define PARTY_RESULTS     (4)
#define PARTY_RESULTS_2   (5)

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
   int group;
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

extern void PartyTimeFilterCmd( CLI_PARSE_INFO *pInfo);
extern int TeamGrit(char *name);
extern int RacedToday(char *name, char *teamName);
extern void RidersMissed(CLI_PARSE_INFO *info);

void TeamNameCleanup(TimeLineInfo_t *timeLineInfo, char *team);
int PartyForceAthleteA(CLI_PARSE_INFO *info, char *name);


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
}

void PartyReset(CLI_PARSE_INFO *pInfo, LinkList_t *list)
{
   Link_t *ptr;
   Link_t *tmp;
    
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
         *ptr = ' ';
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
   else if (strstr(tmp, "Rafa Dzieko") != 0)
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
   else if (strstr(timeLineInfo->name, "John Jeffries") != 0)
   {
      strcpy(timeLineInfo->name, "John Jeffries");
      strcpy(timeLineInfo->team, "[AA Bikes][GRIT]");
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
   else if ((strstr(timeLineInfo->name, "J B") != 0) && (strstr(timeLineInfo->team, "SWOZR") != 0))
   {
      strcpy(timeLineInfo->name, "J B #1");
      strcpy(timeLineInfo->team, "SWOZR");
   }
   else if ((strstr(timeLineInfo->name, "J B") != 0) && (strstr(timeLineInfo->team, "Bees") != 0))
   {
      strcpy(timeLineInfo->name, "J B #2");
      strcpy(timeLineInfo->team, "BCC - Bees");
   }
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
   else if (strstr(timeLineInfo->name, "Rafa Dzieko") != 0)
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
   int group = GROUP_ALL;

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
   char bpm[MAX_STRING_SIZE];
   char vid[MAX_STRING_SIZE];
   char time[MAX_STRING_SIZE];
   char test[MAX_STRING_SIZE];
   char teamName[MAX_STRING_SIZE];
   
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
        
   idx = 0;
   while (1)
   {
      if (!StringGet(tmp, fp_in, NL_KEEP))
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
      if ((partyMode == PARTY_KOM_SPRINTS) || 
          (partyMode == PARTY_RESULTS) ||
          (partyMode == PARTY_RESULTS_2))
      {
         if (((timeLine[0] == 'B') || (timeLine[0] == 'A')) && 
             (((timeLine[1] == ' ') && (timeLine[2] == ' ')) || (timeLine[1] == '\n')) &&
             (strstr(timeLine, "Distance") == NULL))
         {
#if 1
            if ((timeLine[0] == 'B') && 
                (partyControl.group == GROUP_A))
            {
               goIntoLoop = 0;
            }
            else if ((timeLine[0] == 'A') && 
                     (partyControl.group == GROUP_B))
            {
               goIntoLoop = 0;
            }
            else
#endif
            {
               goIntoLoop = 1;
            }
         }
      }
      else if ((partyMode == PARTY_FOLLOWING) || (partyMode == PARTY_FOLLOWING_2))
      {
         if (strstr(timeLine, "Rank ") != NULL)
         {
            goIntoLoop = 1;
            if (strstr(timeLine, "VAM ") != NULL)
            {
               includeVAM = 1;
            }
         }
      }
      else
      {
         goIntoLoop = 1;
      }

      if (goIntoLoop == 1)
      {
         while (continueInLoop == 1)
         {
            if ((partyMode == PARTY_FOLLOWING) || (partyMode == PARTY_FOLLOWING_2))
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

            if (partyMode == PARTY_FOLLOWING)
            {
               if ((tmp[0] == '\n') || (tmp[0] == ' ') || (tmp[0] == '\r') || (tmp[0] == '\t'))
               {
                  continueInLoop = 0;
                  break;
               }

               if (includeVAM == 1)
               {
                  /* 1    Lance Anderson  Sep 8, 2020     21.2mi/h    170   431W    1,345.5     4:42 */
                  sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s %s",
                         place, first, last, month, day, year, speed, bpm, watts, vid, time);

                  sprintf(name, "%s ", first);
                  strcat(name, last);

                  (pInfo->print_fp)("%s   %s %s   %s %s %s   %s   %s\n",
                                    place, first, last, month, day, year, watts, time);

                  fprintf(fp_out, "%s   %s %s   %s %s %s   %s   %s\n",
                          place, first, last, month, day, year, watts, time);
               }
               else
               {
                  /* 1       Steve Peplinski 	Feb 28, 2019 	46.1mi/h 	169bpm 	736W 	9s */
                  sprintf(vid, "%s", "0");
                  sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s",
                         place, first, last, month, day, year, speed, bpm, watts, time);

                  sprintf(name, "%s ", first);
                  strcat(name, last);

                  (pInfo->print_fp)("%s   %s  %s %s %s   %s   %s\n",
                                    place, name, month, day, year, watts, time);

                  fprintf(fp_out, "%s   %s   %s %s %s   %s   %s\n",
                          place, name, month, day, year, watts, time);
               }
            }
            else
            {
               if (partyMode == PARTY_FOLLOWING_2)
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
               }


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

               if (partyMode == PARTY_FOLLOWING_2)
               {
                  if (includeVAM == 1)
                  {
                     /* 1    Lance Anderson  Sep 8, 2020     21.2mi/h    170   431W    1,345.5     4:42 */
                     sscanf(tmp, "%s %s %s %s %s %s %s %s %s %s %s",
                            place, first, last, month, day, year, speed, bpm, watts, vid, time);


                     sprintf(name, "%s ", first);
                     strcat(name, last);

                     NameInsert(timeLineInfo, name);

                     if (RacedToday(name, teamName))
                     {
                        sprintf(timeLineInfo->team, "%s", teamName);
                        sprintf(place, "%d", count);
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

                     NameInsert(timeLineInfo, name);

                     if (RacedToday(name, teamName))
                     {
                        sprintf(timeLineInfo->team, "%s", teamName);
                        sprintf(place, "%d", count);
                        count++;

                        (pInfo->print_fp)("%s   %s  %s  %s %s %s   %s   %s\n",
                                          place, name, teamName, month, day, year, watts, time);

                        fprintf(fp_out, "%s   %s  %s  %s %s %s   %s   %s\n",
                                place, name, teamName, month, day, year, watts, time);
                     }
                  }

                  if (RacedToday(name, teamName))
                  {
                     TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);
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

                     TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);
                  }

                  /* TEAM can be found in the name (Zwift does that) or on next line */
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("%3d  %s  ", count, timeLineInfo->name);
#else

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
#endif                  
                  fprintf(fp_out, "%s", tmp2);

                  if ((partyMode == PARTY_RESULTS) || (partyMode == PARTY_RESULTS_2))
                  {
                     /* If first athlete we DO NOT have a '+' difference */
                     if (firstAthlete != 1)
                     {
                        /* + */
                        if (!StringGet(tmp, fp_in, NL_KEEP))
                        {
                           /* Done reading file */
                           continueInLoop = 0;
                           break;
                        }

                     }
                     firstAthlete = 0;
                  }

                  /* watts */
                  if (!StringGet(tmp, fp_in, NL_KEEP))
                  {
                     /* Done reading file */
                     continueInLoop = 0;
                     break;
                  }

                  AddSpace(tmp2, tmp);
#ifdef CONSOLE_OUTPUT
                  (pInfo->print_fp)("%s", tmp2);
#else
#endif                  
                  fprintf(fp_out, "%s", tmp2);

                  if (partyMode != PARTY_RESULTS_2)
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
                  if ((tmp[0] != 'B') && (tmp[0] != 'A'))
                  {
                     continueInLoop = 0;
                     break;
                  }

                  if ((tmp[0] == 'B') && 
                      (partyControl.group == GROUP_A))
                  {
#if 0
                     /* Put count back */
                     count -= 1;

                     /* Free up entry */
                     TimeLineInfoFree(pInfo, listTimeLine, timeLineInfo);
#endif
                     continueInLoop = 0;
                     break;
                  }
                  else if ((tmp[0] == 'A') && 
                           (partyControl.group == GROUP_B))
                  {
#if 0
                     /* Put count back */
                     count -= 1;

                     /* Free up entry */
                     TimeLineInfoFree(pInfo, listTimeLine, timeLineInfo);
#endif
                     continueInLoop = 0;
                     break;
                  }
               }
            }
         }
      }
   }

   fclose(fp_in);

   AddUpPoints(pInfo, count, raceId);

   if (partyMode == PARTY_FOLLOWING_2)
   {
      /* Did we find everyone? */
      RidersMissed(pInfo);
   }

#ifdef CONSOLE_OUTPUT
   (pInfo->print_fp)("\n\n");
#else
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
    int count = 1;

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

   ptr = (Link_t *)listTimeOrder->head;
   while (ptr->next != NULL)
   {
      currInfo = (TimeLineInfo_t *)ptr->currentObject;

      timeLineInfo = (TimeLineInfo_t *)currInfo->me;
      
      if (timeLineInfo->team[0] != '\0')
      {
         (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, timeLineInfo->team);
      }
      else
      {
         (pInfo->print_fp)("  %-3d %-32s %-30s ", count, timeLineInfo->name, " ");
      }
      count++;

      for (i = 0; i < totalRacesIncluded; i++)
      {
         if (timeLineInfo->race[i].place != -1)
         {
            sprintf(placePoints, "%2d/%-2d", timeLineInfo->race[i].place, timeLineInfo->race[i].points);
            // (pInfo->print_fp)(" %-10d/%d ", timeLineInfo->race[i].place, timeLineInfo->race[i].points);
            (pInfo->print_fp)(" %-10s ", placePoints);
         }
         else
         {
            sprintf(placePoints, "%2d/%-2d", 0, 0);
            // (pInfo->print_fp)(" %-10d/0 ", 0);
            (pInfo->print_fp)(" %-10s ", placePoints);
         }
      }

      // (pInfo->print_fp)(" %-10d ", timeLineInfo->points);
      (pInfo->print_fp)(" %-10d ", currInfo->points);

      (pInfo->print_fp)("\n");

      ptr = ptr->next;
   }

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
      (pInfo->print_fp)("USAGE: %s {A|B|ALL}}\n", pInfo->argv[0]);
      return;
   }

    if (strcmp(pInfo->argv[1], "A") == 0)
    {
       partyControl.group = GROUP_A;
    }
    else if (strcmp(pInfo->argv[1], "B") == 0)
    {
       partyControl.group = GROUP_B;
    }
    else if (strcmp(pInfo->argv[1], "ALL") == 0)
    {
       partyControl.group = GROUP_ALL;
    }
    else
    {
        (pInfo->print_fp)("USAGE: %s {A|B|ALL}\n", pInfo->argv[0]);
        return;
    }
}

void PartyInit(CLI_PARSE_INFO *pInfo)
{
   int i;
   
   partyControl.max = -1;
   partyControl.bestOf = BEST_OF_TWO;
   partyControl.bonus = 0;
   partyControl.doubleUp = 0;
   partyControl.ageBonus = 0;
   partyControl.debug = 0;
   partyControl.group = GROUP_ALL;

    for (i = 0; i < MAX_ATHLETES; i++)
    {
       athleteA[i].enabled = 0;
       athleteB[i].enabled = 0;
    }

    strcpy(athleteA[i-1].name, "ATHLETE_END");
    strcpy(athleteB[i-1].name, "ATHLETE_END");
}

static const CLI_PARSE_CMD party_set_cmd[] =
{
    { "max",       PartySetMax,       "max number of race entries"},
    { "bestof",    PartySetBestOf,    "N in best-of-N races"},
    { "bonus",     PartySetBonus,     "on|off - 5 pts for 3 races, 10 pts for 4 races"},
    { "double",    PartySetDouble,    "on|off - DOUBLE points for Alpe race (warranted)"},
    { "ageBonus",  PartySetAgeBonus,  "on|off - extra 5 points for being 55+"},
    { "debug",     PartySetDebug,     "on|off"},
    { "group",     PartySetGroup,     "A|B|ALL (ALL is default)"},
    { NULL, NULL, NULL }
};

static void cmd_party_set(CLI_PARSE_INFO *info)
{
    cliDefaultHandler( info, party_set_cmd );
}

static void cmd_party_reset(CLI_PARSE_INFO *info)
{
   /* Reset things each time through */
   PartyReset(info, listTimeLine);
   PartyReset(info, listTimeOrder);
   PartyInit(info);
}

int PartyForceAthleteA(CLI_PARSE_INFO *info, char *name)
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

       if (strcmp(athleteA[i].name, name) != 0)
       {
          return 1;
       }
    }

    return 0;
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

static const CLI_PARSE_CMD cmd_party_commands[] =
{
   { "start",           cmd_party_start,                "start party"},
   { "stop",            cmd_party_stop,                 "stop party"},
   { "reset",           cmd_party_reset,                "reset party"},
   { "show",            cmd_party_show,                 "show party status"},
   { "set",             cmd_party_set,                  "party set commands"},
   { "force",           cmd_party_force,                "force {athlete} {A|B} force an athlete to a category"},
   { "following",       cmd_party_following,            "following [infile] [outfile]"},
   { "following2",      cmd_party_following2,           "following2 [infile] [outfile]"},
   { "results",         cmd_party_results,              "results [infile] [outfile]"},
   { "results2",        cmd_party_results2,             "results2 [infile] [outfile]"},
   { "kom",             cmd_party_kom_and_sprints,      "kom [infile] [outfile]"},
   { "sprints",         cmd_party_kom_and_sprints,      "sprints [infile] [outfile]"},
   { "filter",          PartyTimeFilterCmd,             "filter commands"},
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
