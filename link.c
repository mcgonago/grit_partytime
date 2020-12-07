#include "link.h"

Link_t
*LinkCreate(void *object)
{
	Link_t *thiss ;

	thiss = (Link_t *)malloc(sizeof(Link_t));
	thiss->currentObject = (DummyType_t *)object ;
	thiss->prev = NULL ;
	thiss->next = NULL ;
	return thiss ;
}

void
LinkDelete(Link_t *thiss)
{
    free(thiss);
}
