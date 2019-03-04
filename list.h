#ifndef __FROST_LIST__
#define __FROST_LIST__
#include <stdlib.h>
#include <libpmemobj.h>

POBJ_LAYOUT_BEGIN(dlist);
POBJ_LAYOUT_ROOT(dlist, struct list);
POBJ_LAYOUT_TOID(dlist, struct list_node);
POBJ_LAYOUT_END(dlist);

typedef struct list_node {
    TOID(struct list_node) prev;
    TOID(struct list_node) next;
} list_node_t;

typedef struct list {
    TOID(struct list_node) head;
} list_t;


// I should try to hide those ugly interfaces in PMDk

// void *ptr should actually be TOID(struct list_node *) type
int
list_node_constr(PMEMobjpool *pop, void *ptr, void *arg);


int
new_list(PMEMobjpool *pop, TOID(struct list) *list);

void
list_add_to_head(PMEMobjpool *pop,
                 TOID(struct list) *list, TOID(struct list_node) *node);

void
list_add(PMEMobjpool *pop,
         TOID(struct list_node) *prev, TOID(struct list_node) *node) ;

void
list_remove(PMEMobjpool *pop, TOID(struct list_node) *node);

void
list_destroy(TOID(struct list) *l);

void
free_list_node(TOID(struct list_node) *node);
#endif
