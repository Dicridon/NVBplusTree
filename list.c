#include "list.h"


int
list_node_constr(PMEMobjpool *pop, void *ptr, void *arg)
{
    TOID(struct list_node) *node = ptr;
    TOID(struct list_node) *prev = &((struct list_node *)arg)->prev;
    TOID(struct list_node) *next = &((struct list_node *)arg)->next;
    
    list_node_t *node_ptr = D_RW(*node);
    list_node_t *prev_ptr = D_RW(*prev);
    list_node_t *next_ptr = D_RW(*next);
    
    node_ptr->prev = *prev;
    node_ptr->next = *next;
    prev_ptr->next = *node;
    next_ptr->prev = *node;

    pmemobj_persist(pop, node, sizeof(TOID(struct list_node)));
    pmemobj_persist(pop, prev, sizeof(TOID(struct list_node)));
    pmemobj_persist(pop, next, sizeof(TOID(struct list_node)));
    return 0;
}

int list_new_node(PMEMobjpool *pop, TOID(struct list_node) *node) {
    struct list_node arg = {
        .prev = *node,
        .next = *node
    };
    return POBJ_NEW(pop, node, struct list_node, list_node_constr,&arg);
}

static int
list_head_constr(PMEMobjpool *pop, void *ptr, void *arg)
{
    // just to silence warnings
    void *temp = arg;
    arg = temp;

    
    TOID(struct list_node) *head = ptr;
    list_node_t *head_ptr = D_RW(*head);
    head_ptr->next = *head;
    head_ptr->prev = *head;
    pmemobj_persist(pop, head, sizeof(TOID(struct list_node)));
    return 0;
}


// the address of root object may vary due to the change of size
// be cautious when storing a pointer pointing to this root in other objects
int
list_new(PMEMobjpool *pop, TOID(struct list) *list)
{
    // POBJ_ROOT will allocate space or retrive exsiting root
    *list = POBJ_ROOT(pop, struct list);
    list_t *list_ptr = D_RW(*list);
    list_head_constr(pop, &list_ptr->head, NULL);
    // no need to invode persist on list
    return 1;
}

void
list_add_to_head(PMEMobjpool *pop,
                 TOID(struct list) *list, TOID(struct list_node) *node)
{
    list_t *list_ptr = D_RW(*list);
    
    TOID(struct list_node) *head = &list_ptr->head;
    list_node_t *head_ptr = D_RW(*head);

    TOID(struct list_node) *next = &head_ptr->next;

    list_node_t arg = {
        .prev = *head,
        .next = *next
    };

    POBJ_ALLOC(pop, node,
               struct list_node, sizeof(list_node_t), list_node_constr, &arg);
}

void
list_add(PMEMobjpool *pop,
         TOID(struct list_node) *prev, TOID(struct list_node) *node) {
    list_node_t *prev_ptr = D_RW(*prev);
    
    TOID(struct list_node) *next = &prev_ptr->next;
    
    list_node_t arg = {
        .prev = *prev,
        .next = *next
    };

    POBJ_ALLOC(pop, node,
               struct list_node, sizeof(list_node_t), list_node_constr, &arg);
}

void
list_remove(PMEMobjpool *pop, TOID(struct list_node) *node) {
    list_node_t *node_ptr = D_RW(*node);

    TOID(struct list_node) *prev = &node_ptr->prev;
    list_node_t *prev_ptr = D_RW(*prev);
    
    TOID(struct list_node) *next = &node_ptr->next;
    list_node_t *next_ptr = D_RW(*next);

    prev_ptr->next = *next;
    next_ptr->prev = *prev;
    POBJ_FREE(node);
    pmemobj_persist(pop, prev, sizeof(TOID(struct list_node)));
    pmemobj_persist(pop, next, sizeof(TOID(struct list_node)));
}

void
list_destroy(TOID(struct list) *list) {
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
free_list_node(TOID(struct list_node) *node) {
    POBJ_FREE(node);
}
