#include "stdio.h"

#include "link.h"
#include "linklist.h"

uint32_t
LinkListEmpty(LinkList_t *list)
{
    Link_t *start ;

    start =  (Link_t *)list->head ;
    if (start == (Link_t *)NULL)
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

uint32_t
LinkHeadAdd(LinkList_t *list, Link_t *link)
{
    Link_t *start ;

    start =  (Link_t *)list->head ;
    if (start == NULL)
    {
        if (link->next != NULL)
        {
            return FALSE ; /* got an error, no NULL link for end*/
        }
        else
        {
            list->head = link ;
            list->tail = link ;
            return TRUE ;
        }

    }
    else
    {
        link->next = start ;
        start->prev = link ;
        list->head = link ;
        return TRUE ;
    }
}

uint32_t
LinkTailAdd(LinkList_t *list, Link_t *link)
{
    Link_t *tail ;

    tail =  (Link_t *)list->tail ;
    if (tail == NULL)
    {
        if (link->next != NULL)
        {
            printf("ERRROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            return FALSE ; /* got an error, no NULL link for end*/
        }
        else
        {
            list->tail = link ;
            list->head = link ;
            return TRUE ;
        }

    }
    else
    {
        /*  First link in list */
        if (list->tail->prev == NULL)
        {
            link->next = tail ;
            tail->prev = link ;
            list->head = link;
        }
        else /* Insert link at end of list */
        {
            link->next = list->tail ;		
            link->prev = list->tail->prev ;
            list->tail->prev->next = link ;
            list->tail->prev = link ;
			
        }
        return TRUE ;
    }
}


/* This routine removes a link. The linked list is searched from start to
   the end. When the link remove is found, it is removed from the list. */

uint32_t
LinkRemove(LinkList_t *list, Link_t *link)
{
    Link_t *start ;

    start = (Link_t *)list->head ;
    if ((NULL == start) || (NULL == link))
    {
        return FALSE ;
    }
    else if (start == link) /* first list entry */
    {
        if (NULL == start->next) /* list constitutes only one entry */
        {
              list->head = list->tail = NULL ;
        }
        else
        {
              start->next->prev = NULL ;
              list->head = start->next ;
        }
    }
    else
    {
        while (start != link)        // PR 191216   CID 25837
        {
            if (NULL == start->next) /* link not in list */
            {
                return FALSE ;
            }
            else
            {
                start = start->next ;
            }
        }
        start->prev->next = start->next ;
        if (NULL == start->next)  /* being the last node to be deleted */
        {
             list->tail = start->prev ;
        }
        else
        {
             start->next->prev = start->prev ;      // PR 191216   CID 25837
        }
    }

    /* destroy the old link */
    LinkDelete(link);

    return TRUE ;
}

void LinkListWalk(LinkList_t *list)
{
#if 0
    Link_t *start ;

    start = (Link_t *)list->head ;

    while(start->next != NULL)
    {
        (*start->current_object->exercise)(start->current_object) ;
        start = start->next ;
    }
#endif
}

LinkList_t
*LinkListCreate(void)
{
    LinkList_t *thiss ;

    thiss = (LinkList_t *)malloc(sizeof(LinkList_t));
    thiss->head = NULL ; /*start with empty list */
    thiss->tail = NULL ; /*start with empty list */
    return thiss ;
}

void
LinkListDelete(LinkList_t  *thiss)
{
    free(thiss);
}

uint32_t
LinkAfter(LinkList_t *list, Link_t *linkCurrent, Link_t *linkNew)
{
    if (linkCurrent == list->tail)
    {
        LinkTailAdd(list,linkNew);
    }
    else
    {
        linkNew->next = linkCurrent->next;
        linkCurrent->next = linkNew;
        linkNew->prev = linkCurrent;
        // +++owen BEEN BROKEN FOREVER!!!
#if 0
        linkCurrent->next->prev = linkNew;
#else
        linkNew->next->prev = linkNew;
#endif        
    }

    return 0;
}

uint32_t
LinkBefore(LinkList_t *list, Link_t *linkCurrent, Link_t *linkNew)
{
    if (linkCurrent == list->head)
    {
        LinkHeadAdd(list, linkNew);
    }
    else
    {
        linkNew->prev = linkCurrent->prev;
        linkCurrent->prev->next = linkNew;
        linkNew->next = linkCurrent;
        linkCurrent->prev = linkNew;
    }

    return 0;
}
