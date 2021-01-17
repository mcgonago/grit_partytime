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
#include "partycontrol.h"

#if 0
#include "xstatcommands.h"
#include "uint64commas.h"
#include "xstatcustom.h"
#include "rcli_server.h"
#include "rcli.h"
#include "countercustom.h"
#include "xstatcolor.h"
#endif

#include "zcli.h"
#include "zcolor.h"
#include "tracebuffer.h"
#include "timeline.h"
#include "usrsprintf.h"
#include "columnprintf.h"
#include "partycontrol.h"

PartyControl_t partyControl;

void PartyInit(CLI_PARSE_INFO *pInfo)
{
   int i;

   partyControl.max = -1;
   partyControl.bestOf = BEST_OF_ONE;
   partyControl.calendar = 0;
   partyControl.pr = 0;
   partyControl.adjust = 1;
   partyControl.tallyMin = TALLY_MIN;
   partyControl.bonus = 0;
   partyControl.doubleUp = 0;
   partyControl.ageBonus = 0;
   partyControl.debug = 0;
   partyControl.groupId = GROUP_UNKNOWN;
   partyControl.lockDown = 0;
   partyControl.clip = 0;
   partyControl.cat = 0;
   partyControl.cat1 = 0.0;
   partyControl.cat2 = 0.0;
   partyControl.cat3 = 0.0;

   ColumnReset(pInfo);
}
