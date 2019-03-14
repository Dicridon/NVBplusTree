#ifndef __FROST_LIST__
#define __FROST_LIST__
#include <stdlib.h>
#include <libpmemobj.h>

POBJ_LAYOUT_BEGIN(dlist);
POBJ_LAYOUT_ROOT(dlist, struct list);
POBJ_LAYOUT_TOID(dlist, struct list_node);
POBJ_LAYOUT_END(dlist);

typedef struct list_node {
    PMEMoid prev;
    PMEMoid next;
} list_node_t;

typedef struct list {
    TOID(struct list_node) head;
} list_t;

int
list_new(PMEMobjpool *pop, TOID(struct list) *list);

void
list_destroy(TOID(struct list) *l);
#endif
