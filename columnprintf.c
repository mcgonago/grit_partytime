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
//#include "partycontrol.h"

#include "zcli.h"
#include "zcolor.h"
#include "tracebuffer.h"
#include "timeline.h"
#include "zcli_server.h"
#include "usrsprintf.h"
#include "partycontrol.h"
#include "columnprintf.h"

static volatile int gdbStop = 0;

#ifdef UNIT_TEST
static char newCommandLine[160];
Arguments_t arguments;

void cliPrintHelp( CLI_PARSE_INFO *info, const CLI_PARSE_CMD *cmds );
void cmdHelp( CLI_PARSE_INFO *info);
#endif

#define MAX_MY_COLUMNS (8192)

#define COLUMN_SPACING (2)
// #define COLUMN_SPACING (0)

#define COLUMNIZE_FOLLOWING (1)
#define COLUMNIZE_RESULTS   (2)
#define COLUMNIZE_UNKNOWN   (3)

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


   outIdx -= 1;
   outStr[outIdx++] = '|';
   outStr[outIdx++] = '\0';

   (pInfo->print_fp)("%s\n", outStr);

}

void PrintTable(CLI_PARSE_INFO *pInfo)
{
   int i;

   for (i = 0; i < MAX_ENTRIES; i++)
   {
      if (printTable[i].enabled == 0)
      {
         break;
      }

      ColumnPrintf(pInfo, printTable[i].entry);
   }
}

static int owen = 0;

void ColumnStore(CLI_PARSE_INFO *pInfo, char *str)
{
   int i, j;
   int idx = 0;
   int count = 0;
   int endCount = 0;
   char *ptr;

   RemoveTrailingAdd2Inline(str);

#if 0
   if (owen < 2)
   {
      owen++;
      (pInfo->print_fp)("INFO: %s\n", str);
   }
#endif
   
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

void RemoveTrailingAdd2Inline(char *in)
{
  char *ptrIn = in;
  char *ptr;

  if (*in == '\0')
  {
     return;
  }

  if (strstr(in, "foob") != 0)
  {
     gdbStop = 1;
  }

  /* Go to end and removed trailing spaces */
  ptr = in;
  while (*ptr != '\0')
  {
     ptr++;
  }

  ptr--;
  if (*ptr == ' ')
  {
     while (*ptr == ' ')
     {
        ptr--;
     }
     ptr++;
     *ptr = '\0';
  }
  else
  {
     ptr++;
  }

  *ptr++ = ' ';
  *ptr++ = ' ';
  *ptr = '\0';
}

void RemoveTrailingNew(char *out, char *in)
{
    RemoveTrailingAdd2Inline(in);
    strcpy(out, in);
//  RemoveTrailingAdd2Inline(in);
//  AddSpace(out, in);
}

void RemoveTrailing(char *out, char *in)
{
  RemoveTrailingAdd2Inline(in);
  AddSpace(out, in);
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
     else if ((*ptrIn == '\n') || (*ptrIn == '\r'))
     {
        /* Get rid of newlines and returns also */
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


void FixTeamNameNew(CLI_PARSE_INFO *pInfo, char *out, char *in)
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

#ifdef UNIT_TEST


char *StringGet2(char *out, FILE *fp)
{
   char *ret;
   int len;
   char tmp[MAX_STRING_SIZE];
   char tmp2[MAX_STRING_SIZE];

   ret = fgets(tmp, MAX_STRING_SIZE, fp);

   if (ret != NULL)
   {
      ReplaceTabs(tmp2, tmp);
   }

   RemoveTrailingNew(out, tmp2);

   return(ret);
}

void ColumnizeFollowing(CLI_PARSE_INFO *pInfo, char *fname)
{
   int i;
   int outIdx;

   FILE *fp_in;
   char tmp[MAX_STRING_SIZE];
   char teamName[MAX_STRING_SIZE];
   char teamName2[MAX_STRING_SIZE];
   char fixed[MAX_STRING_SIZE];
   char fixed2[MAX_STRING_SIZE];
   char header[MAX_STRING_SIZE];

   int trigger = 0;

   fp_in = fopen(fname, "r");

   if (!fp_in)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not open %s\n", fname);
      exit(-1);
   }

   while (1)
   {
      if (!StringGet2(tmp, fp_in))
      {
         /* Done reading file */
         break;
      }

//      (pInfo->print_fp)("INFO: %s\n", tmp);

      if ((strstr(tmp, "Rank ") != NULL) || (strstr(tmp, "VAM ") != NULL))
      {
         trigger = 1;

         if (strstr(tmp, "VAM ") != NULL)
         {
            /* Create headers */
            outIdx = 0;
            outIdx += sprintf(&header[outIdx], "%s", "#  name  date  speed  BPM  watts  VAM   time  team");
            ColumnStore(pInfo, header);

            sprintf(header, "%s", "--  ----   ----   ----   -----  ----  ----  ----  -----");
            ColumnStore(pInfo, header);
         }
         else
         {
            /* Create headers */
            outIdx = 0;
            outIdx += sprintf(&header[outIdx], "%s", "#  name  date  speed  BPM  watts  time  team");
            ColumnStore(pInfo, header);

            sprintf(header, "%s", "--  ----   ----   ----   -----  ----  ----  -----");
            ColumnStore(pInfo, header);
         }

         continue;
      }

      if (trigger == 1)
      {
         if ((tmp[0] == '\n') || (tmp[0] == ' ') || (tmp[0] == '\r') || (tmp[0] == '\t'))
         {
            continue;
         }

         if ((strstr(tmp, "mi/h") != NULL) && (strstr(tmp, "W ") != NULL) &&
             (strstr(tmp, ", 20") != NULL))
         {
            strcpy(fixed, tmp);

            teamName[0] = '\0';

            FixLine(pInfo, fixed2, fixed, teamName);
            FixTeamNameNew(pInfo, teamName2, teamName);

            if ((teamName[0] != '\0') && (strlen(teamName2) < 15))
            {
               strcat(fixed2, "  ");
               strcat(fixed2, teamName2);
               strcat(fixed2, "  ");
            }
            else if ((teamName[0] != '\0') && (strlen(teamName2) > 10))
            {
               strcat(fixed2, "  *  ");
            }
            else
            {
               strcat(fixed2, "  -  ");
            }

            ColumnStore(pInfo, fixed2);
         }
      }
   }

   PrintTable(pInfo);

   fclose(fp_in);
}

void NameInsert(char *out, char *name, char *teamOut)
{
   char *ptr;
   char tmp[MAX_STRING_SIZE];
   char team[MAX_STRING_SIZE];
   int teamFound = 0;
   int ret = 0;

   teamOut[0] = '\0';

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

   strcpy(out, tmp);

   ptr = out;
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

   /* Split team from name */
   ptr = tmp;

   if (strstr(tmp, "MHA_ KOH") != 0)
   {
      teamFound = 1;
      strcpy(out, "MHA KOH");
      strcpy(teamOut, "[JETT]");
   }
   else if (strstr(tmp, "Derek Sawyer") != 0)
   {
      teamFound = 1;
      strcpy(out, "Derek Sawyer");
      strcpy(teamOut, "[GRIT][Rippers]");
   }
   else if (strstr(tmp, "conrad moss") != 0)
   {
      teamFound = 1;
      strcpy(out, "Conrad Moss");
      strcpy(teamOut, "Velopower PC, P-TJ RT");
   }
   else if (strstr(tmp, "Oleg Semeko") != 0)
   {
      teamFound = 1;
      strcpy(out, "Oleg Semeko");
      strcpy(teamOut, "Veter.cc");
   }
   else if (strstr(tmp, "Gabriel Mathisen") != 0)
   {
      teamFound = 1;
      strcpy(out, "Gabriel Mathisen");
      strcpy(teamOut, "[VEGAN][GRIT]");
   }
   else if ((strstr(tmp, "Korea") != 0) && (strstr(tmp, "Marc") != 0))
   {
      teamFound = 1;
      strcpy(out, "Korea Marc");
      strcpy(teamOut, "-");
   }
   else if ((strstr(tmp, "Ren") != 0) && (strstr(tmp, "Jeppesen") != 0))
   {
      teamFound = 1;
      strcpy(out, "Rene Jeppesen");
      strcpy(teamOut, "-");
   }
   else if ((strstr(tmp, "tak ina") != 0) || (strstr(tmp, "Tak Ina") != 0))
   {
      teamFound = 1;
      strcpy(out, "Tak Ina");
      strcpy(teamOut, "[JETT][GRIT]");
   }
   else if (strstr(tmp, "VV Cucumber") != 0)
   {
      teamFound = 1;
      strcpy(out, "VV Cucumber");
      strcpy(teamOut, "[LOOKCYCLE CHN]");
   }
   else if (strstr(tmp, "Ricardo Abela") != 0)
   {
      teamFound = 1;
      strcpy(out, "Ricardo Abela");
      strcpy(teamOut, "TEAM BMTR");
   }
   else if (strstr(tmp, "Dan Nelson") != 0)
   {
      teamFound = 1;
      strcpy(out, "Dan Nelson");
      strcpy(teamOut, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(tmp, "Kelly Toth") != 0)
   {
      teamFound = 1;
      strcpy(out, "Kelly Toth");
      strcpy(teamOut, "CRCA/NYCC");
   }
   else if (strstr(tmp, "John Jeffries") != 0)
   {
      teamFound = 1;
      strcpy(out, "John Jeffries");
      strcpy(teamOut, "[AA Bikes][GRIT]");
   }
   else if (strstr(tmp, "Guido Kramer Leymen") != 0)
   {
      teamFound = 1;
      strcpy(out, "Guido Kramer Leymen");
      strcpy(teamOut, "-");
   }
   else if (strstr(tmp, "Paul Roy") != 0)
   {
      teamFound = 1;
      strcpy(out, "Paul Roy");
      strcpy(teamOut, "RETO!");
   }
   else if (strstr(tmp, "Guillaume Lafleur") != 0)
   {
      teamFound = 1;
      strcpy(out, "Guillaume Lafleur");
      strcpy(teamOut, "leMETIER");
   }
   else if (strstr(tmp, "Bruch Wu") != 0)
   {
      teamFound = 1;
      strcpy(out, "Bruch Wu");
      strcpy(teamOut, "-");
   }
   else if ((strstr(tmp, "Bergenzaun") != 0) && (strstr(tmp, "ns ") != 0))
   {
      teamFound = 1;
      strcpy(out, "Mans Bergenzaun");
      strcpy(teamOut, "TEAM BTC");
   }
   else if ((strstr(tmp, "Egon") != 0) && (strstr(tmp, "Ivan") != 0))
   {
      teamFound = 1;
      strcpy(out, "Egon Ivanjsek");
      strcpy(teamOut, "PPRteam");
   }
   else if ((strstr(tmp, "Thomas") != 0) && (strstr(tmp, "Schr") != 0))
   {
      teamFound = 1;
      strcpy(out, "Thomas Schrader");
      strcpy(teamOut, "ZRG");
   }
   else if (strstr(tmp, "C hris M") != 0)
   {
      teamFound = 1;
      strcpy(out, "Chris M");
      strcpy(teamOut, "B4H");
   }
   else if (strstr(tmp, "Seth G ") != 0)
   {
      teamFound = 1;
      strcpy(out, "Seth G");
      strcpy(teamOut, "[GRIT]");
   }
   else if (strstr(tmp, "Renfrew Bunch") != 0)
   {
      teamFound = 1;
      strcpy(out, "Renfrew Bunch");
      strcpy(teamOut, "[Scotland]");
   }
   else if (!isprint(tmp[0]) && !isprint(tmp[1]) && !isprint(tmp[2]))
   {
      teamFound = 1;
      strcpy(out, "__STAR__");
      strcpy(teamOut, "-");
   }
   else if ((strstr(tmp, "Bergenzaun") != 0) && (strstr(tmp, "M") != 0))
   {
      teamFound = 1;
      strcpy(out, "Mans Bergenzaun");
      strcpy(teamOut, "Team BTC");
   }
   else if (strstr(tmp, "Dax Nelson") != 0)
   {
      teamFound = 1;
      strcpy(out, "Dax Nelson");
      strcpy(teamOut, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(tmp, "Greg Langman") != 0)
   {
      teamFound = 1;
      strcpy(out, "Greg Langman");
      strcpy(teamOut, "DIRT");
   }
   else if ((strstr(tmp, "Hiroki") != 0) && (strstr(tmp, "Tanaka") != 0))
   {
      teamFound = 1;
      strcpy(out, "Hiroki Tanaka");
      strcpy(teamOut, "-");
   }
   else if ((strstr(tmp, "Hans") != 0) && (strstr(tmp, "rgen") != 0))
   {
      teamFound = 1;
      strcpy(out, "Hans-Jurgen G");
      strcpy(teamOut, "-");
   }
   else if ((strstr(tmp, "J B") != 0) && (strstr(tmp, "SWOZR") != 0))
   {
      teamFound = 1;
      strcpy(out, "J B #1");
      strcpy(teamOut, "SWOZR");
   }
   else if ((strstr(tmp, "J B") != 0) && (strstr(tmp, "Bees") != 0))
   {
      teamFound = 1;
      strcpy(out, "J B #2");
      strcpy(teamOut, "BCC - Bees");
   }
#if 0
   else if (strstr(tmp, "J B") != 0)
   {
      teamFound = 1;
      strcpy(out, "J B");
      strcpy(teamOut, "-");
   }
#endif
   else if (strstr(tmp, "Sebastien Loir") != 0)
   {
      teamFound = 1;
      strcpy(out, "Sebastien Loir");
      strcpy(teamOut, "BIKES-FR");
   }
   else if (strstr(tmp, "Greg Rashford") != 0)
   {
      teamFound = 1;
      strcpy(out, "Greg Rashford");
      strcpy(teamOut, "#NOMAD");
   }
   else if (strstr(tmp, "Dan Newton") != 0)
   {
      teamFound = 1;
      strcpy(out, "Dan Newton");
      strcpy(teamOut, "WGT - Bynea CC");
   }
   else if (strstr(tmp, "Dan Nelson") != 0)
   {
      teamFound = 1;
      strcpy(out, "Dan Nelson");
      strcpy(teamOut, "!DUX! TPA-FLA - HERD");
   }
   else if ((strstr(tmp, "Rafa") != 0) && (strstr(tmp, "Dzieko") != 0))
   {
      teamFound = 1;
      strcpy(out, "Rafa Dzieko");
      strcpy(teamOut, "ski ZTPL.CC");
   }
#if 0
   else if (strstr(tmp, "Denis Frolov") != 0)
   {
      teamFound = 1;
      strcpy(out, "Denis Frolov");
      strcpy(teamOut, "UTT");
   }
   else if (strstr(tmp->name, "Roy Woodland") != 0)
   {
      teamFound = 1;
      strcpy(out, "Roy Woodland");
      strcpy(teamOut, "PACK");
   }
#endif
   else if (strstr(tmp, "Steve Tappan") != 0)
   {
      teamFound = 1;
      strcpy(out, "Steve Tappan");
      strcpy(teamOut, "[GRIT]");
   }
   else
   {
      if ((teamOut[0] == '\0') || (teamOut[0] == '-'))
      {
         while (*ptr != '\0')
         {
            if (isdigit(*ptr))
            {
               /* If we hit a number at any point before a true team name - assume it is ZwiftPower nametag */
               strcpy(teamOut, "-");
               teamFound = 1;

               ptr--;
               while (*ptr == ' ')
               {
                  ptr--;
               }

               ptr++;
               *ptr = '\0';
               strcpy(out, tmp);
               break;
            }
            else if ((*ptr == '[') || (*ptr == '(') || (*ptr == '_'))
            {
               teamFound = 1;
               strcpy(teamOut, ptr);
               // ptrName = ptr;

               ptr--;
               while (*ptr == ' ')
               {
                  ptr--;
               }

               ptr++;
               *ptr = '\0';

               strcpy(out, tmp);

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
      if (0) /* ((ret = TeamGrit(out)) == 1) */
      {
         strcpy(teamOut, "[GRIT]");
      }
      else
      {
         /* Just put in a dash for now */
         strcpy(teamOut, "-");
      }
   }
}

void TeamNameCleanup(char *nameOut, char *name, char *team, char *teamOut)
{
   int len;

   if (strstr(name, "MHA_ KOH") != 0)
   {
      strcpy(nameOut, "MHA KOH");
      strcpy(teamOut, "[JETT]");
   }
   else if (strstr(name, "Derek Sawyer") != 0)
   {
      strcpy(nameOut, "Derek Sawyer");
      strcpy(teamOut, "[GRIT][Rippers]");
   }
   else if (strstr(name, "Dan Nelson") != 0)
   {
      strcpy(nameOut, "Dan Nelson");
      strcpy(teamOut, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(name, "Kelly Toth") != 0)
   {
      strcpy(nameOut, "Kelly Toth");
      strcpy(teamOut, "CRCA/NYCC");
   }
   else if (strstr(name, "Bruch Wu") != 0)
   {
      strcpy(nameOut, "Bruch Wu");
      strcpy(teamOut, "-");
   }
   else if (strstr(name, "John Jeffries") != 0)
   {
      strcpy(nameOut, "John Jeffries");
      strcpy(teamOut, "[AA Bikes][GRIT]");
   }
   else if (strstr(name, "Guido Kramer Leymen") != 0)
   {
      strcpy(nameOut, "Guido Kramer Leymen");
      strcpy(teamOut, "-");
   }
   else if (strstr(name, "Paul Roy") != 0)
   {
      strcpy(nameOut, "Paul Roy");
      strcpy(teamOut, "RETO!");
   }
   else if (strstr(name, "Guillaume Lafleur") != 0)
   {
      strcpy(nameOut, "Guillaume Lafleur");
      strcpy(teamOut, "leMETIER");
   }
   else if ((strstr(name, "Bergenzaun") != 0) && (strstr(name, "ns ") != 0))
   {
      strcpy(nameOut, "Mans Bergenzaun");
      strcpy(teamOut, "TEAM BTC");
   }
   else if ((strstr(name, "Egon") != 0) && (strstr(name, "Ivan") != 0))
   {
      strcpy(nameOut, "Egon Ivanjsek");
      strcpy(teamOut, "PPRteam");
   }
   else if ((strstr(name, "Thomas") != 0) && (strstr(name, "Schr") != 0))
   {
      strcpy(nameOut, "Thomas Schrader");
      strcpy(teamOut, "ZRG");
   }
   else if (strstr(name, "C hris M") != 0)
   {
      strcpy(nameOut, "Chris M");
      strcpy(teamOut, "B4H");
   }
   else if (strstr(name, "Seth G ") != 0)
   {
      strcpy(nameOut, "Seth G");
      strcpy(teamOut, "[GRIT]");
   }
   else if (strstr(name, "Renfrew Bunch") != 0)
   {
      strcpy(nameOut, "Renfrew Bunch");
      strcpy(teamOut, "[Scotland]");
   }
   else if (!isprint(name[0]) && !isprint(name[1]) && !isprint(name[2]))
   {
      strcpy(nameOut, "__STAR__");
      strcpy(teamOut, "-");
   }
   else if ((strstr(name, "Bergenzaun") != 0) && (strstr(name, "M") != 0))
   {
      strcpy(nameOut, "Mans Bergenzaun");
      strcpy(teamOut, "Team BTC");
   }
   else if (strstr(name, "Dax Nelson") != 0)
   {
      strcpy(nameOut, "Dax Nelson");
      strcpy(teamOut, "!DUX! TPA-FLA HERD");
   }
   else if (strstr(name, "Greg Langman") != 0)
   {
      strcpy(nameOut, "Greg Langman");
      strcpy(teamOut, "DIRT");
   }
   else if ((strstr(name, "Hiroki") != 0) && (strstr(name, "Tanaka") != 0))
   {
      strcpy(nameOut, "Hiroki Tanaka");
      strcpy(teamOut, "-");
   }
   else if ((strstr(name, "Hans") != 0) && (strstr(name, "rgen") != 0))
   {
      strcpy(nameOut, "Hans-Jurgen G");
      strcpy(teamOut, "-");
   }
   else if ((strstr(name, "J B") != 0) && (strstr(team, "SWOZR") != 0))
   {
      strcpy(nameOut, "J B #1");
      strcpy(teamOut, "SWOZR");
   }
   else if ((strstr(name, "J B") != 0) && (strstr(team, "Bees") != 0))
   {
      strcpy(nameOut, "J B #2");
      strcpy(teamOut, "BCC - Bees");
   }
#if 0
   else if (strstr(name, "J B") != 0)
   {
      strcpy(nameOut, "J B");
      strcpy(teamOut, "-");
   }
#endif
   else if (strstr(name, "Sebastien Loir") != 0)
   {
      strcpy(nameOut, "Sebastien Loir");
      strcpy(teamOut, "BIKES-FR");
   }
   else if (strstr(name, "Greg Rashford") != 0)
   {
      strcpy(nameOut, "Greg Rashford");
      strcpy(teamOut, "#NOMAD");
   }
   else if (strstr(name, "Dan Newton") != 0)
   {
      strcpy(nameOut, "Dan Newton");
      strcpy(teamOut, "WGT - Bynea CC");
   }
   else if (strstr(name, "Dan Nelson") != 0)
   {
      strcpy(nameOut, "Dan Nelson");
      strcpy(teamOut, "!DUX! TPA-FLA - HERD");
   }
   else if ((strstr(name, "Rafa") != 0) && (strstr(name, "Dzieko") != 0))
   {
      strcpy(nameOut, "Rafa Dzieko");
      strcpy(teamOut, "ski ZTPL.CC");
   }
   else if (strstr(name, "conrad moss") != 0)
   {
      strcpy(nameOut, "Conrad Moss");
      strcpy(teamOut, "Velopower PC, P-TJ RT");
   }
   else if (strstr(name, "Oleg Semeko") != 0)
   {
      strcpy(nameOut, "Oleg Semeko");
      strcpy(teamOut, "Veter.cc");
   }
   else if (strstr(name, "Gabriel Mathisen") != 0)
   {
      strcpy(nameOut, "Gabriel Mathisen");
      strcpy(teamOut, "[VEGAN][GRIT]");
   }
   else if ((strstr(name, "Korea") != 0) && (strstr(name, "Marc") != 0))
   {
      strcpy(nameOut, "Korea Marc");
      strcpy(teamOut, "-");
   }
   else if ((strstr(name, "Ren") != 0) && (strstr(name, "Jeppesen") != 0))
   {
      strcpy(nameOut, "Rene Jeppesen");
      strcpy(teamOut, "-");
   }
   else if ((strstr(name, "tak ina") != 0) || (strstr(name, "Tak Ina") != 0))
   {
      strcpy(nameOut, "Tak Ina");
      strcpy(teamOut, "[JETT][GRIT]");
   }
   else if (strstr(name, "VV Cucumber") != 0)
   {
      strcpy(nameOut, "VV Cucumber");
      strcpy(teamOut, "[LOOKCYCLE CHN]");
   }
   else if (strstr(name, "Ricardo Abela") != 0)
   {
      strcpy(nameOut, "Ricardo Abela");
      strcpy(teamOut, "TEAM BMTR");
   }
#if 0
   else if (strstr(name, "Denis Frolov") != 0)
   {
      strcpy(nameOut, "Denis Frolov");
      strcpy(teamOut, "UTT");
   }
   else if (strstr(name, "Roy Woodland") != 0)
   {
      strcpy(nameOut, "Roy Woodland");
      strcpy(teamOut, "PACK");
   }
#endif
   else if (strstr(name, "Steve Tappan") != 0)
   {
      strcpy(nameOut, "Steve Tappan");
      strcpy(teamOut, "[GRIT]");
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
         strcpy(teamOut, "-");
      }
      else
      {
         strcpy(teamOut, team);
      }
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

void RemoveWpkgNew(CLI_PARSE_INFO *pInfo, char *str)
{
   char *p;
   if ((p = strstr(str, "wkg")) != NULL)
   {
      *p = '\0';
   }

   if ((p = strstr(str, "w  ")) != NULL)
   {
      *p = '\0';
   }
}

void ColumnizeResults(CLI_PARSE_INFO *pInfo, char *fname)
{
   int i;
   int outIdx = 0;

   FILE *fp_in;
   char tmp[MAX_STRING_SIZE];
   char teamName[MAX_STRING_SIZE];
   char teamName2[MAX_STRING_SIZE];
   char fixed[MAX_STRING_SIZE];
   char fixed2[MAX_STRING_SIZE];
   char header[MAX_STRING_SIZE];

   char tmp2[MAX_STRING_SIZE];
   char tmp3[MAX_STRING_SIZE];
   char raceName[MAX_STRING_SIZE];
   char tmpName[MAX_STRING_SIZE];
   char updatedLine[MAX_STRING_SIZE];
   char timeLine[MAX_STRING_SIZE];

   char tmpTeam[MAX_STRING_SIZE];
   char teamOut[MAX_STRING_SIZE];
   char nameOut[MAX_STRING_SIZE];
   char outStr[MAX_STRING_SIZE];

   int preRead = 0;
   int count = 0;
   int trigger = 0;
   int firstAthlete = 1;

   fp_in = fopen(fname, "r");

   if (!fp_in)
   {
      (pInfo->print_fp)("INTERNAL ERROR: Could not open %s\n", fname);
      exit(-1);
   }

   /* Create headers */
   outIdx = 0;
   outIdx += sprintf(&header[outIdx], "%s", "#  name  team  time  watts");
   ColumnStore(pInfo, header);

   sprintf(header, "%s", "--  ----   ----   ----   -----");
   ColumnStore(pInfo, header);

   while (1)
   {
      if (preRead == 0)
      {
         if (!StringGet2(tmp, fp_in))
         {
            /* Done reading file */
            break;
         }
      }

      preRead = 0;
      if ((strstr(tmp, "Description") != NULL) || (strstr(tmp, "Powered ") != NULL))
      {
         trigger = 0;
         continue;
      }

      if (((tmp[0] == 'A') || (tmp[0] == 'B') || (tmp[0] == 'C') || (tmp[0] == 'D')) &&
          (((tmp[1] == ' ') && (tmp[2] == ' ')) || (tmp[1] == '\n')) &&
          (strstr(tmp, "Distance") == NULL))
      {
         trigger = 1;
         continue;
      }

      if ((tmp[0] == '\n') || (tmp[0] == ' ') || (tmp[0] == '\r') || (tmp[0] == '\t'))
      {
         continue;
      }

      outIdx = 0;
      teamOut[0] = '\0';

      if (trigger == 1)
      {
         if (strstr(tmp, "Maximilian") != 0)
         {
            gdbStop = 1;
         }

         strcpy(tmpName, tmp);
         NameInsert(nameOut, tmpName, teamOut);

         // Team or empty
         if (!StringGet2(tmp, fp_in))
         {
            /* Done reading file */
            break;
         }

         // If team, skip
         if ((tmp[0] != '\n') && (tmp[0] != ' ') && (tmp[0] != '\r') && (tmp[0] != '\t'))
         {
            /* TEAM can be found in the name (Zwift does that) or on next line */
//                   TeamNameCleanup(timeLineInfo, tmp);
            strcpy(tmpTeam, teamOut);
            strcpy(tmpName, nameOut);
            TeamNameCleanup(nameOut, tmpName, tmp, teamOut);


            /* CATCH if/when we have DUPLICATE names (i.e., P B) with different teams */
//                     NameDouble(pInfo, timeLineInfo);

            /* We have team name, thus skip next <empty> line */
            if (!StringGet2(tmp, fp_in))
            {
               /* Done reading file */
               break;
            }
         }
         else
         {
            /* CATCH if/when we have DUPLICATE names (i.e., P B) with different teams */
//                     NameDouble(pInfo, timeLineInfo);
         }

         /* TEAM can be found in the name (Zwift does that) or on next line */
         outIdx += sprintf(&outStr[outIdx], "%d  %s  ", count, nameOut);
         count++;

         /* TEAM can be found in the name (Zwift does that) or on next line */
         outIdx += sprintf(&outStr[outIdx], "%s  ", teamOut);

         /* time */
         if (!StringGet2(tmp, fp_in))
         {
            /* Done reading file */
            break;
         }

#if 0
         /* SAVE time */
         SaveTime(pInfo, timeLineInfo, tmp);
         TimeLineInfoUpdate(pInfo, timeLineInfo, listTimeLine, raceName);
         TimeLineInfoInsert(pInfo, timeLineInfo, listTimeLine, RACE_NO);
#endif

         /* time */
         AddSpace(tmp2, tmp);
         outIdx += sprintf(&outStr[outIdx], "%s  ", tmp2);

         /* 1:04:01 */
         /* +27.769s */
         /*             3.8w/kg 274w */
         /* 283w */
         /*    4.0w/kg 4.4w/kg 4.9w/kg 5.6w/kg 6.8w/kg 72.6kg  169bpm  184bpm  172cm        */
         /* B 		*/

         /* OR */

         /* 46:05 */
         /* +5.312s */
         /*    4.0w/kg 217w                            54.0kg  143bpm      167cm        */
         /* B 		*/

         /* OR */

         /* 1:03:34 */
         /* 3.8w/kg 274w */
         /* 283w */
         /*    4.0w/kg 4.4w/kg 4.9w/kg 5.6w/kg 6.8w/kg 72.6kg  169bpm  184bpm  172cm        */
         /* B 		*/

         /* OR */

         /* 16:42   */
         /* 405w 5.6wkg */
         /*	        Power      */
         /* B 		*/

         /* OR */

         /* 16:42   */
         /* 405w 5.6wkg */
         /*             */
         /* 23.183      */
         /* 236w 3.3wkg */
         /*	        Power      */
         /* B 		*/

         /* Take a peek */
         /* + */
         if (!StringGet2(tmp, fp_in))
         {
            /* Done reading file */
            break;
         }

         /* If se see a '+' we know we are good, and can throw away */
         if (strstr(tmp, "+") != 0)
         {
            /* Throw away above, and get the next wpkg and watts*/

            /*             3.8w/kg 274w */
            if (!StringGet2(tmp, fp_in))
            {
               /* Done reading file */
               break;
            }

            AddSpace(tmp2, tmp);
            RemoveWpkgNew(pInfo, tmp2);

            outIdx += sprintf(&outStr[outIdx], "%s  ", tmp2);
            ColumnStore(pInfo, outStr);

            /* Peek */
            /* 283w - or B*/
            if (!StringGet2(tmp, fp_in))
            {
               /* Done reading file */
               break;
            }

            /* If we DO Not see a Letter, continue until we do !!! */
            if ((tmp[0] == 'A') || (tmp[0] == 'B') || (tmp[0] == 'C') || (tmp[0] == 'D'))
            {
               preRead = 1;
               continue;
            }

            /* Otherwise, throw away next */

            /*    4.0w/kg 4.4w/kg 4.9w/kg 5.6w/kg 6.8w/kg 72.6kg  169bpm  184bpm  172cm        */
            if (!StringGet2(tmp, fp_in))
            {
               /* Done reading file */
               break;
            }
         }
         else
         {
            /* 3.8w/kg 274w */
            /* 405w 5.6wkg */

            AddSpace(tmp2, tmp);
            RemoveWpkgNew(pInfo, tmp2);

            outIdx += sprintf(&outStr[outIdx], "%s  ", tmp2);
            ColumnStore(pInfo, outStr);

            if (strstr(tmp2, "w/kg") != 0)
            {
               /* 283w - or 'B' */
               if (!StringGet2(tmp, fp_in))
               {
                  /* Done reading file */
                  break;
               }

               /* If we DO Not see a Letter, continue until we do !!! */
               if ((tmp[0] == 'A') || (tmp[0] == 'B') || (tmp[0] == 'C') || (tmp[0] == 'D'))
               {
                  preRead = 1;
                  continue;
               }

               /*    4.0w/kg 4.4w/kg 4.9w/kg 5.6w/kg 6.8w/kg 72.6kg  169bpm  184bpm  172cm        */
               if (!StringGet2(tmp, fp_in))
               {
                  /* Done reading file */
                  break;
               }
            }
            else
            {
               /* 405w 5.6wkg */
               if (!StringGet2(tmp, fp_in))
               {
                  /* Done reading file */
                  break;
               }

               /*           Power      */
               if (!StringGet2(tmp, fp_in))
               {
                  /* Done reading file */
                  break;
               }

               /* THIS IS WHERE WE COULD HAVE MORE THAN ONE SET OF watts + Power */
               /* HOW do we account for MULTIPLE watts + Power */

               /* For now, we do a "pre" read */
               if (!StringGet2(tmp, fp_in))
               {
                  /* Done reading file */
                  break;
               }

               /* If we DO Not see a Letter, continue until we do !!! */
               preRead = 1;
               while (1)
               {
                  if ((tmp[0] != 'A') && (tmp[0] != 'B') && (tmp[0] != 'C') && (tmp[0] != 'D'))
                  {
                     /* means we have another combo of watts + Power - throw away !! */
                     if (!StringGet2(tmp, fp_in))
                     {
                        /* Done reading file */
                        break;
                     }

                     continue;
                  }
                  break;
               } /* while 1 */
            } /* ! w/kg */
         } /* ! + */
      } /* trigger == 1 */
   } /* while */

   fclose(fp_in);

   PrintTable(pInfo);
}

void Columnize(CLI_PARSE_INFO *pInfo, char *fname, int mode)
{
   if (mode == COLUMNIZE_FOLLOWING)
   {
      ColumnizeFollowing(pInfo, fname);
   }
   else if (mode == COLUMNIZE_RESULTS)
   {
      ColumnizeResults(pInfo, fname);
   }
}

static void cmd_columnize_common(CLI_PARSE_INFO *pInfo, int mode)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {filename}\n", pInfo->argv[0]);
      return;
   }

   ColumnReset(pInfo);
   Columnize(pInfo, pInfo->argv[1], mode);
}

static void cmd_columnize_following(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {filename}\n", pInfo->argv[0]);
      return;
   }

   cmd_columnize_common(pInfo, COLUMNIZE_FOLLOWING);
}

static void cmd_columnize_results(CLI_PARSE_INFO *pInfo)
{
   if ( pInfo->argc < 2)
   {
      (pInfo->print_fp)("USAGE: %s {filename}\n", pInfo->argv[0]);
      return;
   }

   cmd_columnize_common(pInfo, COLUMNIZE_RESULTS);
}

static const CLI_PARSE_CMD cmd_columnize_opt[] =
{
   { "following", cmd_columnize_following, "columnize following list"},
   { "results",   cmd_columnize_results,   "columnize results list"},
   { NULL, NULL, NULL }
};


static void cmd_columnize(CLI_PARSE_INFO *pInfo)
{
    cliDefaultHandler( pInfo, cmd_columnize_opt );
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

/* file: clicmds_common.c */

CLI_PARSE_CMD cliRocEntry[] =
{
    { "help",             cmdHelp,             "Show Help"},
    { ".help",            cmdHelp,             "Show Help"},
    { "?",                cmdHelp,             "Show Help"},
    { ".?",               cmdHelp,             "Show Help"},
    { "exit",             cmd_exit,            "exit" },
    { "echo",             cmd_echo,            "echo" },
    { "quit",             cmd_quit,            "quit" },
    { "self",             PartySelf,           "self test"},
    { "source",           cmd_source,          "source" },
    { "columnize",        cmd_columnize,       "columnize" },
    { NULL,               NULL,                NULL }
};

void cmdHelp( CLI_PARSE_INFO *info)
{
  cliPrintHelp( info, cliRocEntry );
}

void
PrintUsage(int exitFlag)
{
    printf("Usage: columnprintf [OPTION...]\n");
    printf("\n");
    printf("  -s, --script       run script 1/second\n");
    printf("  -k, --kom          columnize {filename} kom results\n");
    printf("  -r, --results      columnize {filename} results\n");
#if 0
    printf("  -D, --DDR          start DSP Data Record (DDR) recording...\n");
    printf("  -P, --PDR          start PFE/DSP Data Record (PDR) recording...\n");
    printf("  -L, --Local        run locally - do not connect to server (q20 support)\n");
    printf("  -T, --Top          top - screen update every -T {seconds}\n");
    printf("  -R, --Refresh      top - idle task refresh rate (default - refresh after 4 idle ticks)\n");
    printf("  -C, --Cut          top - cut idle and non-idle tasks into two tables (default single table)\n");
    printf("  -F, --Flow         top - disable automatic reset of screen - enable flow of results\n");
    printf("  -r, --rate         change rate of execution\n");
    printf("  -I, --ip-primary   primiary servier IP address\n");
    printf("  -S, --ip-secondary secondary servier IP address\n");
    printf("  -u, --usdp         connect to USDP (default USBC)\n");
    printf("  -m, --monitor      connect to monitor (default USBC)\n");
    printf("  -e, --media        connect to media (default USBC)\n");
    printf("  -U, --unittest     connect to UNITTEST jig\n");
#endif
    printf("  -d, --debug        turn debug messages on (1) or off (0)\n");
    printf("  -?, --help         show this help information\n");
    printf("  -h, --help         show this help information\n");

    if (exitFlag == 1)
    {
        exit(-1);
    }
}

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
    arguments.filename[0] = '\0';
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
    arguments.mode = COLUMNIZE_UNKNOWN;

    while (1)
    {
        int option_index = 0;

        static struct option long_options[] =
            {
                {"kom",               required_argument,    0, 'k'},
                {"results",           required_argument,    0, 'r'},
                {"script",            required_argument,    0, 's'},
#if 0
                {"PDR",               no_argument,          0, 'P'},
                {"Local",             no_argument,          0, 'L'},
                {"Top",               required_argument,    0, 'T'},
                {"Refresh",           required_argument,    0, 'R'},
                {"Cut",               no_argument,          0, 'C'},
                {"Flow",              no_argument,          0, 'F'},
                {"rate",              required_argument,    0, 'r'},
                {"ip-primary",        required_argument,    0, 'I'},
                {"ip-secondary",      required_argument,    0, 'S'},
                {"usdp",              no_argument,          0, 'u'},
                {"monitor",           no_argument,          0, 'm'},
                {"media",             no_argument,          0, 'e'},
                {"unittest",          no_argument,          0, 'U'},
#endif
                {"debug",             required_argument,    0, 'd'},
                {"help",              no_argument,          0, '?'},
                {"help",              no_argument,          0, 'h'},
                {0,0,0,0}
            };

        c = getopt_long_only(argc, argv, "k:PLT:R:CFs:r:I:S:umeUd:h?",
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
            // arguments.rate = strtoul(optarg, NULL, 10);
            arguments.mode = COLUMNIZE_RESULTS;
            strcpy(arguments.filename, optarg);
            break;

        case 'I':
            strcpy(arguments.ip_primary, optarg);
            break;

        case 'S':
            strcpy(arguments.ip_secondary, optarg);
            break;

        case 'k':
            arguments.mode = COLUMNIZE_FOLLOWING;
            strcpy(arguments.filename, optarg);
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

    if (arguments.mode == COLUMNIZE_FOLLOWING)
    {
       sprintf(newCommandLine, "columnize following %s", arguments.filename);
       cliEntry(newCommandLine, &printf);
       exit(0);
    }

    if (arguments.mode == COLUMNIZE_RESULTS)
    {
       sprintf(newCommandLine, "columnize results %s", arguments.filename);
       cliEntry(newCommandLine, &printf);
       exit(0);
    }

    input[0] = '\0';
    while (1)
    {
        printf ("columnprintf >> ");

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
