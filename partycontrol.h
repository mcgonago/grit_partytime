#ifndef __PARTYCONTROL_H__
#define __PARTYCONTROL_H__

#include "zcli.h"

#define BEST_OF_ONE   (1)
#define BEST_OF_TWO   (2)
#define BEST_OF_THREE (3)
#define BEST_OF_FOUR  (4)

#define TALLY_MIN     (1)

#define TYPE_USBC    (1)
#define TYPE_USDP    (2)
#define TYPE_UNKNOWN (3)

#define NL_REMOVE (1)
#define NL_KEEP   (2)

#define SHOW_POINTS   (1)
#define SHOW_TALLY    (2)
#define SHOW_TEMPOISH (3)
#define SHOW_TIME     (4)

#define GROUP_A       (1)
#define GROUP_B       (2)
#define GROUP_ALL     (3)
#define GROUP_GRIT    (4)
#define GROUP_UNKNOWN (5)

typedef struct PartyControl_s
{
   int max;
   int bestOf;
   int calendar;
   int tallyMin;
   int bonus;
   int doubleUp;
   int ageBonus;
   int debug;
   int groupId;
   int lockDown;
   int clip;
   int pr;
   int adjust;
   int cat;
   float cat1;
   float cat2;
   float cat3;
} PartyControl_t;

extern PartyControl_t partyControl;

extern void PartyInit(CLI_PARSE_INFO *pInfo);

#endif /* __PARTYCONTROL_H__ */
