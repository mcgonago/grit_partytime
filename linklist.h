#ifndef __LINKLIST_H__
#define __LINKLIST_H__

#include "link.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LinkList_s
{
  bool taken;
  Link_t *head ;
  Link_t *tail ;
} LinkList_t ;

LinkList_t *LinkListCreate(void);
uint32_t LinkHeadAdd(LinkList_t *list, Link_t *link);
uint32_t LinkTailAdd(LinkList_t *list, Link_t *link);
uint32_t LinkRemove(LinkList_t *list, Link_t *link);
uint32_t LinkAfter(LinkList_t *list, Link_t *linkCurrent, Link_t *linkNew);
uint32_t LinkBefore(LinkList_t *list, Link_t *linkCurrent, Link_t *linkNew);
void LinkListDelete(LinkList_t *list);
uint32_t LinkListEmpty(LinkList_t *list);

#ifdef __cplusplus
}
#endif

#endif  /* __LINKLIST_H__ */
