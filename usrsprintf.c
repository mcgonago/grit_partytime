/***************************************************************************** 
File:     usrprintf.c
Contents: User level printf routine.

Copyright (c) 2005 Reef Point Systems Inc.
All Rights Reserved.

8 New England Executive Park
Burlington, MA 01803
781-505-8316
*****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "usrsprintf.h"
#include "zcli.h"

//typedef unsigned long uint32_t;

void SysNewLineStrip(char *buff);

static char inputString[MAX_STRING_SIZE];
//static char outputString[MAX_STRING_SIZE];
static char columnString[MAX_STRING_SIZE];
static char fileCopy[MAX_STRING_SIZE];
static char functionCopy[MAX_STRING_SIZE];
static char statusCopy[MAX_STRING_SIZE];
//static char strCmd[255];

int
usrSPrintf(char *buffer, const char *file, const char *function, int line, const char *status, const char *format, ...)
{
    va_list args;
    int i;
    int numberOfSpaces;
    char *ptr, *ptr2;
    
    int nextPtr = 0;
    int inputLen = 0;
    int len = 0;
    int len2 = 0;
    int len3 = 0;

#if 0
    struct tm *ptm;
    char *timeAndDate;
    time_t rawtime;

    time(&rawtime);
    ptm = localtime(&rawtime);
    timeAndDate = asctime(ptm);

    /* Get rid of newline */
    timeAndDate[strlen(timeAndDate) - 1] = '\0';
    nextPtr += sprintf(&buffer[0], "[%s] ", timeAndDate);
#endif
    
    do
    {
        if (((COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 1) < 2) ||
            ((COLUMN_TWO_WIDTH - SPACES_BETWEEN_COLS - 1) < 2) ||
            ((COLUMN_THREE_WIDTH - SPACES_BETWEEN_COLS - 1) < 2))
        {
            nextPtr += sprintf(&buffer[nextPtr], "ERROR: Could not fit requested string");
            break;
        }

        va_start(args ,format);
        inputLen = vsprintf(inputString, format, args);
        va_end(args);

        if (status == COLUMN_FORMATTING_DISABLE)
        {
            /* Print out input string, without fancy formatting of columns */
            nextPtr += sprintf(&buffer[nextPtr], inputString);
            break;
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
                       strcpy(&fileCopy[0], ptr);
                   }
                   else
                   {
                       ptr2++;
                       strcpy(&fileCopy[0], ptr2);
                   }
               }
               else
               {
                   strcpy(&fileCopy[0], file);
               }
            }
            else
            {
                fileCopy[0] = '\0';
            }
        
            if ((function != NULL) && (len = (strlen(function)) < MAX_STRING_SIZE) && (len > 0))
            {
                strcpy(functionCopy, (char *) function);
            }
            else
            {
                functionCopy[0] = '\0';
            }
        
            if ((status != NULL) && (len = (strlen(status)) < MAX_STRING_SIZE) && (len > 0))
            {
                strcpy(statusCopy, (char *) status);
            }
            else
            {
                statusCopy[0] = '\0';
            }

            if (inputLen > 0)
            {
                if (TraceColumnOn(1) == 1)
                {
                    /* Format the [{file}:{line}] column */
                    if (fileCopy[0] == '\0')
                    {
                        if (line == 0)
                        {
                            len = sprintf(columnString, "[xxxxx:0]");
                        }
                        else
                        {
                            len = sprintf(columnString, "[xxxxx:%d]", line);
                        }
                    }
                    else
                    {
                        if (line == 0)
                        {
                            len = sprintf(columnString, "[%s]", fileCopy);
                        }
                        else
                        {
                            len = sprintf(columnString, "[%s:%d]", fileCopy, line);
                        }
                    }
                    
                
                    /* See if length of [{file}:{line}] exceeds column width */
                    if (len > COLUMN_ONE_WIDTH)
                    {
#if 1
                        /* Keep most of the last part of filename:line */
                        // len = strlen(columnString) - COLUMN_ONE_WIDTH - 1;
                        // len2 = len - COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 2;
                        columnString[0] = '[';
                        len2 = len - COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 2;
                        len3 = len - len2;

                        strncpy(&columnString[1], &columnString[len2 + 6], (len3 - 6));
                        columnString[1 + (len3 - 6)] = '\0';
                        len = 1 + (len3 - 6);
#else
                        /* Shorten the file name */
                        fileCopy[(strlen(fileCopy))/2] = '\0';

                        /* Try it again */
                        len = sprintf(columnString, "[%s:%d]", fileCopy, line);

                        if (len > COLUMN_ONE_WIDTH)
                        {
                            /* Truncate it */
                            columnString[COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS - 1] = ']';
                            columnString[COLUMN_ONE_WIDTH - SPACES_BETWEEN_COLS]     = '\0';
                            len = strlen(columnString);
                        }
#endif
                    }

                    /* print [{file}:{line}] */
                    nextPtr += sprintf(&buffer[nextPtr], columnString);

                    /* Fill spaces to start of next column */
                    numberOfSpaces = (COLUMN_ONE_WIDTH - len) + SPACES_BETWEEN_COLS;
                    for (i = 0; i < numberOfSpaces; i++)
                    {
                        nextPtr += sprintf(&buffer[nextPtr], " ");
                    }
                }

                if (TraceColumnOn(2) == 1)
                {
                    /* Format the [{function}] column */
                    if  (functionCopy[0] == '\0')
                    {
                        len = sprintf(columnString, "[xxxxx]");
                    }
                    else
                    {
                        len = sprintf(columnString, "[%s]", functionCopy);
                    }
                
                    /* See if [{function}] exceeds column width */
                    if (len > COLUMN_TWO_WIDTH)
                    {
                        /* Truncate it */
                        columnString[COLUMN_TWO_WIDTH - SPACES_BETWEEN_COLS - 1] = ']';
                        columnString[COLUMN_TWO_WIDTH - SPACES_BETWEEN_COLS]     = '\0';
                        len = strlen(columnString);
                    }

                    nextPtr += sprintf(&buffer[nextPtr], columnString);

                    /* Fill spaces to start of next column */
                    numberOfSpaces = (COLUMN_TWO_WIDTH - len) + SPACES_BETWEEN_COLS;
                    for (i = 0; i < numberOfSpaces; i++)
                    {
                        nextPtr += sprintf(&buffer[nextPtr], " ");
                    }
                }

                if (TraceColumnOn(3) == 1)
                {
                    if (statusCopy[0] == '\0')
                    {
                        len = sprintf(columnString, "[xxxxx]");
                    }
                    else
                    {
                        len = sprintf(columnString, "%s", statusCopy);
                    }

                    if (len > COLUMN_THREE_WIDTH)
                    {
                        /* Truncate it */
                        statusCopy[COLUMN_THREE_WIDTH - SPACES_BETWEEN_COLS] = '\0';
                        len = strlen(statusCopy);
                    }
                    
                    /* Print status string */
                    nextPtr += sprintf(&buffer[nextPtr], statusCopy);

                    /* Fill spaces to start of next column */
                    numberOfSpaces = (COLUMN_THREE_WIDTH - len) + SPACES_BETWEEN_COLS;
                    for (i = 0; i < numberOfSpaces; i++)
                    {
                        nextPtr += sprintf(&buffer[nextPtr], " ");
                    }
                }

                if (TraceColumnOn(4) == 1)
                {
                    if (inputLen > COLUMN_FOUR_WIDTH)
                    {
                        /* Truncate it */
                        inputString[COLUMN_FOUR_WIDTH - SPACES_BETWEEN_COLS] = '\0';
                        inputLen = strlen(inputString);
                    }

                    /* Quick check to remove trailing newlines and/or carriage returns */
                    SysNewLineStrip(inputString);
                
                    /* Print out input string */
                    nextPtr += sprintf(&buffer[nextPtr], inputString);
                    /* +++owen - DO NOT DO THIS WHEN SENDING TO TstPuts, since puts shoves in the new line for you. */
                    nextPtr += sprintf(&buffer[nextPtr], "\n");
                }
            }
            else
            {
                nextPtr += sprintf(&buffer[0], "ERROR: sysPrintf: Could not format input string\n");
            }
        }
    } while (0);
    
    //printf("%s", buffer);
    //sprintf(strCmd, "TstPuts \"%s \" ", buffer);
    //Tcl_Eval(gpTclInterp, strCmd);

    //sprintf(strCmd, "puts \"Testing, 1,2,3, ... \" ");
    //Tcl_Eval(gpTclInterp, strCmd);

    //sprintf(strCmd, "puts \"%s \" ", buffer);
    //Tcl_Eval(gpTclInterp, strCmd);
    
    return(len); 
}

void
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
