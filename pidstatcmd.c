#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>
#include <pthread.h>

#include "zcli.h"
#include "zcolor.h"
#include "tracebuffer.h"
#include "timeline.h"
#include "link.h"
#include "linklist.h"

#if 0
#define DEBUG2(fmt, args...) (pInfo->print_fp)(fmt, ##args)
#else
#define DEBUG2(fmt, args...)
#endif

XStatCmdControl_t xstatCmdControl;

/* Default settings */    
PidStatConfig_t pidStatConfig;

typedef struct FilterLibrary_s
{
    uint32_t enabled;
    char name[MAX_NAME];
} FilterLibrary_t;

/* Generic filter - lots of noise */
FilterLibrary_t filterLibraryA[] = {
    {1,"Lance Anderson"},
    {1,"Alan Brannan"},
    {1,"Shaun Corbin"},
    {1,"Luke Elton"},
    {1,"Rob Fullerton"},
    {1,"Seth G"},
    {1,"tak ina"},
    {1,"John Jeffries"},
    {1,"Gabriel Mathisen"},
    {1,"Owen McGonagle"},
    {1,"Steve Peplinski"},
    {1,"Carson Shedd"},
    {1,"Steve Tappan"},
    {1,"Maximilian Weniger"},
    {1,"Derek Sawyer"},
    {0,"FILTER_END"},
};

/* Signaling only */
FilterLibrary_t filterLibraryB[] = {
    {0,"FILTER_END"},
};

FilterLibrary_t filterLibraryC[] = {
    {0,"FILTER_END"},
};

FilterLibrary_t filterLibraryD[] = {
    {0,"FILTER_END"},
};

#define MAX_USER_FILTERS (1024)

FilterLibrary_t filterLibraryUser[MAX_USER_FILTERS] = {
    {0,"FILTER_END"},
};

uint32_t PidModeGet(CLI_PARSE_INFO *info)
{
    return(pidStatConfig.mode);
}

int TeamGrit(char *name)
{
   int i;
   for (i = 0; ;i++)
   {
      if (strstr(filterLibraryA[i].name, "FILTER_END") != 0)
      {
         break;
      }

      if (strstr(name, filterLibraryA[i].name) != 0)
      {
         return 1;
      }
      
   }

   return 0;
}

static void PartyTimeSetMode(CLI_PARSE_INFO *info)
{
    if ( info->argc != 2)
    {
        (info->print_fp)("USAGE: %s {on|off}\n", info->argv[0]);
        return;
    }

    if (strcmp(info->argv[1], "on") == 0)
    {
        pidStatConfig.mode = 1;
    }
    else if (strcmp(info->argv[1], "off") == 0)
    {
        pidStatConfig.mode = 0;
    }
    else
    {
        (info->print_fp)("USAGE: %s {on|off}\n", info->argv[0]);
        return;
    }
}

static const CLI_PARSE_CMD partytime_filter_set[] =
{
    { "mode",         PartyTimeSetMode,          "set mode"},
    { NULL,           NULL,                              NULL }
};


static void PartyTimeSet(CLI_PARSE_INFO *info)
{
    cliDefaultHandler(info, partytime_filter_set);
}

static void PartyTimeShow(CLI_PARSE_INFO *info)
{
    (info->print_fp)("mode             = %s\n", ((pidStatConfig.mode == 1) ? "ON" : "OFF"));
}

static void PartyTimeReset(CLI_PARSE_INFO *info)
{
   pidStatConfig.mode = 0;
}

static void LibraryList(CLI_PARSE_INFO *info)
{
    (info->print_fp)("A:    Generic Linux\n");
    (info->print_fp)("B:    Generic Signaling\n");
    (info->print_fp)("C:    iMedia only\n");
    (info->print_fp)("B:    UA thread Signaling\n");
    (info->print_fp)("USER: User defined filter library\n");
}

static void PartyTimeFilterLibraryShow(CLI_PARSE_INFO *info)
{
    int i;
    uint32_t showMask = 0;
    
    if ( info->argc != 2)
    {
        (info->print_fp)("USAGE: %s {A|B|C|D|USER} | all\n", info->argv[0]);
        (info->print_fp)("where: {A|B|C|D|USER} reflect one of the following:\n");
        LibraryList(info);
        return;
    }

    if (strcmp(info->argv[1], "A") == 0)
    {
        showMask |= FILTER_SHOW_A;
    }
    else if (strcmp(info->argv[1], "B") == 0)
    {
        showMask |= FILTER_SHOW_B;
    }
    else if (strcmp(info->argv[1], "C") == 0)
    {
        showMask |= FILTER_SHOW_C;
    }
    else if (strcmp(info->argv[1], "D") == 0)
    {
        showMask |= FILTER_SHOW_C;
    }
    else if (strcmp(info->argv[1], "USER") == 0)
    {
        showMask |= FILTER_SHOW_USER;
    }
    else if (strcmp(info->argv[1], "all") == 0)
    {
        showMask |= FILTER_SHOW_ALL;
    }

    if (showMask & FILTER_SHOW_A)
    {
        (info->print_fp)("Filter A:\n");
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryA[i].name, "FILTER_END") != 0)
            {
                break;
            }
            (info->print_fp)("%d: %s\n", i, filterLibraryA[i].name);
        }
    }
    
    if (showMask & FILTER_SHOW_B)
    {
        (info->print_fp)("Filter B:\n");
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryB[i].name, "FILTER_END") != 0)
            {
                break;
            }
            (info->print_fp)("%d: %s\n", i, filterLibraryB[i].name);
        }
    }
    
    if (showMask & FILTER_SHOW_C)
    {
        (info->print_fp)("Filter C:\n");
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryC[i].name, "FILTER_END") != 0)
            {
                break;
            }
            (info->print_fp)("%d: %s\n", i, filterLibraryC[i].name);
        }
    }
    
    
    if (showMask & FILTER_SHOW_D)
    {
        (info->print_fp)("Filter D:\n");
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryD[i].name, "FILTER_END") != 0)
            {
                break;
            }
            (info->print_fp)("%d: %s\n", i, filterLibraryD[i].name);
        }
    }
    
    if (showMask & FILTER_SHOW_USER)
    {
        (info->print_fp)("Filter USER:\n");
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryUser[i].name, "FILTER_END") != 0)
            {
                break;
            }
            (info->print_fp)("%d: %s\n", i, filterLibraryUser[i].name);
        }
    }
}

int Skip(CLI_PARSE_INFO *info, char *line)
{
    int i;
    int skip = 0;
    
    if (pidStatConfig.filterSelect == FILTER_SELECT_NONE)
    {
        pidStatConfig.filterSelect |= FILTER_SELECT_USER;
    }
    
    if (pidStatConfig.filterSelect & FILTER_SELECT_A)
    {
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryA[i].name, "FILTER_END") != 0)
            {
                break;
            }

            if (strstr(line, filterLibraryA[i].name) != 0)
            {
                if (pidStatConfig.filterMatchYesSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
            else
            {
                if (pidStatConfig.filterMatchNoSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
        }
    }
    
    if (pidStatConfig.filterSelect & FILTER_SELECT_B)
    {
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryB[i].name, "FILTER_END") != 0)
            {
                break;
            }

            if (strstr(line, filterLibraryB[i].name) != 0)
            {
                if (pidStatConfig.filterMatchYesSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
            else
            {
                if (pidStatConfig.filterMatchNoSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
        }
    }
    
    if (pidStatConfig.filterSelect & FILTER_SELECT_C)
    {
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryC[i].name, "FILTER_END") != 0)
            {
                break;
            }

            if (strstr(line, filterLibraryC[i].name) != 0)
            {
                if (pidStatConfig.filterMatchYesSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
            else
            {
                if (pidStatConfig.filterMatchNoSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
        }
    }
    
    
    if (pidStatConfig.filterSelect & FILTER_SELECT_D)
    {
        for (i = 0; ;i++)
        {
            if (strstr(filterLibraryD[i].name, "FILTER_END") != 0)
            {
                break;
            }

            if (strstr(line, filterLibraryD[i].name) != 0)
            {
                if (pidStatConfig.filterMatchYesSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
            else
            {
                if (pidStatConfig.filterMatchNoSkip == 1)
                {
                    /* Skip it */
                    return 1;
                }
                else
                {
                    /* Keep it */
                    return 0;
                }
            }
        }
    }
    
    if (pidStatConfig.filterSelect & FILTER_SELECT_USER)
    {
        if (pidStatConfig.filterMatchYesSkip == 1)
        {
            skip = 0;
            
            for (i = 0; ;i++)
            {
                if (strstr(filterLibraryUser[i].name, "FILTER_END") != 0)
                {
                    break;
                }

                if (strstr(line, filterLibraryUser[i].name) != 0)
                {
                    skip = 1;
                    break;
                }
            }
        }

        if (skip == 0)
        {
            if (pidStatConfig.filterMatchNoSkip == 1)
            {
                skip = 1;
            
                for (i = 0; ;i++)
                {
                    if (strstr(filterLibraryUser[i].name, "FILTER_END") != 0)
                    {
                        break;
                    }

                    if (strstr(line, filterLibraryUser[i].name) != 0)
                    {
                        skip = 0;
                        break;
                    }
                }
            }
        }
    }
    

    /* Keep it */
    return skip;
}


static void PartyTimeFilterLibraryChoose(CLI_PARSE_INFO *info)
{

    if ( info->argc != 2)
    {
        (info->print_fp)("USAGE: %s {A|B|C|D|USER} | all\n", info->argv[0]);
        (info->print_fp)("where: {A|B|C|D|USER} reflect one of the following:\n");
        LibraryList(info);
        return;
    }

    if (strcmp(info->argv[1], "A") == 0)
    {
        pidStatConfig.filterSelect |= FILTER_SELECT_A;
    }
    else if (strcmp(info->argv[1], "B") == 0)
    {
        pidStatConfig.filterSelect |= FILTER_SELECT_B;
    }
    else if (strcmp(info->argv[1], "C") == 0)
    {
        pidStatConfig.filterSelect |= FILTER_SELECT_C;
    }
    else if (strcmp(info->argv[1], "D") == 0)
    {
        pidStatConfig.filterSelect |= FILTER_SELECT_C;
    }
    else if (strcmp(info->argv[1], "USER") == 0)
    {
        pidStatConfig.filterSelect |= FILTER_SELECT_USER;
    }
    else if (strcmp(info->argv[1], "all") == 0)
    {
        pidStatConfig.filterSelect |= FILTER_SELECT_ALL;
    }
}

static void PartyTimeFilterLibraryDefine(CLI_PARSE_INFO *info)
{
    int i,j;
    int idx = 0;
    char cmd[MAX_COMMAND_LENGTH];

    if ( info->argc < 2)
    {
        (info->print_fp)("USAGE: %s {name}\n", info->argv[0]);
        return;
    }
    
    /* Find first available slot */
    for (i = 0; i < MAX_USER_FILTERS; i++)
    {
        if (strstr(filterLibraryUser[i].name, "FILTER_END") != 0)
        {
            break;
        }
    }

    if (i == MAX_USER_FILTERS)
    {
        (info->print_fp)("INFO: No more available filter slots - maximum number = %d\n", MAX_USER_FILTERS);
    }

#if 0
    for (j = 1; j < info->argc; j++, i++)
    {
        if (i == MAX_USER_FILTERS)
        {
            (info->print_fp)("INFO: No more available filter slots - maximum number = %d\n", MAX_USER_FILTERS);
        }

        strcpy(filterLibraryUser[i].name, info->argv[j]);
        filterLibraryUser[i].enabled = 1;
    }
#else 
    /* package command line arguments into a single string */
    memset(cmd, 0, MAX_COMMAND_LENGTH);

    for (j = 0; j < (info->argc - 1); j++)
    {
       /* Package up argc and argv */
       idx += sprintf(&cmd[idx], "%s", (char *)info->argv[j+1]);

       cmd[idx++] = ' ';

       if (idx >= MAX_COMMAND_LENGTH)
       {
          (info->print_fp)("Command exceeded maximum length of %d characters\n", MAX_COMMAND_LENGTH);
          return;
       }
    }

#if 0
    cmd[idx-1] = '\n';
    cmd[idx] = '\0';
#else
    cmd[idx-1] = '\0';
#endif

    strcpy(filterLibraryUser[i].name, cmd);
#endif        

    filterLibraryUser[i].enabled = 1;

    if (i == MAX_USER_FILTERS)
    {
        (info->print_fp)("INFO: No more available filter slots - maximum number = %d\n", MAX_USER_FILTERS);
        i -= 1;
    }

    /* Set new END */
    strcpy(filterLibraryUser[i+1].name, "FILTER_END");
    filterLibraryUser[i+1].enabled = 0;

    /* Show what we have so far */
    for (i = 0; ; i++)
    {
        if (strstr(filterLibraryUser[i].name, "FILTER_END") != 0)
        {
            break;
        }

        (info->print_fp)("%3d: [%d] %s\n", i, filterLibraryUser[i].enabled, filterLibraryUser[i].name);
    }
}

static void PartyTimeFilterLibraryReset(CLI_PARSE_INFO *info)
{
    int i;

    for (i = 0; ;i++)
    {
        if (strstr(filterLibraryA[i].name, "FILTER_END") != 0)
        {
            break;
        }
        filterLibraryA[i].enabled = 1;
    }

    for (i = 0; ;i++)
    {
        if (strstr(filterLibraryB[i].name, "FILTER_END") != 0)
        {
            break;
        }
        filterLibraryB[i].enabled = 1;
    }

    for (i = 0; ;i++)
    {
        if (strstr(filterLibraryC[i].name, "FILTER_END") != 0)
        {
            break;
        }
        filterLibraryC[i].enabled = 1;
    }

    for (i = 0; ;i++)
    {
        if (strstr(filterLibraryD[i].name, "FILTER_END") != 0)
        {
            break;
        }
        filterLibraryD[i].enabled = 1;
    }

    for (i = 0; ;i++)
    {
        if (strstr(filterLibraryUser[i].name, "FILTER_END") != 0)
        {
            break;
        }
        filterLibraryUser[i].enabled = 1;
    }
}

static void PartyTimeFilterLibraryList(CLI_PARSE_INFO *info)
{
    LibraryList(info);
}

static void PartyTimeFilterMatchYesSkip(CLI_PARSE_INFO *info)
{
    /* Default */
    pidStatConfig.filterMatchYesSkip = 1;
}

static void PartyTimeFilterMatchYesKeep(CLI_PARSE_INFO *info)
{
    pidStatConfig.filterMatchYesSkip = 0;
}

static const CLI_PARSE_CMD partytime_filter_cmd_match_yes[] =
{
    { "skip",          PartyTimeFilterMatchYesSkip,  "Match Yes, skip (skip == true)  - default"},
    { "keep",          PartyTimeFilterMatchYesKeep,  "Match Yes, keep (skip == false) - user selection"},
    { NULL,           NULL,                          NULL}
};

static void PartyTimeFilterMatchYes(CLI_PARSE_INFO *info)
{
    cliDefaultHandler(info, partytime_filter_cmd_match_yes);
}

static void PartyTimeFilterMatchNoKeep(CLI_PARSE_INFO *info)
{
    /* Default */
    pidStatConfig.filterMatchNoSkip = 0;
}

static void PartyTimeFilterMatchNoSkip(CLI_PARSE_INFO *info)
{
    pidStatConfig.filterMatchNoSkip = 1;
}


static const CLI_PARSE_CMD partytime_filter_cmd_match_no[] =
{
    { "keep",          PartyTimeFilterMatchNoKeep,  "Match No,  keep (skip == false) - default"},
    { "skip",          PartyTimeFilterMatchNoSkip,  "Match No,  skip (skip == true)  - user selection "},
    { NULL,           NULL,                                 NULL}
};

static void PartyTimeFilterMatchNo(CLI_PARSE_INFO *info)
{
    cliDefaultHandler(info, partytime_filter_cmd_match_no);
}

static const CLI_PARSE_CMD partytime_filter_cmd_match[] =
{
    { "yes",          PartyTimeFilterMatchYes,      "keep or skip on match (default skip)"},
    { "no",           PartyTimeFilterMatchNo,       "keep or skip on no match (default keep) "},
    { NULL,           NULL,                         NULL}
};

static void PartyTimeFilterMatch(CLI_PARSE_INFO *info)
{
    cliDefaultHandler(info, partytime_filter_cmd_match);
}

static const CLI_PARSE_CMD partytime_library_cmd[] =
{
    { "choose",        PartyTimeFilterLibraryChoose,  "choose filtering library (default user defined)"},
    { "match",         PartyTimeFilterMatch,          "skip on match, or, keep"},
    { "list",          PartyTimeFilterLibraryList,    "list available libraries"},
    { "show",          PartyTimeFilterLibraryShow,    "show filtering library"},
    { "define",        PartyTimeFilterLibraryDefine,  "define filtering library"},
    { "reset",         PartyTimeFilterLibraryReset,   "reset filters to default values"},
    { NULL,           NULL,                           NULL }
};

static void PartyTimeLibrary(CLI_PARSE_INFO *info)
{
    cliDefaultHandler(info, partytime_library_cmd);
}


void PidStatInitCmd(CLI_PARSE_INFO *pInfo)
{                                             
    static uint32_t firstTime = 1;

    if (firstTime == 1)
    {
        firstTime = 0;
        pidStatConfig.mode                = 0;                                                                     
        pidStatConfig.filterSelect        = FILTER_SELECT_USER;                                                    
        pidStatConfig.filterMatchYesSkip  = 0;                   /* Match Yes, keep (skip == 0) - default */       
        pidStatConfig.filterMatchNoSkip   = 1;                   /* Match No,  skip (skip == 1) - default */
    }
}                                  


static const CLI_PARSE_CMD partytime_filter_cmd[] =
{
    { "set",          PartyTimeSet,           "set commands"},
    { "reset",        PartyTimeReset,         "reset filter"},
    { "show",         PartyTimeShow,          "show filter"},
    { "library",      PartyTimeLibrary,       "filter library commands"},
    { NULL,           NULL,                   NULL }
};

void PartyTimeFilterCmd( CLI_PARSE_INFO *pInfo)
{
    PidStatInitCmd(pInfo);
    
    cliDefaultHandler(pInfo, partytime_filter_cmd);
}
