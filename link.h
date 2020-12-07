#ifndef __LINK_H__
#define __LINK_H__

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif

#ifndef SUCCESS
#define SUCCESS         0
#endif

#ifndef FAIL
#define FAIL            1
#endif

typedef struct DummyType_s {
    uint32_t dummy;
} DummyType_t;

typedef struct Link_s
{
    bool taken;
    DummyType_t *currentObject;
    struct Link_s *prev;
    struct Link_s *next;
} Link_t;

Link_t *LinkCreate(void *object);
void LinkDelete(Link_t *thiss);

#ifdef __cplusplus
}
#endif

#endif /* __LINK_H__ */
