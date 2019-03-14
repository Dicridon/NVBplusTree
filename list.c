#include "list.h"
#include "en_debug.h"
#include "debug.h"

static int list_constr(PMEMobjpool *pop, void* ptr, void* arg){return 0;}

static int
list_node_constr(PMEMobjpool *pop, void *ptr, void *arg)
{

    DEBUG_ENT();
    PMEMoid prev = ((struct list_node *)arg)->prev;
    PMEMoid next = ((struct list_node *)arg)->next;
    
    struct list_node *node_ptr = ptr;    
    
    node_ptr->prev = prev;
    node_ptr->next = next;
    
    pmemobj_persist(pop, node_ptr, sizeof(TOID(struct list_node)));
    DEBUG_LEA();
    return 0;
}

static int
list_new_node(PMEMobjpool *pop, TOID(struct list_node) *node)
{
     DEBUG_ENT();
     struct list_node arg = {
         .prev = {0, 0},
         .next = {0, 0}
     };
     DEBUG_MESG("node %p\n", node);
     int rev = POBJ_NEW(pop, node, struct list_node, list_node_constr,&arg);
     DEBUG_LEA();
     return rev;
    return 1;
}

// the address of root object may vary due to the change of size
// be cautious when storing a pointer pointing to this root in other objects
int
list_new(PMEMobjpool *pop, TOID(struct list) *list)
{
    
    DEBUG_ENT();
    // POBJ_ROOT will allocate space or retrive exsiting root
    // *list = POBJ_ROOT(pop, struct list);
    POBJ_NEW(pop, list, struct list, list_constr, NULL);
    struct list *list_ptr = D_RW(*list);
    list_new_node(pop, &list_ptr->head);
    struct list_node *head_ptr = D_RW(list_ptr->head);
    head_ptr->prev = list_ptr->head.oid;
    head_ptr->next = list_ptr->head.oid;
    pmemobj_persist(pop, list_ptr, sizeof(struct list));
    DEBUG_LEA();
    return 1;
}

void
list_destroy(TOID(struct list) *list)
{
    POBJ_FREE(D_RW(D_RW(*list)->head));
    POBJ_FREE(list);
}
