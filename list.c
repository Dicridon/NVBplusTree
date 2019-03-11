#include "list.h"
#include "en_debug.h"
#include "debug.h"

static int list_constr(PMEMobjpool *pop, void* ptr, void* arg){return 0;}

int
list_node_constr(PMEMobjpool *pop, void *ptr, void *arg)
{

    DEBUG_ENT();
    TOID(struct list_node) *prev = &((struct list_node *)arg)->prev;
    TOID(struct list_node) *next = &((struct list_node *)arg)->next;
    
    struct list_node *node_ptr = ptr;    
    
    node_ptr->prev = *prev;
    node_ptr->next = *next;

    pmemobj_persist(pop, node_ptr, sizeof(TOID(struct list_node)));
    DEBUG_LEA();
    return 0;
}

int
list_new_node(PMEMobjpool *pop, TOID(struct list_node) *node)
{
    DEBUG_ENT();
    struct list_node arg = {
        .prev = TOID_NULL(struct list_node),
        .next = TOID_NULL(struct list_node)
    };
    int rev = POBJ_NEW(pop, node, struct list_node, list_node_constr,&arg);
    DEBUG_LEA();
    return rev;
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
    head_ptr->prev = list_ptr->head;
    head_ptr->next = list_ptr->head;
    pmemobj_persist(pop, list_ptr, sizeof(struct list));
    DEBUG_LEA();
    return 1;
}

void
list_add_to_head(PMEMobjpool *pop,
                 TOID(struct list) *list, TOID(struct list_node) *node)
{
    DEBUG_ENT();
    list_t *list_ptr = D_RW(*list);
    
    TOID(struct list_node) *head = &list_ptr->head;
    struct list_node *head_ptr = D_RW(*head);

    TOID(struct list_node) *next = &head_ptr->next;
    
    struct list_node *node_ptr = D_RW(*node);

    node_ptr->next = *next;
    node_ptr->prev = *head;
    *next = *node;
    pmemobj_persist(pop, head_ptr, sizeof(struct list_node));
    pmemobj_persist(pop, node_ptr, sizeof(struct list_node));
    DEBUG_LEA(); 
}

void
list_add(PMEMobjpool *pop,
         TOID(struct list_node) *prev, TOID(struct list_node) *node)
{
    list_node_t *prev_ptr = D_RW(*prev);
    
    TOID(struct list_node) *next = &prev_ptr->next;
    struct list_node *next_ptr = D_RW(*next);
    struct list_node *node_ptr = D_RW(*node);

    node_ptr->prev = *prev;
    node_ptr->next = *next;
    prev_ptr->next = *node;
    next_ptr->prev = *node;
    pmemobj_persist(pop, node_ptr, sizeof(struct list_node));
    pmemobj_persist(pop, prev_ptr, sizeof(struct list_node));
    pmemobj_persist(pop, next_ptr, sizeof(struct list_node));
}

void
list_remove(PMEMobjpool *pop, TOID(struct list_node) *node)
{
    list_node_t *node_ptr = D_RW(*node);

    TOID(struct list_node) *prev = &node_ptr->prev;
    list_node_t *prev_ptr = D_RW(*prev);
    
    TOID(struct list_node) *next = &node_ptr->next;
    list_node_t *next_ptr = D_RW(*next);

    prev_ptr->next = *next;
    next_ptr->prev = *prev;
    POBJ_FREE(node);
    pmemobj_persist(pop, prev_ptr, sizeof(TOID(struct list_node)));
    pmemobj_persist(pop, next_ptr, sizeof(TOID(struct list_node)));
}

void
list_destroy(TOID(struct list) *list)
{
    list_t *list_ptr = D_RW(*list);

    TOID(struct list_node) *head = &list_ptr->head;
    list_node_t *head_ptr = D_RW(*head);
    TOID(struct list_node) *walk = &head_ptr->next;
    list_node_t *walk_ptr = D_RW(*walk);

    while (walk != head) {
        head_ptr->next = walk_ptr->next;
        POBJ_FREE(walk);
        walk = &head_ptr->next;
        walk_ptr = D_RW(*walk);
    }
    POBJ_FREE(list);
}

void
free_list_node(TOID(struct list_node) *node)
{
    POBJ_FREE(node);
}
