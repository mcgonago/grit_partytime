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

LinkList_t *listTimeLine = NULL;

TimeLinePool_t *timeLinePool;

//#define DEBUG_LEVEL_1 (1)

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

#define MAX_FRIENDS (100)

typedef struct Friend_t
{
   char name[MAX_STRING_SIZE];
} Friend_s;

Friend_s friend[MAX_FRIENDS] =
{
   "Lance Anderson",
   "Alan Brannan",
   "Shaun Corbin",
   "Luke Elton",
   "Rob Fullerton",
   "Seth G",
   "tak ina",
   "John Jeffries",
   "Gabriel Mathisen",
   "Owen McGonagle",
   "Steve Peplinski",
   "Carson Shedd",
   "Steve Tappan",
   "Maximilian Weniger",
   "Derek Sawyer",
};

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
TimeLineRemove(TimeLineInfo_t *timeLineInfo)
{
   /* write pattern to catch reuse problems */
   memset(timeLineInfo, 0xCC, sizeof(TimeLineInfo_t));

   timeLineEnv.numberOfTimeLineInfoBlocks -= 1;

   timeLineInfo->taken = 0;
   timeLineInfo->timeLine[0] = '\0';
   timeLineInfo->timeStamp = 0;
   timeLineInfo->fraction = 0;
   timeLineInfo->msecs = 0;
   timeLineInfo->type = TYPE_UNKNOWN;
   timeLineInfo->processed = 0;
   timeLineInfo->pair = NULL;
   timeLineInfo->paired = 0;
}

void PartyReset(CLI_PARSE_INFO *pInfo)
{
   Link_t *ptr;
   Link_t *tmp;
    
   /* See if there is a match */
   ptr = (Link_t *)listTimeLine->head ;

   if (ptr->next != NULL)
   {
      while(ptr->next != NULL)
      {
         /* Free object */
         TimeLineRemove((TimeLineInfo_t *)ptr->currentObject);

         tmp = ptr->next;
            
         if (LinkRemove(listTimeLine, ptr) != TRUE)
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

   for (i = 0; i < MAX_RACES; i++)
   {
      timeLineInfo->race[i].place = -1;
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

static int owen_iter = 1;
static volatile int debug_catch = 0;


TimeLineInfo_t *
TimeLineInfoInsert(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, int race)
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

#if 0
   LinkTailAdd(listTimeLine, link);
#else
   /* See if name already exists */
   
    ptr = (Link_t *)listTimeLine->head;
    currInfo = (TimeLineInfo_t *)ptr->currentObject;
    if (currInfo == NULL)
    {
        /* First one */
        LinkHeadAdd(listTimeLine, link);
    }
    else
    {
#if 0
        LinkTailAdd(listTimeLine, link);
#endif
        ptr = (Link_t *)listTimeLine->head;
        while (ptr->next != NULL)
        {
            currInfo = (TimeLineInfo_t *)ptr->currentObject;

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
           LinkTailAdd(listTimeLine, link);
        }

#if 0
        ptr = (Link_t *)listTimeLine->head;
        while (ptr->next != NULL)
        {
            currInfo = (TimeLineInfo_t *)ptr->currentObject;

            if (strcmp(timeLineInfo->name, currInfo->name) == 0)
            {
               if (race == RACE_YES)
               {
                  /* If we are adding a race result, do so now */
                  for (i = 0; i < MAX_RACES; i++)
                  {
                     if (timeLineInfo->race[i].place == -1)
                     {
                        timeLineInfo->race[i].place = atoi(timeLineInfo->place);
                        strcpy(timeLineInfo->race[i].raceName, raceName);
                     }
                  }
               }
            }
        }
#endif
    }
#endif

   return (timeLineInfo);
}

void PartyInit(CLI_PARSE_INFO *pInfo)
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

}

static void cmd_party_start(CLI_PARSE_INFO *info)
{
   timeTable.on = 1;
}

static void cmd_party_stop(CLI_PARSE_INFO *info)
{
   timeTable.on = 0;
}

static int ToleranceMet_orig(Link_t *p1, Link_t *p2)
{
   int diff;
    
   TimeLineInfo_t *t1 = (TimeLineInfo_t *)p1->currentObject;
   TimeLineInfo_t *t2 = (TimeLineInfo_t *)p2->currentObject;
    
   if (t1->timeStamp == t2->timeStamp)
   {
      if (t1->fraction > t2->fraction)
      {
         diff = t1->fraction - t2->fraction;
      }
      else
      {
         diff = t2->fraction - t1->fraction;
      }

      if (timeTable.toleranceFraction != -1)
      {
         if (diff <= timeTable.toleranceFraction)
         {
            return 1;
         }
      }

      return 0;
   }
   else if (t1->timeStamp > t2->timeStamp)
   {
      diff = t1->timeStamp - t2->timeStamp;
   }
   else
   {
      diff = t2->timeStamp - t1->timeStamp;
   }

   if (timeTable.toleranceSeconds != -1)
   {
      if (diff <= timeTable.toleranceSeconds)
      {
         return 1;
      }
   }
    
        
   return 0;
}
              
static int ToleranceMet(Link_t *p1, Link_t *p2)
{
   int diff;
    
   TimeLineInfo_t *t1 = (TimeLineInfo_t *)p1->currentObject;
   TimeLineInfo_t *t2 = (TimeLineInfo_t *)p2->currentObject;

   TimeSpec_t time1;
   TimeSpec_t time2;
   TimeSpec_t result;
   TimeSpec_t tolerance;

   time1.tv_sec = t1->timeStamp;
   time2.tv_sec = t2->timeStamp;


   /* convert to microseconds */
   time1.tv_usec = t1->fraction * 10000; 
   time2.tv_usec = t2->fraction * 10000; 

   if (timeTable.toleranceSeconds == -1)
   {
      tolerance.tv_sec = 0;
   }
   else
   {
      tolerance.tv_sec = timeTable.toleranceSeconds;
   }
    
   tolerance.tv_usec = timeTable.toleranceFraction * 10000;

   if (TraceBufferTimeGreaterEqual(&time1, &time2))
   {
      TraceBufferTimeSubtract(&time2, &time1, &result);
   }
   else
   {
      TraceBufferTimeSubtract(&time1, &time2, &result);
   }

   if (TraceBufferTimeGreaterEqual(&tolerance, &result))
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

static void cmd_party_tolerance_seconds(CLI_PARSE_INFO *pInfo)
{
   timeTable.toleranceSeconds = cliCharToLong(pInfo->argv[1]);;
}

static void cmd_party_tolerance_fraction(CLI_PARSE_INFO *pInfo)
{
   timeTable.toleranceFraction = cliCharToLong(pInfo->argv[1]);;
}

void TimeLineFill(CLI_PARSE_INFO *pInfo, TimeLineInfo_t *timeLineInfo, char *timeLine)
{
#if 0
   char *ptr;
   char *ptr2;
   char tmp[MAX_STRING_SIZE];

//
//      |  # | Name                               |    time | wpkg | watts |
//      |----|------------------------------------|---------|------|-------|
//      |  1 | Maximilian Kuen                    | 1:07:00 |  3.5 |   251 |
//
   strcpy(tmp, timeLine);

   /* Find placement - formality */
   ptr = tmp;
   if (*ptr != '|')
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not find starting | in: %s\n", timeLine);
      exit(-1);
   }

   ptr++;
   while (*ptr == ' ')
   {
      ptr++;
   }

   ptr2 = ptr;
   while (*ptr2 != ' ')
   {
      ptr2++;
   }

   *ptr2 = '\0';
   strcpy(timeLineInfo->place, ptr);

   ptr = ++ptr2;

   /* Find name */

   ptr++;
   while (*ptr != '|')
   {
      ptr++;
   }
   ptr++;

   while (*ptr == ' ')
   {
      ptr++;
   }

   /* Go to end, then back */
   ptr2 = ptr;
   while (*ptr2 != '|')
   {
      ptr2++;
   }

   //  |  1 | Maximilian Kuen                    | 1:07:00 |  3.5 |   251 |

   /* Now go back */
   ptr2--;
   while (*ptr2 == ' ')
   {
      ptr2--;
   }

   ptr2++;
   *ptr2 = '\0';
   strcpy(timeLineInfo->name, ptr);
   
   /* Find time */

   ptr = ++ptr2;
   while (*ptr != '|')
   {
      ptr++;
   }
   ptr++;
   while (*ptr == ' ')
   {
      ptr++;
   }

   ptr2 = ptr;
   while (*ptr2 != ' ')
   {
      ptr2++;
   }
   *ptr2 = '\0';

   strcpy(timeLineInfo->time, ptr2);

   /* Find wpkg */

   //  |  1 | Maximilian Kuen                    | 1:07:00 |  3.5 |   251 |

   ptr = ++ptr2;
   while (*ptr != '|')
   {
      ptr++;
   }
   ptr++;
   while (*ptr == ' ')
   {
      ptr++;
   }

   ptr2 = ptr;
   while (*ptr2 != ' ')
   {
      ptr2++;
   }
   *ptr2 = '\0';

   strcpy(timeLineInfo->wpkg, ptr2);


   /* Find watts */

   //  |  1 | Maximilian Kuen                    | 1:07:00 |  3.5 |   251 |

   ptr = ++ptr2;
   while (*ptr != '|')
   {
      ptr++;
   }
   ptr++;
   while (*ptr == ' ')
   {
      ptr++;
   }

   ptr2 = ptr;
   while (*ptr2 != ' ')
   {
      ptr2++;
   }
   *ptr2 = '\0';

   strcpy(timeLineInfo->watts, ptr2);
#endif
}

void StrStrip(char *out, char *in)
{
  char *ptrIn = in;
  char *ptrOut = out;

  while (*ptrIn != '\0')
  {
    if ((*ptrIn == '\t') || (*ptrIn == '\r') || (*ptrIn == '\n'))
     {
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

#if 0
  // Go to end
  while (*ptrUpdate != '\0')
  {
     ptrUpdate++;
  }

  // Find first space going back
  while ((*ptrUpdate != ' ') && (*ptrUpdate != '\t'))
  {
     ptrUpdate--;
  }

  ptrUpdate++;
  left = *ptrUpdate;
  *ptrUpdate = ' ';
  while (*ptrUpdate != '\0')
  {
     right = *(ptrUpdate + 1);
     *ptrUpdate = left;
     ptrUpdate++;
     left = right;
  }
#endif
#if 1
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
#endif

#if 0
  while (*ptrIn != '\0')
  {
     if ((spaceFound == 0) && ((*ptrIn == ' ') || (*ptrIn == '\t')))
     {
        spaceFound = 1;
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
#endif

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
      out[len-1] = ' ';
      out[len] = ' ';
      out[len+1] = '\0';
   }

   return(ret);
}

void NameInsert(TimeLineInfo_t *timeLineInfo, char *name)
{
   char *ptr;

   if ((ptr = strstr(name, "LEADER:")) != NULL)
   {
      strcpy(timeLineInfo->name, &name[8]);
   }
   else if ((ptr = strstr(name, "SWEEPER:")) != NULL)
   {
      strcpy(timeLineInfo->name, &name[9]);
   }
   else
   {
      strcpy(timeLineInfo->name, name);
   }
}

void cmd_party_kom_and_sprints(CLI_PARSE_INFO *pInfo)
{
   int ret;
   char buff[180];
   char time_details1[80];
   int idx;
   int si;
   char *c;
   int len;
   int count = 1;

   FILE *fp_in;
   FILE *fp_out;
   char infile[MAX_STRING_SIZE];
   char outfile[MAX_STRING_SIZE];

   char tmp[MAX_STRING_SIZE];
   char tmp2[MAX_STRING_SIZE];
   char tmp3[MAX_STRING_SIZE];
   char raceName[MAX_STRING_SIZE];

   char timeLine[MAX_STRING_SIZE];

   Link_t *ptr, *ptr2;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *timeLineInfo2;

   Link_t *ptrI;
   Link_t *ptrJ;

   TimeLineInfo_t *timeLineInfoI;
   TimeLineInfo_t *timeLineInfoJ;

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
   firstTime = 1;

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

      if ((timeLine[0] == 'B') && 
          ((timeLine[1] == ' ') || (timeLine[1] == '\n')) &&
          (strstr(timeLine, "Distance") == NULL))
      {
         while (1)
         {
            // Athletes name
            if (!StringGet(tmp, fp_in, NL_REMOVE))
            {
               /* Done reading file */
               break;
            }

            if (strstr(tmp, "Description") != NULL)
            {
               /* Ooops... Found the first B or A before athletes */
               break;
            }

            timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);
            strcpy(timeLineInfo->timeLine, tmp);

            // strcpy(timeLineInfo->name, tmp);
            NameInsert(timeLineInfo, tmp);

            if (strcmp(raceName, "race1") == 0)
            {
               timeLineInfo->race[0].place = count;
               sprintf(timeLineInfo->race[0].raceName, "%s", raceName);
            }
            else if (strcmp(raceName, "race2") == 0)
            {
               timeLineInfo->race[1].place = count;
               sprintf(timeLineInfo->race[1].raceName, "%s", raceName);
            }
            else if (strcmp(raceName, "race3") == 0)
            {
               timeLineInfo->race[2].place = count;
               sprintf(timeLineInfo->race[2].raceName, "%s", raceName);
            }

            TimeLineInfoInsert(pInfo, timeLineInfo, RACE_NO);

            (pInfo->print_fp)("%3d  %s", count, tmp);
            fprintf(fp_out, "%3d  %s", count, tmp);
            count++;

            // Team or empty
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            // If team, skip
            if ((tmp[0] != '\n') && (tmp[0] != ' ') && (tmp[0] != '\r') && (tmp[0] != '\t'))
            {
               /* We have team name, thus skip next <empty> line */
               if (!StringGet(tmp, fp_in, NL_KEEP))
               {
                  /* Done reading file */
                  break;
               }
            }

            /* time */
            if (!StringGet(tmp, fp_in, NL_REMOVE))
            {
               /* Done reading file */
               break;
            }

            AddSpace(tmp2, tmp);

            (pInfo->print_fp)("%s", tmp2);
            fprintf(fp_out, "%s", tmp2);

            /* watts */
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            AddSpace(tmp2, tmp);
            (pInfo->print_fp)("%s", tmp2);
            fprintf(fp_out, "%s", tmp2);

            /* power */
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            /* B <original place> */
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            // Break if we see garbage
            if (tmp[0] != 'B')
            {
               break;
            }
         }
      }

#if 0
      timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);
      strcpy(timeLineInfo->timeLine, tmp);
      TimeLineFill(pInfo, timeLineInfo, tmp);
      TimeLineInfoInsert(pInfo, timeLineInfo);
#endif
   }

   fclose(fp_in);

#if 0
   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;
        
      (pInfo->print_fp)("%s", timeLineInfo->timeLine);
      ptr = ptr->next;
   }

   (pInfo->print_fp)("\n\n");
#endif
}


void cmd_party_results(CLI_PARSE_INFO *pInfo)
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
   
   FILE *fp_in;
   FILE *fp_out;
   char infile[MAX_STRING_SIZE];
   char outfile[MAX_STRING_SIZE];

   char tmp[MAX_STRING_SIZE];
   char tmp2[MAX_STRING_SIZE];
   char tmp3[MAX_STRING_SIZE];
   char raceName[MAX_STRING_SIZE];

   char timeLine[MAX_STRING_SIZE];

   Link_t *ptr, *ptr2;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *timeLineInfo2;

   Link_t *ptrI;
   Link_t *ptrJ;

   TimeLineInfo_t *timeLineInfoI;
   TimeLineInfo_t *timeLineInfoJ;

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
   firstTime = 1;

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

      if (((timeLine[0] == 'B') || (timeLine[0] == 'A')) &&
          ((timeLine[1] == ' ') || (timeLine[1] == '\n')) &&
          (strstr(timeLine, "Distance") == NULL))
      {
         while (1)
         {
            // Athletes name
            if (!StringGet(tmp, fp_in, NL_REMOVE))
            {
               /* Done reading file */
               break;
            }

            if (strstr(tmp, "Description") != NULL)
            {
               /* Ooops... Found the first B or A before athletes */
               break;
            }

            timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);
            strcpy(timeLineInfo->timeLine, tmp);
            
            // strcpy(timeLineInfo->name, tmp);
            NameInsert(timeLineInfo, tmp);

            if (strcmp(raceName, "race1") == 0)
            {
               timeLineInfo->race[0].place = count;
               sprintf(timeLineInfo->race[0].raceName, "%s", raceName);
            }
            else if (strcmp(raceName, "race2") == 0)
            {
               timeLineInfo->race[1].place = count;
               sprintf(timeLineInfo->race[1].raceName, "%s", raceName);
            }
            else if (strcmp(raceName, "race3") == 0)
            {
               timeLineInfo->race[2].place = count;
               sprintf(timeLineInfo->race[2].raceName, "%s", raceName);
            }

            TimeLineInfoInsert(pInfo, timeLineInfo, RACE_NO);

            (pInfo->print_fp)("%3d  %s", count, tmp);
            fprintf(fp_out, "%3d  %s", count, tmp);
            count++;

            // Team or empty
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            // If team, skip
            if ((tmp[0] != '\n') && (tmp[0] != ' ') && (tmp[0] != '\r') && (tmp[0] != '\t'))
            {
               /* We have team name, thus skip next <empty> line */
               if (!StringGet(tmp, fp_in, NL_KEEP))
               {
                  /* Done reading file */
                  break;
               }
            }

            /* time */
            if (!StringGet(tmp, fp_in, NL_REMOVE))
            {
               /* Done reading file */
               break;
            }

            AddSpace(tmp2, tmp);

            (pInfo->print_fp)("%s", tmp2);
            fprintf(fp_out, "%s", tmp2);

            /* If first athlete we DO NOT have a '+' difference */
            if (firstAthlete != 1)
            {
               /* + */
               if (!StringGet(tmp, fp_in, NL_KEEP))
               {
                  /* Done reading file */
                  break;
               }

            }
            firstAthlete = 0;

            /* watts */
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            AddSpace(tmp2, tmp);
            (pInfo->print_fp)("%s", tmp2);
            fprintf(fp_out, "%s", tmp2);

            /* TOTAL watts? */
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            /* extended stats */
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            // Break if we see garbage
            if (tmp[0] != 'B')
            {
               break;
            }

         }
      }
   }

   fclose(fp_in);

#if 0
   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;
        
      (pInfo->print_fp)("%s", timeLineInfo->timeLine);
      ptr = ptr->next;
   }

   (pInfo->print_fp)("\n\n");
#endif
    
}

void cmd_party_following(CLI_PARSE_INFO *pInfo)
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
   
   FILE *fp_in;
   FILE *fp_out;
   char infile[MAX_STRING_SIZE];
   char outfile[MAX_STRING_SIZE];

   char tmp[MAX_STRING_SIZE];
   char tmp2[MAX_STRING_SIZE];
   char tmp3[MAX_STRING_SIZE];
   char updatedLine[MAX_STRING_SIZE];

   char timeLine[MAX_STRING_SIZE];

   Link_t *ptr, *ptr2;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *timeLineInfo2;

   Link_t *ptrI;
   Link_t *ptrJ;

   TimeLineInfo_t *timeLineInfoI;
   TimeLineInfo_t *timeLineInfoJ;

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

   sprintf(test, "%s", "1 owen mcgonagle");

   printf("TEST: IN: %s\n", test);

   sscanf(test, "%s %s %s",
          &place[0], &first[0], &last[0]);

   (pInfo->print_fp)("TEST: OUT: %s %s %s\n", place, first, last);

   // fp_in = fopen("zwiftpower.txt", "r");
   sprintf(infile,  "%s", "database/_posts/2020/12/01/A-Smack-down-on-Petit-KOM_following.txt");
   sprintf(outfile, "%s", "database/_posts/2020/12/01/A-Smack-down-on-Petit-KOM_following.md");

   if ( pInfo->argc > 1)
   {
      sprintf(infile, "%s", pInfo->argv[1]);
   }

   if ( pInfo->argc > 2)
   {
      sprintf(outfile, "%s", pInfo->argv[2]);
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
   firstTime = 1;

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

      if (strstr(timeLine, "Rank ") != NULL)
      {
         if (strstr(timeLine, "VAM ") != NULL)
         {
            includeVAM = 1;
         }

         while (1)
         {
            // Athletes name
            if (!StringGet(tmp, fp_in, NL_KEEP))
            {
               /* Done reading file */
               break;
            }

            if ((tmp[0] == '\n') || (tmp[0] == ' ') || (tmp[0] == '\r') || (tmp[0] == '\t'))
            {
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

#if 0
            timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);

            strcpy(timeLineInfo->timeLine, tmp);

            timeLineInfo->race[0].place = atoi(place);
            sprintf(timeLineInfo->race[0].raceName, "%s", "foobar");

            strcpy(timeLineInfo->place, place);
            strcpy(timeLineInfo->name, name);
            strcpy(timeLineInfo->month, month);
            strcpy(timeLineInfo->day, day);
            strcpy(timeLineInfo->year, year);
//          strcpy(timeLineInfo->speed, speed);
            strcpy(timeLineInfo->watts, watts);
//          strcpy(timeLineInfo->wpkg, wpkg);
            strcpy(timeLineInfo->bpm, bpm);
            strcpy(timeLineInfo->vid, vid);
            strcpy(timeLineInfo->time, time);
//          strcpy(timeLineInfo->test, test);

            TimeLineFill(pInfo, timeLineInfo, tmp);
        
            TimeLineInfoInsert(pInfo, timeLineInfo, RACE_NO);
#endif
         }
      }

   }

   fclose(fp_in);

#if 0
   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;
        
      (pInfo->print_fp)("%s", timeLineInfo->timeLine);
      ptr = ptr->next;
   }

   (pInfo->print_fp)("\n\n");
#endif
   
}



void cmd_party_run(CLI_PARSE_INFO *pInfo)
{
   int ret;
    
#ifdef REVISIT_TIME_ADJUSTMENT
   TimeSpec_t baseWall;
   TimeSpec_t baseDiff;

   TimeSpec_t currWall;
   TimeSpec_t currDiff;

   TimeSpec_t resultWall;
   TimeSpec_t resultDiff;
   TimeSpec_t zeroTime;
#endif

   char buff[180];

   char time_details1[80];

#ifdef REVISIT_MKTIME_CONVERSION
   char time_details2[80];

   struct tm tm1;
   struct tm tm2;
   time_t t1, t2;

   time_t converted1;
   time_t converted2;
#endif
    
   int hh, mm, ss;
   struct tm when = {0};
   int doubledUp = 0;
    
#ifdef UNIT_TEST_SELF_CREATE
   int fd_usbc;
   int fd_usdp;
#endif

   FILE *fp_in;
   FILE *fp_usdp;

   char tmp[MAX_STRING_SIZE];
   char timeLine[MAX_STRING_SIZE];
   char updatedLine[MAX_STRING_SIZE];

   Link_t *ptr, *ptr2;
   TimeLineInfo_t *timeLineInfo;
   TimeLineInfo_t *timeLineInfo2;

   Link_t *ptrI;
   Link_t *ptrJ;

   TimeLineInfo_t *timeLineInfoI;
   TimeLineInfo_t *timeLineInfoJ;

#if 0
   TimeLineInfo_t *timeLineInfoPrev;
#endif

#if 0
   Jan 11 15:56:00.56 [000.000024] [000.000024]: [linux/private/usdp/usdp.c:91]               [main]                 [tLinux]             HELLO !!!!!!!!!!! 
      Jan 11 15:56:00.57 [000.001112] [000.001088]: [linux/private/usdp/usdp.c:141]              [main]                 [tLinux]             Calling initUsdp ... 
#endif

      sprintf(buff, "%s", "Jan 11 15:56:00.56 [000.000024] [000.000024]: [linux/private/usdp/usdp.c:91]               [main]                 [tLinux]             HELLO !!!!!!!!!!! ");

#ifdef REVISIT_MKTIME_CONVERSION
    
   // Convert 15:56:00.56 to a NUMBER that can be compared against?

   // t1 is now your desired time_t

   sprintf(time_details2, "%s", "16:35:13");
   strptime(time_details2, "%H:%M:%S", &tm2);

   t2 = mktime(&tm2);

   sprintf(time_details1, "%s", "16:35:12");
   strptime(time_details1, "%H:%M:%S", &tm1);

   t1 = mktime(&tm1);


   if (t1 > t2)
   {
      (pInfo->print_fp)("%s (%d) is greater than %s (%d)\n", time_details1, tm1, time_details2, tm2);
   }
   else
   {
      (pInfo->print_fp)("%s (%d) is greater than %s (%d)\n", time_details2, tm2, time_details1, tm1);
   }

   sprintf(time_details1, "%s", "16:35:12");
   sscanf(time_details1, "%d:%d:%d", &hh, &mm, &ss);

   when.tm_hour = hh;
   when.tm_min = mm;
   when.tm_sec = ss;

   converted1 = mktime(&when);
    
   sprintf(time_details2, "%s", "16:35:13");
   sscanf(time_details2, "%d:%d:%d", &hh, &mm, &ss);

   when.tm_hour = hh;
   when.tm_min = mm;
   when.tm_sec = ss;

   converted2 = mktime(&when);

   if (converted1 > converted2)
   {
      (pInfo->print_fp)("%s (%d) is greater than %s (%d)\n", time_details1, converted1, time_details2, converted2);
   }
   else
   {
      (pInfo->print_fp)("%s (%d) is greater than %s (%d)\n", time_details2, converted2, time_details1, converted1);
   }
    

   sprintf(time_details1, "%s", "16:34:12");
   sscanf(time_details1, "%d:%d:%d", &hh, &mm, &ss);

   when.tm_hour = hh;
   when.tm_min = mm;
   when.tm_sec = ss;

   converted1 = mktime(&when);
    
   sprintf(time_details2, "%s", "16:38:13");
   sscanf(time_details2, "%d:%d:%d", &hh, &mm, &ss);

   when.tm_hour = hh;
   when.tm_min = mm;
   when.tm_sec = ss;

   converted2 = mktime(&when);

   if (converted1 > converted2)
   {
      (pInfo->print_fp)("%s (%d) is greater than %s (%d)\n\n", time_details1, converted1, time_details2, converted2);
   }
   else
   {
      (pInfo->print_fp)("%s (%d) is greater than %s (%d)\n\n", time_details2, converted2, time_details1, converted1);
   }
#endif
    

// Unhook to create specific usbc_1.txt and usdp_1.txt 
#ifdef UNIT_TEST_SELF_CREATE
   fd_usbc = open("usbc_1.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
   if (fd_usbc < 0)
   {
      (pInfo->print_fp)("ERROR: Could not open usbc_1.txt\n");
      exit(-1);
   }

   fd_usdp = open("usdp_1.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
   if (fd_usdp < 0)
   {
      (pInfo->print_fp)("ERROR: Could not open usdp_1.txt\n");
      exit(-1);
   }

   /* 0-158 - including \n and \0 gives us 159 + 2 = 161 */
   /* 0-158 - including \n        gives us 159 + 1 = 160 */

#if 1
   write(fd_usbc, "Jan 14 02:50:39.17 [000.000014] [000.000014]: [linux/private/common/src/usrAppInit.c:872]            [main]                 [tLinux]             IN.......   f1\n", 160);
   write(fd_usbc, "Jan 14 02:50:39.20 [000.036851] [000.036837]: [npsoft/apps/dpdk/acmeUsdpClient.cpp:1726]             [usdpClientIsDPDKC]    [tLinux]             IN...ONLY ONE \n", 160);
   write(fd_usbc, "Jan 14 02:50:39.20 [000.036962] [000.000111]: [linux/private/common/src/usrAppInit.c:633]            [user_sysinit]         [tLinux]             IN.......   f1\n", 160);
   write(fd_usbc, "Jan 14 02:50:39.40 [000.037109] [000.000147]: [npsoft/apps/dpdk/acmeUsdpClient.cpp:119]              [initUsdpClient]       [tLinux]             IN.......   f1\n", 160);
   write(fd_usbc, "Jan 14 02:50:39.41 [000.038720] [000.001601]: [npsoft/apps/dpdk/acmeUsdpClient.cpp:186]              [UsdpClientObject]     [tLinux]             IN.......   f1\n", 160);
#endif

#if 0
   char jan[] = "Jan\n";
   write(fd_usbc, jan, 5);
#endif

#if 0
   write(fd_usbc, "Jan\n", 4);
   write(fd_usbc, "Jan\n", 4);
   write(fd_usbc, "Jan\n", 4);
   write(fd_usbc, "Jan\n", 4);
#endif
          
   /* 0 - 154 - including \n and \0 gives us 155 + 2 = 157 */
   /* 0 - 154 - including \n        gives us 155 + 1 = 156 */
   write(fd_usdp, "Jan 14 02:50:38.04 [000.000030] [000.000030]: [linux/private/usdp/usdp.c:109]                        [main]                 [tLinux]             IN.... f1 \n", 156);
   write(fd_usdp, "Jan 14 02:50:39.30 [000.001151] [000.001121]: [linux/private/usdp/usdp.c:159]                        [main]                 [tLinux]             Calling in\n", 156);
   write(fd_usdp, "Jan 14 02:50:39.30 [000.001167] [000.000016]: [npsoft/target/dpdk/acmeUsdp.cpp:399]                  [initUsdp]             [tLinux]             IN..... f1\n", 156);
   write(fd_usdp, "Jan 14 02:50:39.51 [000.007791] [000.006624]: [npsoft/lib/dpdk/common/acmeDpdk.cpp:973]              [createBwListString]   [tLinux]             IN..... f1\n", 156);

   close(fd_usbc);
   close(fd_usdp);
#endif

   fp_in = fopen("partytime.txt", "r");
   if (!fp_in)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not open partytime.txt\n");
      exit(-1);
   }
        
   (pInfo->print_fp)("\n-------- USBC ORIG\n");

   while (1)
   {
      if (!fgets(tmp, MAX_STRING_SIZE, fp_in))
      {
         /* Done reading file */
         break;
      }

      StrStrip(timeLine, tmp);
      
      (pInfo->print_fp)("%s", timeLine);

      if (!FormattedProper(timeLine))
      {
         /* prperly formatted line not found */
         continue;
      }

      timeLineInfo = TimeLineInfoNew(pInfo, TYPE_USBC);

      sprintf(updatedLine, "%s", timeLine);
      strcpy(timeLineInfo->timeLine, updatedLine);

      TimeLineFill(pInfo, timeLineInfo, timeLine);

#if 0
      // sprintf(time_details1, "%s", "16:34:12");
      strncpy(time_details1, &updatedLine[13], 9);
      time_details1[8] = '\0';

      sscanf(time_details1, "%d:%d:%d", &hh, &mm, &ss);

      when.tm_hour = hh;
      when.tm_min = mm;
      when.tm_sec = ss;

      // converted1 = mktime(&when);
      timeLineInfo->timeStamp = mktime(&when);

      strncpy(time_details1, &updatedLine[22], 2);
      time_details1[2] = '\0';

      sscanf(time_details1, "%02d", &timeLineInfo->fraction);

#ifdef DEBUG_LEVEL_1
      (pInfo->print_fp)("     %s", updatedLine);
#else
      (pInfo->print_fp)("%s", updatedLine);
#endif        
#endif
        
      TimeLineInfoInsert(pInfo, timeLineInfo, RACE_NO);
   }

   fclose(fp_in);
        

   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;
        
      (pInfo->print_fp)("%s", timeLineInfo->timeLine);
      ptr = ptr->next;
   }

   (pInfo->print_fp)("\n\n");
    
}

void cmd_party_show(CLI_PARSE_INFO *pInfo)
{
    TimeLineInfo_t *timeLineInfo;
    Link_t *link;      
    Link_t *ptr;
    int i;

   ptr = (Link_t *)listTimeLine->head;
   while (ptr->next != NULL)
   {
      timeLineInfo = (TimeLineInfo_t *)ptr->currentObject;
      
      (pInfo->print_fp)("%32s ", timeLineInfo->name);

      for (i = 0; i < MAX_RACES; i++)
      {
         if (timeLineInfo->race[i].place != -1)
         {
            (pInfo->print_fp)(" %2d ", timeLineInfo->race[i].place);
         }
      }

      (pInfo->print_fp)("\n");

#if 0
      // (pInfo->print_fp)("%s", timeLineInfo->timeLine);
      (pInfo->print_fp)("%s   %s  %s %s %s   %s   %s\n",
         timeLineInfo->place,
         timeLineInfo->name,
         timeLineInfo->month,
         timeLineInfo->day,
         timeLineInfo->year,
         timeLineInfo->watts,
         timeLineInfo->time);
#endif

      ptr = ptr->next;
   }
}

static const CLI_PARSE_CMD cmd_party_commands[] =
{
   { "start",           cmd_party_start,                "start party"},
   { "stop",            cmd_party_stop,                 "stop party"},
   { "show",            cmd_party_show,                 "show party status"},
   { "following",       cmd_party_following,            "following [infile] [outfile]"},
   { "results",         cmd_party_results,              "results [infile] [outfile]"},
   { "kom",             cmd_party_kom_and_sprints,      "kom [infile] [outfile]"},
   { "sprints",         cmd_party_kom_and_sprints,      "sprints [infile] [outfile]"},
   { NULL, NULL, NULL }
};

void cmd_party( CLI_PARSE_INFO *info)
{
   if (firstTime == 1)
   {
      PartyInit(info);
      firstTime = 0;
   }

   /* Reset things each time through */
   // PartyReset(info);

   cliDefaultHandler( info, cmd_party_commands );
}

