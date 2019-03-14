 /*
  !!!!!!!!!!!!!!
  !!!! NOTE: I ALWAYS ASSUME THAT MALLOC/REALLOC/CALLOC WOULD NOT FAIL!!!!!!
  !!!!       THIS IS JUST FOR TEST
  !!!!!!!!!!!!!!
*/

#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "list.h"
#include "queue.h"
#include "bplustree_dev.h"
#include "en_debug.h"
#include "debug.h"


// insertion
static int
bpt_new_leaf(PMEMobjpool *pop, TOID(struct bpt_node) *node);

static int
bpt_new_non_leaf(PMEMobjpool *pop, TOID(struct bpt_node) *node);

static int
bpt_complex_insert(PMEMobjpool *pop, TOID(struct bpt) *t,
                   TOID(struct bpt_node) *leaf, const char *key,
                   const char *value);
static int
bpt_simple_insert(PMEMobjpool *pop,
                  TOID(struct bpt_node) *leaf,
                  const char *key, const char *value);

static void
bpt_insert_child(PMEMobjpool *pop,
                 TOID(struct bpt_node) *old, TOID(struct bpt_node) *new);

static int
bpt_insert_adjust(PMEMobjpool *pop, TOID(struct bpt) *t,
                  TOID(struct bpt_node) *old, TOID(struct bpt_node) *new);
static TOID(struct bpt_node) *
find_leaf(TOID(struct bpt) *t, const char *key);

static TOID(struct bpt_node) *
find_non_leaf(TOID(struct bpt) *t, const char *key);

// deletion
static bool
bpt_is_root(const TOID(struct bpt_node) *t);

static int
bpt_insert_key(PMEMobjpool *pop,
               TOID(struct bpt_node) *t, const TOID(struct string) *key);

static const TOID(struct bpt_node) *
bpt_check_redistribute(const TOID(struct bpt_node) *t);

static int
redistribute_leaf(PMEMobjpool *pop, TOID(struct bpt) *t,
                  TOID(struct bpt_node) *leaf, const char *key);

static int
bpt_simple_delete(PMEMobjpool *pop, TOID(struct bpt) *t,
                  TOID(struct bpt_node) *leaf, const char *key);

static int
redistribute_internal(PMEMobjpool *pop, const TOID(struct string) *split_key,
                      TOID(struct bpt_node) *parent, TOID(struct bpt_node) *left,
                      TOID(struct bpt_node) *right);

static TOID(struct bpt_node) *
merge_leaves(PMEMobjpool *pop,
             TOID(struct bpt) *t, TOID(struct bpt_node) *leaf, const char *key);
static int
merge_internal(PMEMobjpool *pop, TOID(struct bpt) *t,
               TOID(struct bpt_node) *parent,
               const TOID(struct string) *split_key);

static int
merge(PMEMobjpool *pop, TOID(struct bpt) *t,
      TOID(struct bpt_node) *parent, const char *key,
      const TOID(struct string) *split_key);

static int
bpt_remove_key_and_data(PMEMobjpool *pop,
                        TOID(struct bpt_node) *node, const char *key);

static int
bpt_complex_delete(PMEMobjpool *pop, TOID(struct bpt) *t,
                   TOID(struct bpt_node) *leaf, const char *key);

static int
bpt_complex_delete(PMEMobjpool *pop, TOID(struct bpt) *t,
                   TOID(struct bpt_node) *leaf, const char *key);

static void
bpt_free_leaf(TOID(struct bpt_node) *leaf);

static void
bpt_free_non_leaf(TOID(struct bpt_node) *nleaf);


int
bpt_new(PMEMobjpool *pop, TOID(struct bpt) *t);

int
bpt_retrieve(PMEMobjpool *pop, TOID(struct bpt) *t);

int
bpt_insert(PMEMobjpool *pop,
           TOID(struct bpt) *t, const char *key, const char *value);

int
bpt_delete(PMEMobjpool *pop, TOID(struct bpt) *t, const char *key);

int
bpt_get(TOID(struct bpt) *t, const char *key, char *buffer);

int
bpt_range(TOID(struct bpt) *t, const char *start, const char *end, char **buffer);

int
bpt_destroy(TOID(struct bpt) *t);

// int bpt_serialize_f(bpt_t *t, char *file_name);
// 
// int bpt_serialize_fp(bpt_t *t, FILE *fp);

void
bpt_print(const TOID(struct bpt) *t);

void
bpt_print_leaves(const TOID(struct bpt) *t);

static int
bpt_node_constr(PMEMobjpool *pop, void *ptr, void *arg)
{
    if (!pop)
        return -1;  // just silence the warnings
    DEBUG_ENT();
    if (arg) {
        DEBUG_MESG("non empty args\n");
        DEBUG_LEA();
        return -1;
    }

    struct bpt_node *node_ptr = ptr;
    DEBUG_MESG("node %p\n", node_ptr);
    /*
      This is really wierd
      If I uncomment function call below, my code would trigger seg fault
      so in order to initialized a bpt_node structure, I have to initialize the 
      field link outsize this function, or the bpt_node structure will be given 
      a zero pool_id
     */
    // list_new_node(pop, &node_ptr->link);
    DEBUG_LEA();
    return 0;
}

static int
bpt_constr(PMEMobjpool *pop, void *ptr, void *arg)
{
    DEBUG_ENT();
    if (arg) {
        DEBUG_MESG("non empty args\n");
        DEBUG_LEA();
        return -1;
    }

    struct bpt *bpt_ptr = ptr;

    bpt_new_leaf(pop, &bpt_ptr->root);
    struct bpt_node *root_ptr = D_RW(bpt_ptr->root);
    
    list_new(pop, &bpt_ptr->list);
#ifdef __DEBUG__
    if (TOID_IS_NULL(bpt_ptr->list)) {
        printf("No memory in %s\n", __FUNCTION__);
        assert(0);
    }
#endif

    // list_add_to_head(pop, &bpt_ptr->list, &root_ptr->link);
    root_ptr->link.prev = D_RW(bpt_ptr->list)->head.oid;
    root_ptr->link.next = D_RW(D_RW(bpt_ptr->list)->head)->next;
    D_RW(D_RW(bpt_ptr->list)->head)->next = bpt_ptr->root.oid;
    
    bpt_ptr->free_key = NULL_STR;
    bpt_ptr->level = 1;

    pmemobj_persist(pop, bpt_ptr, sizeof(struct bpt));
    DEBUG_LEA();
    return 0;
}

int
bpt_new (PMEMobjpool *pop, TOID(struct bpt) *t) {
    DEBUG_ENT();
    *t = POBJ_ROOT(pop, struct bpt);
    DEBUG_MESG("After POBJ_ROOT: \n"
               "address: %p, pool_uuid_lo: %ld, offer: %ld\n",
               D_RO(*t), t->oid.pool_uuid_lo, t->oid.off);
    struct bpt *t_ptr = D_RW(*t);
    bpt_constr(pop, t_ptr, NULL);
    DEBUG_MESG("After bpt_constr: \n"
               "address: %p, pool_uuid_lo: %ld, offer: %ld\n",
               D_RO(*t), t->oid.pool_uuid_lo, t->oid.off);
    DEBUG_LEA();
    return 1;
}

int
bpt_retrieve(PMEMobjpool *pop, TOID(struct bpt) *t)
{
    *t = POBJ_ROOT(pop, struct bpt);
    return 1;
}

static int
bpt_new_leaf(PMEMobjpool *pop, TOID(struct bpt_node) *node)
{
    DEBUG_ENT();
    DEBUG_MESG("node %p pool %ld, off %ld\n",
               node, node->oid.pool_uuid_lo, node->oid.off);
    POBJ_NEW(pop, node, struct bpt_node, bpt_node_constr, NULL);
#ifdef __DEBUG__
    if (TOID_IS_NULL(*node)) {
        printf("No memory in %s\n", __FUNCTION__);
        assert(0);
    }
#endif
        
    DEBUG_MESG("node %p pool %ld, off %ld\n",
               node, node->oid.pool_uuid_lo, node->oid.off);
    struct bpt_node *node_ptr = D_RW(*node);
    DEBUG_MESG("node %p pool %ld, off %ld\n",
               node, node->oid.pool_uuid_lo, node->oid.off);

    // filed link is not initialized here, because interfaces in list.c
    // will handle the initialization
    node_ptr->num_of_children = 0;
    node_ptr->num_of_keys = 0;
    node_ptr->parent = NULL_BPT_NODE;
    node_ptr->type = LEAF;
    // list_new_node(pop, &node_ptr->link);
    // D_RW(node_ptr->link)->prev = node_ptr->link;
    // D_RW(node_ptr->link)->next = node_ptr->link;
    node_ptr->link.prev = node->oid;
    node_ptr->link.next = node->oid;
    
    for (int i = 0; i < DEGREE; i++) {
        node_ptr->keys[i] = NULL_STR;
        node_ptr->data[i] = NULL_STR;        
    }
    pmemobj_persist(pop, node_ptr, sizeof(struct bpt_node));
    DEBUG_LEA();
    return 1;
}

static int
bpt_new_non_leaf(PMEMobjpool *pop, TOID(struct bpt_node) *node) {
    POBJ_NEW(pop, node, struct bpt_node, bpt_node_constr, NULL);
#ifdef __DEBUG__
    if (TOID_IS_NULL(*node)) {
        printf("No memory in %s\n", __FUNCTION__);
        assert(0);
    }
#endif
    struct bpt_node *node_ptr = D_RW(*node);

    // filed link is not initialized here, because interfaces in list.c
    // will handle the initialization
    node_ptr->num_of_children = 0;
    node_ptr->num_of_keys = 0;
    node_ptr->parent = NULL_BPT_NODE;
    node_ptr->type = NON_LEAF;
    // list_new_node(pop, &node_ptr->link);
    // D_RW(node_ptr->link)->prev = node_ptr->link;
    // D_RW(node_ptr->link)->next = node_ptr->link;
    node_ptr->link.prev = node->oid;
    node_ptr->link.next = node->oid;
    
    for (int i = 0; i < DEGREE; i++) {
        node_ptr->keys[i] = NULL_STR;
        node_ptr->children[i] = NULL_BPT_NODE;
    }
    node_ptr->children[DEGREE] = NULL_BPT_NODE;
    pmemobj_persist(pop, node_ptr, sizeof(struct bpt_node));
    return 1;
}

// outter function has determined that bpt_complex_insert should be called
// we have to split the leaf and insert new node into parent node
// adjustment to parent is necessary
static int
bpt_simple_insert(PMEMobjpool *pop, TOID(struct bpt_node) *leaf,
                  const char *key, const char *value)
{
    DEBUG_ENT();
    struct bpt_node *leaf_ptr = D_RW(*leaf);

    DEBUG_MESG("inserting target: %ld, %ld",
               leaf->oid.pool_uuid_lo, leaf->oid.off);
    
    unsigned long long i;
    for (i = 0; i < leaf_ptr->num_of_keys; i++) {
        // if the tree is empty, we may encounter a NULL key
        if (TOID_IS_NULL(leaf_ptr->keys[i]) ||
            strcmp(str_get(&leaf_ptr->keys[i]), key) >= 0)
            break;
    }

    for (unsigned long long j = leaf_ptr->num_of_keys; j > i; j--) {
        leaf_ptr->keys[j] = leaf_ptr->keys[j - 1];
        leaf_ptr->data[j] = leaf_ptr->data[j - 1];
    }
    str_new(pop, &leaf_ptr->keys[i], key);
#ifdef __DEBUG__
    if (TOID_IS_NULL((leaf_ptr->keys[i]))) {
        printf("No memory in %s\n", __FUNCTION__);
        assert(0);
    }
#endif
    
    str_new(pop, &leaf_ptr->data[i], value);

#ifdef __DEBUG__
    if (TOID_IS_NULL((leaf_ptr->keys[i]))) {
        printf("No memory in %s\n", __FUNCTION__);
        assert(0);
    }
#endif
    leaf_ptr->num_of_keys++;
    pmemobj_persist(pop, leaf_ptr, sizeof(struct bpt_node));
    DEBUG_LEA();
    return 1;
}

static void
bpt_insert_child(PMEMobjpool *pop,
                 TOID(struct bpt_node) *old, TOID(struct bpt_node) *new)
{
    DEBUG_ENT();
    struct bpt_node *old_ptr = D_RW(*old);
    struct bpt_node *new_ptr = D_RW(*new);
    struct bpt_node *parent_ptr = D_RW(old_ptr->parent);
    // insert
    unsigned long long i = 0;
    unsigned long long j = 0;
    // new inherits data from old, so we just need to insert new after old
    for (i = 0; i < parent_ptr->num_of_children; i++) {
        if (TOID_EQUALS(parent_ptr->children[i], *old))
            break;
    }

    for (j = parent_ptr->num_of_keys; j > i; j--) {
        parent_ptr->keys[j] = parent_ptr->keys[j-1];
    }
    parent_ptr->keys[i] = new_ptr->keys[0];
    parent_ptr->num_of_keys++;

    i++; // insert after old
    for (j = parent_ptr->num_of_children; j > i; j--) {
        parent_ptr->children[j] = parent_ptr->children[j-1];
    }
    parent_ptr->children[i] = *new;
    parent_ptr->num_of_children++;
    new_ptr->parent = old_ptr->parent;
    pmemobj_persist(pop , old_ptr, sizeof(struct bpt_node));
    pmemobj_persist(pop , new_ptr, sizeof(struct bpt_node));
    DEBUG_LEA();
 }

static int
bpt_insert_adjust(PMEMobjpool *pop, TOID(struct bpt) *t,
                  TOID(struct bpt_node) *old, TOID(struct bpt_node) *new)
{
    DEBUG_ENT();
    struct bpt *t_ptr = D_RW(*t);
    struct bpt_node *old_ptr = D_RW(*old);
    struct bpt_node *new_ptr = D_RW(*new);
    TOID(struct bpt_node) parent = old_ptr->parent;
    struct bpt_node *parent_ptr = D_RW(old_ptr->parent);
    
    if (t_ptr->level == 1) {
        // bpt_node_t *new_parent = bpt_new_non_leaf();
        TOID(struct bpt_node) new_parent;
        bpt_new_non_leaf(pop, &new_parent);
        struct bpt_node *new_parent_ptr = D_RW(new_parent);
        
        new_parent_ptr->num_of_children = 2;
        new_parent_ptr->num_of_keys = 1;
        new_parent_ptr->keys[0] = new_ptr->keys[0];
        new_parent_ptr->children[0] = *old;
        new_parent_ptr->children[1] = *new;
        t_ptr->root = new_parent;
        old_ptr->parent = new_parent;
        new_ptr->parent = new_parent;

        t_ptr->level++;
        pmemobj_persist(pop, t_ptr, sizeof(struct bpt));
        pmemobj_persist(pop, new_parent_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, new_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, old_ptr, sizeof(struct bpt_node));
        DEBUG_LEA();
        return 1;        
    }
    
    if (TOID_IS_NULL(old_ptr->parent)) {
        // this is internal node split
        // we may not just create a new root and link old and new to it
        // we must pick out the smallest child in new and use this child
        // to construct a new root, or we may not ensure the relationsip
        // that num_of_childen = num_of_keys + 1

        TOID(struct bpt_node) new_parent;
        bpt_new_non_leaf(pop, &new_parent);
        struct bpt_node *new_parent_ptr = D_RW(new_parent);
        new_parent_ptr->num_of_children = 2;
        new_parent_ptr->num_of_keys = 1;
        new_parent_ptr->keys[0] = new_ptr->keys[0];
        new_parent_ptr->children[0] = *old;
        new_parent_ptr->children[1] = *new;
        t_ptr->root = new_parent;
        t_ptr->level++;

        // delete the key copied to root in new
        for (unsigned long long i = 1; i < new_ptr->num_of_keys; i++) {
            new_ptr->keys[i-1] = new_ptr->keys[i];
        }
        new_ptr->keys[new_ptr->num_of_keys - 1] = NULL_STR;
        new_ptr->num_of_keys--;

        old_ptr->parent = new_parent;
        new_ptr->parent = new_parent;
        pmemobj_persist(pop, new_parent_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, new_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, old_ptr, sizeof(struct bpt_node));
        // num_of_children will not be modified
        // we modify this field in else if
        
    } else if (parent_ptr->num_of_keys == DEGREE - 1) {
        // insert
        // new will be added to parent of old
        bpt_insert_child(pop, old, new); 
        if (new_ptr->type == NON_LEAF) {
            // delete the key copied to parent in new
            for (unsigned long long i = 1; i < new_ptr->num_of_keys; i++) {
                new_ptr->keys[i-1] = new_ptr->keys[i];
            }
            new_ptr->keys[new_ptr->num_of_keys - 1] = NULL_STR;
            new_ptr->num_of_keys--;
        }
        
        // split
        // [split] is NOT left to new parent
        TOID(struct bpt_node) new_parent;
        bpt_new_non_leaf(pop, &new_parent);
        struct bpt_node *new_parent_ptr = D_RW(new_parent);
        
        unsigned long long split = parent_ptr->num_of_keys / 2;
        unsigned long long i = 0;
        for (i = 0; i + split < parent_ptr->num_of_keys; i++) {
            new_parent_ptr->keys[i] = parent_ptr->keys[i + split];
            parent_ptr->keys[i + split] = NULL_STR;
            new_parent_ptr->children[i] = parent_ptr->children[i + split + 1];
            struct bpt_node *childre_ptr = D_RW(new_parent_ptr->children[i]);
            // D_RW(new_parent_ptr->children[i])->parent = new_parent;
            childre_ptr->parent = new_parent;
            parent_ptr->children[i + split + 1] = NULL_BPT_NODE;
        }

        new_parent_ptr->num_of_keys = parent_ptr->num_of_keys - split;
        new_parent_ptr->num_of_children = new_parent_ptr->num_of_keys;
        parent_ptr->num_of_keys -= parent_ptr->num_of_keys - split;
        parent_ptr->num_of_children = parent_ptr->num_of_keys + 1;

        
        // recursive handling
        bpt_insert_adjust(pop, t, &parent, &new_parent);
    } else {
        bpt_insert_child(pop, old, new);
        if (new_ptr->type == NON_LEAF) {
            // delete the key copied to parent in new
            for (unsigned long long i = 1; i < new_ptr->num_of_keys; i++) {
                new_ptr->keys[i-1] = new_ptr->keys[i];
            }
            new_ptr->keys[new_ptr->num_of_keys - 1] = NULL_STR;
            new_ptr->num_of_keys--;
        }
    }
    DEBUG_LEA();
    return 1;
}


// outter function has determined to call this function
// so leaf->num_of_keys = DEGREE - 1
static int
bpt_complex_insert(PMEMobjpool *pop,
                   TOID(struct bpt) *t, TOID(struct bpt_node) *leaf,
                   const char *key, const char *value)
{
    DEBUG_ENT();
    TOID(struct bpt_node) new_leaf;
    bpt_new_leaf(pop, &new_leaf);


    struct bpt *t_ptr = D_RW(*t);
    struct bpt_node* leaf_ptr = D_RW(*leaf);
    struct bpt_node *new_leaf_ptr = D_RW(new_leaf);    
    // since we have make extra space
    // we may just insert the key and vlaue into old leaf and then split it
    // what good about this is that we do not have to consider where
    // new data should be inserted.
    unsigned long long i = 0;
    for (i = 0; i < leaf_ptr->num_of_keys; i++) {
        if(strcmp(str_get(&leaf_ptr->keys[i]), key) >= 0)
            break;
    }

    for (unsigned long long j = leaf_ptr->num_of_keys; j > i; j--) {
        leaf_ptr->keys[j] = leaf_ptr->keys[j - 1];
        leaf_ptr->data[j] = leaf_ptr->data[j - 1];
    }

    str_new(pop, &leaf_ptr->keys[i], key);
#ifdef __DEBUG__
    if (TOID_IS_NULL((leaf_ptr->keys[i]))) {
        printf("No memory in %s\n", __FUNCTION__);
        assert(0);
    }
#endif
    
    str_new(pop, &leaf_ptr->data[i], value);
    
#ifdef __DEBUG__
    if (TOID_IS_NULL((leaf_ptr->keys[i]))) {
        printf("No memory in %s\n", __FUNCTION__);
        assert(0);
    }
#endif

    leaf_ptr->num_of_keys++;
    
    // now split this leaf
    // copy the right half to new leaf (including [split])
    unsigned long long split = leaf_ptr->num_of_keys / 2;

    for (i = 0; i + split < leaf_ptr->num_of_keys; i++) {
        new_leaf_ptr->keys[i] = leaf_ptr->keys[i + split];
        new_leaf_ptr->data[i] = leaf_ptr->data[i + split];
        leaf_ptr->keys[i + split] = NULL_STR;
        leaf_ptr->data[i + split] = NULL_STR;
    }

    // we ensure that new leaf is always on the right of leaf
    new_leaf_ptr->num_of_keys = i;
    leaf_ptr->num_of_keys -= leaf_ptr->num_of_keys - split;
    
    // list_add(pop, &leaf_ptr->link, &new_leaf_ptr->link);
    struct list_node *next = pmemobj_direct(leaf_ptr->link.next);
    new_leaf_ptr->link.prev = leaf->oid;
    new_leaf_ptr->link.next = leaf_ptr->link.next;
    leaf_ptr->link.next = new_leaf.oid;
    next->prev = new_leaf.oid;
    ((struct bpt_node*)
     pmemobj_direct(leaf_ptr->link.next))->link.prev = leaf->oid;
    bpt_insert_adjust(pop, t, leaf, &new_leaf);
    pmemobj_persist(pop, t_ptr, sizeof(struct bpt));
    DEBUG_LEA();
    return 1;
}

int
bpt_get(TOID(struct bpt) *t, const char *key, char *buffer)
{
    const struct bpt *t_ptr = D_RO(*t);
    const TOID(struct bpt_node) *walk = &t_ptr->root;

    const struct bpt_node *walk_ptr = D_RO(*walk);
    
    unsigned long long i = 0;
    if (walk_ptr == NULL)
        return -1;
    while(walk_ptr->type != LEAF) {
        for (i = 0; i < walk_ptr->num_of_keys; i++) {
            if (strcmp(str_get(&walk_ptr->keys[i]), key) > 0)
                break;
        }
        walk = &walk_ptr->children[i];
        walk_ptr = D_RO(*walk);
    }

    for (i = 0; i < walk_ptr->num_of_keys; i++) {
        if (strcmp(str_get(&walk_ptr->keys[i]), key) == 0) {
            if (buffer != NULL) {
                strcpy(buffer, str_get(&walk_ptr->data[i]));
            }
            return D_RO(walk_ptr->data[i])->len.l;
        }
    }
    return -1;
}

// find leaf will find a leaf suitable for this key
static TOID(struct bpt_node) *
find_leaf(TOID(struct bpt) *t, const char *key)
{
    DEBUG_ENT();
    struct bpt *t_ptr = D_RW(*t);
    TOID(struct bpt_node) *root = &t_ptr->root;
    struct bpt_node *root_ptr = D_RW(*root);

    TOID(struct bpt_node) *walk = root;
    struct bpt_node *walk_ptr = root_ptr;
    unsigned long long i = 0;
    while(walk_ptr->type != LEAF) {
        for (i = 0; i < walk_ptr->num_of_keys; i++) {
            if (strcmp(str_get(&walk_ptr->keys[i]), key) > 0)
                break;
        }
        walk = &walk_ptr->children[i];
        walk_ptr = D_RW(walk_ptr->children[i]);
    }
    DEBUG_LEA();
    return walk;
}

static TOID(struct bpt_node) *
find_non_leaf(TOID(struct bpt)*t, const char *key)
{
    DEBUG_ENT();
    struct bpt *t_ptr = D_RW(*t);
    TOID(struct bpt_node) *root = &t_ptr->root;
    struct bpt_node *root_ptr = D_RW(*root);

    TOID(struct bpt_node) *walk = root;
    struct bpt_node *walk_ptr = root_ptr;
    unsigned long long i = 0;
    while(walk_ptr->type != LEAF) {
        for (i = 0; i < walk_ptr->num_of_keys; i++) {
            if (strcmp(str_get(&walk_ptr->keys[i]), key) == 0) {
                return walk;                
            }

            if (strcmp(str_get(&walk_ptr->keys[i]), key) > 0)
                break;
        }
        walk = &walk_ptr->children[i];
        walk_ptr = D_RW(walk_ptr->children[i]);
    }
    DEBUG_ENT();
    return NULL;
}

int
bpt_range(TOID(struct bpt) *t,
          const char *start, const char *end, char **buffer)
{
    if (strcmp(start, end) > 0)
        return -1;

    const struct bpt *t_ptr = D_RO(*t);
    
    TOID(struct bpt_node) *leaf = find_leaf(t, start);
    unsigned long long i = 0, j = 0;

    const struct list_node *p = &D_RO(*leaf)->link;

    // TOID(struct bpt_node) *node;
    const struct bpt_node *node_ptr;
    while(p != D_RO(D_RO(t_ptr->list)->head)) {
        // notice that address of p is already the address of a leaf
        // so do not used D_RO again
        // node = (TOID(struct bpt_node) *)p;
        node_ptr = (struct bpt_node *)p;
        for (i = 0; i < node_ptr->num_of_keys; i++) {
            if (strcmp(str_get(&node_ptr->keys[i]), end) > 0)
                return 1;

            if (strcmp(str_get(&node_ptr->keys[i]), start) >= 0)
                strcpy(buffer[j++], str_get(&node_ptr->keys[i]));
        }
        p = pmemobj_direct(p->next);
        // p_ptr = D_RO(*p);
    }
    return -1;
}

int
bpt_destroy(TOID(struct bpt) *t)
{

    struct bpt *t_ptr = D_RW(*t);
    
    if (TOID_IS_NULL(t_ptr->root)) {
        printf("empty tree\n");
        return 1;
    }
    

    TOID(struct bpt_node) __walk__;
    TOID(struct bpt_node) *walk = &__walk__;


    TOID(struct bpt_node) *root = &t_ptr->root;
    queue_t *queue = new_queue();
    enqueue(queue, root);

    while(!queue_empty(queue)) {
        dequeue(queue, (void*)&walk);
        // walk is of TOID(struct bpt_node) * type
        struct bpt_node *walk_ptr = D_RW(*walk);
        for (unsigned long long i = 0;
             walk_ptr->type == NON_LEAF && i < walk_ptr->num_of_children; i++) {
            enqueue(queue, &walk_ptr->children[i]);
        }

        if (walk_ptr->type == LEAF) {
            for (unsigned long long i = 0; i < walk_ptr->num_of_keys; i++) { 
                str_free(&walk_ptr->keys[i]);
                str_free(&walk_ptr->data[i]);
            }
        }
        POBJ_FREE(walk);
    }
    queue_destroy(queue);
    return 1;
}



int
bpt_insert(PMEMobjpool *pop,
           TOID(struct bpt) *t, const char *key, const char *value)
{
    DEBUG_ENT();
    TOID(struct bpt_node) *old = find_leaf(t, key);
    struct bpt_node *old_ptr = D_RW(*old);

    for (unsigned long long i = 0; i < old_ptr->num_of_keys; i++) {
        if (strcmp(str_get(&old_ptr->keys[i]), key) == 0) {
            old_ptr = NULL;
            break;
        }
    }

    if (!old_ptr) {
        DEBUG_LEA();
        return 1;        
    }

    
    if (old_ptr->num_of_keys == DEGREE - 1) {
        // if you want to figure out what is actually going on
        // chekc out website below
        // https://www.cs.usfca.edu/~galles/visualization/BPlustree.html
        // this page indeed benefited me so much
        bpt_complex_insert(pop, t, old, key, value);
    } else {
        bpt_simple_insert(pop, old, key, value);
    }
    DEBUG_LEA();
    return 1;
}

// ensure leaf has no data at all before calling this function
static void
bpt_free_leaf(TOID(struct bpt_node) *leaf)
{
    POBJ_FREE(leaf);
}

static void
bpt_free_non_leaf(TOID(struct bpt_node) *nleaf) {
    POBJ_FREE(nleaf);
}

void
bpt_print(const TOID(struct bpt) * t)
{
    const struct bpt *t_ptr = D_RO(*t);
    if (TOID_IS_NULL(t_ptr->root)) {
        printf("empty tree\n");
        return;
    }
    const TOID(struct bpt_node) __walk__;
    const TOID(struct bpt_node) *walk = &__walk__;
    
    queue_t *queue = new_queue();
    enqueue(queue, &t_ptr->root);

    while(!queue_empty(queue)) {
        dequeue(queue, (void*)&walk);
        const struct bpt_node *walk_ptr = D_RO(*walk);
        for (unsigned long long i = 0;
             walk_ptr->type == NON_LEAF && i < walk_ptr->num_of_children; i++) {
            enqueue(queue, &walk_ptr->children[i]);
        }
        printf("%p(%ld,%ld) child of %p(%ld,%ld): \n",
               walk_ptr, 
               walk->oid.pool_uuid_lo & 0xff, walk->oid.off,
               D_RO(walk_ptr->parent),
               walk_ptr->parent.oid.pool_uuid_lo & 0xff, walk_ptr->parent.oid.off);
        if (walk_ptr->type == LEAF)
            printf("prev: (%ld, %ld), next: (%ld, %ld)\n",
                   walk_ptr->link.prev.pool_uuid_lo & 0xff,
                   walk_ptr->link.prev.off,
                   walk_ptr->link.next.pool_uuid_lo & 0xff,
                   walk_ptr->link.next.off);
        
        if (walk_ptr->type == NON_LEAF)
            printf("non leaf: %llu children, %llu keys\n",
                   walk_ptr->num_of_children, walk_ptr->num_of_keys);
        printf("keys: \n");
        for (unsigned long long i = 0; i < walk_ptr->num_of_keys; i++) {
            printf("%s(%ld), ",
                   str_get(&walk_ptr->keys[i]), walk_ptr->keys[i].oid.off);
        }
        printf("\n\n");
    }
    queue_destroy(queue);
}

void
bpt_print_leaves(const TOID(struct bpt) *t)
{
    const struct bpt *t_ptr = D_RO(*t);
    const struct list_node *p =
        pmemobj_direct(D_RO(D_RO(t_ptr->list)->head)->next);

    // const TOID(struct bpt_node) *node;
    const struct bpt_node *node_ptr;
    while(p != D_RO(D_RO(t_ptr->list)->head)) {
        // node = (TOID(struct bpt_node) *)p;
        node_ptr = (struct bpt_node *)p;
        for (unsigned long long i = 0; i < node_ptr->num_of_keys; i++) {
            printf("%s, ", str_get(&node_ptr->keys[i]));
        }
        // p = &D_RO(*p)->next;
        p = pmemobj_direct(p->next);
    }
    puts("");
}

// deletion

/*
  - How to delete
  1. if leaf->num_of_keys > DEGREE / 2, just delete the key
  2. if leaf->num_of_keys = DEGREE / 2, try to borrow a key from sibling
  3. if siblings can not offer a key, merge this leaf and the sibling which has
     only DEGREE keys, the recursively remove
*/

static bool
bpt_is_root(const TOID(struct bpt_node) *t)
{
    return TOID_IS_NULL(D_RO(*t)->parent);
}

static int
bpt_remove_key_and_data(PMEMobjpool *pop,
                        TOID(struct bpt_node) *node, const char *key)
{
    struct bpt_node *node_ptr = D_RW(*node);
    
    unsigned long long i = 0;
    for (i = 0; i < node_ptr->num_of_keys; i++) {
        if (strcmp(str_get(&node_ptr->keys[i]), key) == 0)
            break;
    }

    for(; i < node_ptr->num_of_keys; i++) {
        node_ptr->keys[i] = node_ptr->keys[i + 1];
        node_ptr->data[i] = node_ptr->data[i + 1];
    }

    node_ptr->num_of_keys--;
    str_free(&node_ptr->keys[node_ptr->num_of_keys]);
    str_free(&node_ptr->data[node_ptr->num_of_keys]);
    node_ptr->keys[node_ptr->num_of_keys] = NULL_STR;
    node_ptr->data[node_ptr->num_of_keys] = NULL_STR;
    pmemobj_persist(pop, node_ptr, sizeof(struct bpt_node));
    return 1;
}

static const TOID(struct bpt_node) *
bpt_check_redistribute(const TOID(struct bpt_node) *t)
{
    const struct bpt_node *t_ptr = D_RO(*t);
    
    if (TOID_IS_NULL(t_ptr->parent))
        return NULL;

    const TOID(struct bpt_node) *parent = &t_ptr->parent;
    const struct bpt_node *parent_ptr = D_RO(*parent);
    unsigned long long i = 0;
    for (i = 0; i < parent_ptr->num_of_children; i++) {
        if (TOID_EQUALS(parent_ptr->children[i], *t))
            break;
    }

    const TOID(struct bpt_node) *left
        = &parent_ptr->children[(i == 0) ? 0 : i - 1];
    
    const TOID(struct bpt_node) *right =
        &parent_ptr->children[(i == parent_ptr->num_of_children - 1) ? i : i + 1];

    if (D_RO(*left)->num_of_keys > DEGREE / 2)
        return left;
    else if (D_RO(*right)->num_of_keys > DEGREE / 2) {
        return right;
    }
    return NULL;
}


// redistribute leaf nodes, not internal nodes
static int
redistribute_leaf(PMEMobjpool *pop, TOID(struct bpt) *t,
                  TOID(struct bpt_node) *leaf, const char *key)
{
    struct bpt_node *leaf_ptr = D_RW(*leaf);
    
    TOID(struct bpt_node) *parent = &leaf_ptr->parent;
    TOID(struct bpt_node) *non_leaf = find_non_leaf(t, key);

    struct bpt_node *parent_ptr = D_RW(*parent);
    struct bpt_node *non_leaf_ptr;
    unsigned long long i = 0;
    unsigned long long idx_replace_key = 0;
    unsigned long long idx_child = 0;
    TOID(struct string) *replace_key;

    // no need to use TOIS_IS_NULL here
    if (non_leaf) {
        non_leaf_ptr = D_RW(*non_leaf);
        for (idx_replace_key = 0;
             idx_replace_key < non_leaf_ptr->num_of_keys; idx_replace_key++)
            if (strcmp(str_get(&non_leaf_ptr->keys[idx_replace_key]), key) == 0)
                break;            
    }

    
    for (i = 0; i < parent_ptr->num_of_children; i++)
        if (TOID_EQUALS(parent_ptr->children[i], *leaf)) {
            idx_child = i;
            break;            
        }


    unsigned long long idx_left = (i == 0) ? 0 : i - 1;
    unsigned long long idx_right =
        (i == parent_ptr->num_of_children - 1) ? i : i + 1;
    TOID(struct bpt_node) *left = &parent_ptr->children[idx_left];
    TOID(struct bpt_node) *right = &parent_ptr->children[idx_right];

    struct bpt_node *left_ptr = D_RW(*left);
    struct bpt_node *right_ptr = D_RW(*right);
 
    if (left_ptr->num_of_keys > DEGREE / 2) {
        unsigned long long tail = left_ptr->num_of_keys - 1;
        parent_ptr->keys[idx_left] = left_ptr->keys[tail];
        
        for (i = 0; i < leaf_ptr->num_of_keys; i++) {
            if (strcmp(str_get(&leaf_ptr->keys[i]), key) == 0) {
                str_free(&leaf_ptr->keys[i]);
                str_free(&leaf_ptr->data[i]);
                for (; i > 0; i--) {
                    leaf_ptr->keys[i] = leaf_ptr->keys[i-1];
                    leaf_ptr->data[i] = leaf_ptr->data[i-1];
                }
                leaf_ptr->keys[0] = left_ptr->keys[tail];
                leaf_ptr->data[0] = left_ptr->data[tail];
                left_ptr->keys[tail] = NULL_STR;
                left_ptr->data[tail] = NULL_STR;
                left_ptr->num_of_keys--;
                replace_key = &leaf_ptr->keys[0];

                pmemobj_persist(pop, leaf_ptr, sizeof(struct bpt_node));
                pmemobj_persist(pop, left_ptr, sizeof(struct bpt_node));
                pmemobj_persist(pop, parent_ptr, sizeof(struct bpt_node));
                break;
            }
        }
    }
    else if (right_ptr->num_of_keys > DEGREE / 2) {
        unsigned long long tail = leaf_ptr->num_of_keys - 1;
        for (i = 0; i < leaf_ptr->num_of_keys; i++) {
            if (strcmp(str_get(&leaf_ptr->keys[i]), key) == 0) {
                str_free(&leaf_ptr->keys[i]);
                str_free(&leaf_ptr->data[i]);
                for (; i < leaf_ptr->num_of_keys - 1; i++) {
                    leaf_ptr->keys[i] = leaf_ptr->keys[i+1];
                    leaf_ptr->data[i] = leaf_ptr->data[i+1];
                }
                leaf_ptr->keys[tail] = right_ptr->keys[0];
                leaf_ptr->data[tail] = right_ptr->data[0];

                for (unsigned long long j = 0; j < right_ptr->num_of_keys; j++) {
                    right_ptr->keys[j] = right_ptr->keys[j+1];
                    right_ptr->data[j] = right_ptr->data[j+1];
                }
                parent_ptr->keys[idx_child] = right_ptr->keys[0];
                right_ptr->num_of_keys--;
                replace_key = &leaf_ptr->keys[0];
                pmemobj_persist(pop, leaf_ptr, sizeof(struct bpt_node));
                pmemobj_persist(pop, right_ptr, sizeof(struct bpt_node));
                pmemobj_persist(pop, parent_ptr, sizeof(struct bpt_node));
                break;
            }
        }
    }
    
    // if the key deleted resides in grandparent's keys, replace it
    if (non_leaf) {
        // str_write(pop,
        //           &non_leaf_ptr->keys[idx_replace_key], str_get(replace_key));
        non_leaf_ptr->keys[idx_replace_key] = *replace_key;
    }

    return 1;
}

static int
bpt_simple_delete(PMEMobjpool *pop, TOID(struct bpt) *t,
                  TOID(struct bpt_node) *leaf, const char *key)
{

    if (!bpt_is_root(leaf)) {
        // this branch will be executed only when deletin takes place on a
        // right subtree of a key
        TOID(struct bpt_node) *non_leaf = find_non_leaf(t, key);
        struct bpt_node *non_leaf_ptr;
        unsigned long long i = 0;
        // no need to use TOID_IS_NULL here
        if (non_leaf) {
            non_leaf_ptr = D_RW(*non_leaf);
            for (i = 0; i < non_leaf_ptr->num_of_keys; i++)
                if (strcmp(str_get(&non_leaf_ptr->keys[i]), key) == 0) {
                    break;
                }
        }
        bpt_remove_key_and_data(pop, leaf, key);
        // replace the key
        if (non_leaf)
            non_leaf_ptr->keys[i] = D_RO(*leaf)->keys[0];
        
        return 1;
    } else
        return bpt_remove_key_and_data(pop, leaf, key);
}



// only used to merge leaves
static TOID(struct bpt_node) *
merge_leaves(PMEMobjpool *pop,
             TOID(struct bpt) *t, TOID(struct bpt_node) *leaf, const char *key)
{
    if (!pop) {
        return NULL;
    }
    
    struct bpt *t_ptr = D_RW(*t);
    struct bpt_node *leaf_ptr = D_RW(*leaf);
    
    TOID(struct bpt_node) *parent = &leaf_ptr->parent;
    struct bpt_node *parent_ptr = D_RW(*parent);
    unsigned long long i;
    unsigned long long idx_left;
    unsigned long long idx_right;
    // delete the key
    for (i = 0; i < leaf_ptr->num_of_keys; i++) {
        if (strcmp(str_get(&leaf_ptr->keys[i]), key) == 0)
            break;
    }

    t_ptr->free_key = leaf_ptr->keys[i];
    str_free(&leaf_ptr->data[i]);
    
    for (unsigned long long j = i; j <= leaf_ptr->num_of_keys - 1; j++) {
        leaf_ptr->keys[j] = leaf_ptr->keys[j+1];
        leaf_ptr->data[j] = leaf_ptr->data[j+1];
    }
    leaf_ptr->num_of_keys--;

    // find a proper sibling
    for (i = 0; i < parent_ptr->num_of_children; i++) {
        if (TOID_EQUALS(parent_ptr->children[i], *leaf))
            break;
    }

    idx_left = (i == 0) ? 0 : i - 1;
    idx_right = (i == parent_ptr->num_of_children - 1) ? i : i + 1;
    TOID(struct bpt_node) *left = &parent_ptr->children[idx_left];
    TOID(struct bpt_node) *right = &parent_ptr->children[idx_right];

    struct bpt_node *left_ptr = D_RW(*left);
    struct bpt_node *right_ptr = D_RW(*right);
    unsigned long long delete_position = 0;
    TOID(struct bpt_node) *rev;
    
    // merge left
    if (i != 0 && left_ptr->num_of_keys <= DEGREE / 2) {
        delete_position = i;
        right_ptr = NULL;
    } else {
        // merge to right
        delete_position = idx_right;
        left_ptr = NULL;        
    }

    // merge
    if (left_ptr) {
        // merge leaf into its left sibling
        for (i = 0; i < leaf_ptr->num_of_keys; i++) {
            left_ptr->keys[i + left_ptr->num_of_keys] = leaf_ptr->keys[i];
            leaf_ptr->keys[i] = NULL_STR;
            left_ptr->data[i + left_ptr->num_of_keys] = leaf_ptr->data[i];
            leaf_ptr->data[i] = NULL_STR;
        }
        // leaf_ptr->link.prev_ptr->next = leaf_ptr->link.next;
        // leaf_ptr->link.next_ptr->prev = leaf_ptr->link.prev;
        // list_remove(pop, &leaf_ptr->link);
        struct list_node *prev = pmemobj_direct(leaf_ptr->link.prev);
        struct list_node *next = pmemobj_direct(leaf_ptr->link.next);
        prev->next = leaf_ptr->link.next;
        next->prev = leaf_ptr->link.prev;
        left_ptr->num_of_keys += leaf_ptr->num_of_keys;
        bpt_free_leaf(leaf);
        rev = left;
    } else {
        // merge leaf into its right sibling
        // we merge right into leaf and modify the point in parent pointing
        // to right to point to leaf, so we may reduce some overhead
        for (i = 0; i < right_ptr->num_of_keys; i++) {
            leaf_ptr->keys[i + leaf_ptr->num_of_keys] = right_ptr->keys[i];
            right_ptr->keys[i] = NULL_STR;
            leaf_ptr->data[i + leaf_ptr->num_of_keys] = right_ptr->data[i];
            right_ptr->data[i] = NULL_STR;

        }
        // right->link.prev->next = right->link.next;
        // right->link.next->prev =right->link.prev;
        // list_remove(pop, &right_ptr->link);
        struct list_node *prev = pmemobj_direct(right_ptr->link.prev);
        struct list_node *next = pmemobj_direct(right_ptr->link.next);
        prev->next = right_ptr->link.next;
        next->prev = right_ptr->link.prev;
        parent_ptr->children[idx_right] = *leaf;
        leaf_ptr->num_of_keys += right_ptr->num_of_keys;
        bpt_free_leaf(right);
        rev = leaf;
    }
    
    // align children
    unsigned long long j;
    for (j = delete_position; j <= parent_ptr->num_of_children - 1; j++) {
        parent_ptr->children[j] = parent_ptr->children[j+1];
    }
    parent_ptr->num_of_children--;
    return rev;
}


static int
bpt_insert_key(PMEMobjpool *pop,
               TOID(struct bpt_node) *t, const TOID(struct string) *__key__) {
    if (!pop)
        return -1;
    struct bpt_node *t_ptr = D_RW(*t);
    const char *key = str_get(__key__);
    unsigned long long i = 0;
    for (i = 0; i < t_ptr->num_of_keys; i++) {
        if (strcmp(str_get(&t_ptr->keys[i]), key) == 0)
            return 1;

        if (strcmp(str_get(&t_ptr->keys[i]), key) > 0)
            break;
    }

    for (unsigned long long j = t_ptr->num_of_keys; j > i; j--) {
        t_ptr->keys[j] = t_ptr->keys[j-1];
    }
    // str_write(pop, &t_ptr->keys[i], key);
    t_ptr->keys[i] = *__key__;
    t_ptr->num_of_keys++;
    return 1;
}

static int
redistribute_internal(PMEMobjpool *pop, const TOID(struct string) *split_key,
                      TOID(struct bpt_node) *node, TOID(struct bpt_node) *left,
                      TOID(struct bpt_node) *right) {
    struct bpt_node *node_ptr = D_RW(*node);
    struct bpt_node *left_ptr = (left) ? D_RW(*left) : NULL;
    struct bpt_node *right_ptr = (right) ? D_RW(*right) : NULL;
    
    TOID(struct bpt_node) *parent = &node_ptr->parent;
    struct bpt_node *parent_ptr = D_RW(*parent);
    unsigned long long i = 0;
    unsigned long long j = 0;
    unsigned long long split = 0;
    for (split = 0; split < parent_ptr->num_of_keys; split++) {
        if (strcmp(str_get(&parent_ptr->keys[split]), str_get(split_key)) == 0)
            break;
    }

    for (i = 0; i < parent_ptr->num_of_children; i++) {
        if (TOID_EQUALS(parent_ptr->children[i], *node))
            break;
    }
    
    if (!left_ptr && right_ptr->num_of_keys > DEGREE / 2) {
        // borrow a key from right
        // str_write(pop, &node_ptr->keys[node_ptr->num_of_keys++],
        //           str_get(split_key));
        node_ptr->keys[node_ptr->num_of_keys++] = *split_key;
        node_ptr->children[node_ptr->num_of_children++] = right_ptr->children[0];
        D_RW(right_ptr->children[0])->parent = *node;
        parent_ptr->keys[split] = right_ptr->keys[0];
        for (j = 0; j <= right_ptr->num_of_keys - 1; j++) {
            right_ptr->keys[j] = right_ptr->keys[j+1];
            right_ptr->children[j] = right_ptr->children[j+1];
        }
        right_ptr->children[j] = NULL_BPT_NODE;
        right_ptr->num_of_children--;
        right_ptr->num_of_keys--;
        pmemobj_persist(pop, parent_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, node_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, right_ptr, sizeof(struct bpt_node));
        
    } else if (!right_ptr && left_ptr->num_of_keys > DEGREE / 2) {
        // make some space
        node_ptr->children[node_ptr->num_of_children] =
            node_ptr->children[node_ptr->num_of_children-1];
        for (j = node_ptr->num_of_keys; j > 0; j--) {
            node_ptr->keys[j] = node_ptr->keys[j-1];
            node_ptr->children[j] = node_ptr->children[j-1];
        }
        // str_write(pop, &node_ptr->keys[0], str_get(split_key));
        node_ptr->keys[0] = *split_key;
        node_ptr->children[0] = left_ptr->children[left_ptr->num_of_children-1];
        D_RW(left_ptr->children[left_ptr->num_of_children-1])->parent = *node;
        parent_ptr->keys[split] = left_ptr->keys[left_ptr->num_of_keys-1];
        left_ptr->keys[left_ptr->num_of_keys-1] = NULL_STR;
        left_ptr->children[left_ptr->num_of_children-1] = NULL_BPT_NODE;
        node_ptr->num_of_keys++;
        node_ptr->num_of_children++;
        left_ptr->num_of_children--;
        left_ptr->num_of_keys--;
        pmemobj_persist(pop, parent_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, node_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, left_ptr, sizeof(struct bpt_node));
    } else if (left_ptr->num_of_keys > DEGREE/2 &&
               right_ptr->num_of_keys > DEGREE/2) {
        // borrow from right
        node_ptr->children[node_ptr->num_of_children++] = right_ptr->children[0];
        D_RW(right_ptr->children[0])->parent = *node;
        // str_write(pop, &node_ptr->keys[node_ptr->num_of_keys++],
        //           str_get(split_key));
        node_ptr->keys[node_ptr->num_of_keys++] = *split_key;
        parent_ptr->keys[split] = right_ptr->keys[0];
        for (j = 0; j <= right_ptr->num_of_keys - 1; j++) {
            right_ptr->keys[j] = right_ptr->keys[j+1];
            right_ptr->children[j] = right_ptr->children[j+1];
        }
        right_ptr->children[j+1] = NULL_BPT_NODE;
        right_ptr->num_of_children--;
        right_ptr->num_of_keys--;
        pmemobj_persist(pop, parent_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, node_ptr, sizeof(struct bpt_node));
        pmemobj_persist(pop, right_ptr, sizeof(struct bpt_node));
    } else {
        return -1;
    }
    return 1;
}

static int
merge_internal(PMEMobjpool *pop, TOID(struct bpt) *t,
               TOID(struct bpt_node) *parent,
               const TOID(struct string) *split_key)
{
    // merge parent into a proper sibling, incorporating a split key from parent
    // of parent which seperate this parent and the sibling
    unsigned long long i = 0;
    struct bpt_node *parent_ptr = D_RW(*parent);
    TOID(struct bpt_node) grandparent = parent_ptr->parent;
    struct bpt_node *gparent_ptr = D_RW(grandparent);

    // struct bpt_node *child_ptr = D_RW(parent_ptr->children[0]);
    // merge this parent and its sibling
    unsigned long long idx_left = 0;
    unsigned long long idx_right = 0;
    for (i = 0; i < gparent_ptr->num_of_children; i++) {
        if (TOID_EQUALS(gparent_ptr->children[i], *parent))
            break;
    }

    idx_left = (i == 0) ? 0 : i - 1;
    idx_right = (i == gparent_ptr->num_of_children - 1) ? i : i + 1;
    TOID(struct bpt_node) left = gparent_ptr->children[idx_left];
    TOID(struct bpt_node) right = gparent_ptr->children[idx_right];
    struct bpt_node *l_ptr = D_RW(left);
    struct bpt_node *r_ptr = D_RW(right);
    TOID(struct bpt_node) *merged = NULL;
    bool left_available = false;
    bool right_available = false;
    unsigned long long delete_position = i;
    if (i == 0 || l_ptr->num_of_keys > DEGREE / 2) {
        delete_position = idx_right;
        if (l_ptr->num_of_keys > DEGREE / 2)
            left_available = true;
        left = NULL_BPT_NODE;
    }

    if (i == gparent_ptr->num_of_children-1 || r_ptr->num_of_keys > DEGREE/2) {
        delete_position = i;
        if (r_ptr->num_of_keys > DEGREE / 2)
            right_available = true;
        right = NULL_BPT_NODE;
    }

    // what sucks is that an internal node may have no proper sibling to merge in
    // so what should we do ?
    // we have to borrow a key from proper sibling just as what we do to leaves.
    // and notice we have to change our split key
    if (TOID_IS_NULL(left) && TOID_IS_NULL(right)) {  // merge is impossible
        if (left_available) {
            split_key = &gparent_ptr->keys[i-1];
            redistribute_internal(pop, split_key, parent,
                                  &gparent_ptr->children[idx_left], NULL);
        }
        else {
            split_key = &gparent_ptr->keys[i];
            redistribute_internal(pop, split_key, parent, NULL,
                                  &gparent_ptr->children[idx_right]);
        }
        return 1;
    }


    struct bpt_node *dont_use_this_ptr;
    if (!TOID_IS_NULL(left)) {
        for (i = 0; i < parent_ptr->num_of_keys; i++) {
            l_ptr->keys[i+l_ptr->num_of_keys] = parent_ptr->keys[i];
            parent_ptr->keys[i] = NULL_STR;
            l_ptr->children[i+l_ptr->num_of_children] = parent_ptr->children[i];
            D_RW(parent_ptr->children[i])->parent = left;
            parent_ptr->children[i] = NULL_BPT_NODE;
        }
        
        l_ptr->children[i+l_ptr->num_of_children] = parent_ptr->children[i];
        struct bpt_node *child_ptr = D_RW(parent_ptr->children[i]);
        dont_use_this_ptr = child_ptr;
        // D_RW(parent_ptr->children[i])->parent = left;
        child_ptr->parent = left;
        parent_ptr->children[i] = NULL_BPT_NODE;

        l_ptr->num_of_keys += parent_ptr->num_of_keys;
        l_ptr->num_of_children += parent_ptr->num_of_children;


        TOID(struct bpt_node) p = *parent;
        bpt_free_non_leaf(&p);
        bpt_insert_key(pop, &left, split_key);
        merged = &left;
        pmemobj_persist(pop, l_ptr, sizeof(struct bpt_node));
    } else {
        struct bpt_node *child_ptr = NULL;
        for (i = 0; i < r_ptr->num_of_keys; i++) {
            parent_ptr->keys[i+parent_ptr->num_of_keys] = r_ptr->keys[i];
            r_ptr->keys[i] = NULL_STR;
            parent_ptr->children[i+parent_ptr->num_of_children] = r_ptr->children[i];
            child_ptr = D_RW(r_ptr->children[i]);
            // D_RW(r_ptr->children[i])->parent = *parent;
            child_ptr->parent = *parent;

            r_ptr->children[i] = NULL_BPT_NODE;
        }
        
        parent_ptr->children[i+parent_ptr->num_of_children] = r_ptr->children[i];
        D_RW(r_ptr->children[i])->parent = *parent;
        r_ptr->children[i] = NULL_BPT_NODE;

        parent_ptr->num_of_keys += r_ptr->num_of_keys;
        parent_ptr->num_of_children += r_ptr->num_of_children;

        bpt_free_non_leaf(&right);
        gparent_ptr->children[idx_right] = *parent;
        bpt_insert_key(pop, parent, split_key);
        merged = parent;
        pmemobj_persist(pop, parent_ptr, sizeof(struct bpt_node));
    }

    if (bpt_is_root(&grandparent) && gparent_ptr->num_of_keys == 1) {
        bpt_free_non_leaf(&grandparent);
        struct bpt *t_ptr = D_RW(*t);
        t_ptr->root = *merged;

        D_RW(*merged)->parent = NULL_BPT_NODE;
        pmemobj_persist(pop, t_ptr, sizeof(struct bpt));
        return 1;
    }

    // align children
    unsigned long long j;
    for ( j = delete_position; j <= gparent_ptr->num_of_children - 1; j++) {
        gparent_ptr->children[j] = gparent_ptr->children[j+1];
    }
    gparent_ptr->num_of_children--;
    pmemobj_persist(pop, gparent_ptr, sizeof(struct bpt_node));
    return 0;
}

static int
merge(PMEMobjpool *pop, TOID(struct bpt) *t,
      TOID(struct bpt_node) *parent, const char *key,
      const TOID(struct string) *split_key)
{
    // remove the split key

    struct bpt_node *parent_ptr = D_RW(*parent);
    
    unsigned long long i = 0;
    for (i = 0; i < parent_ptr->num_of_keys; i++) {
        if (strcmp(str_get(&parent_ptr->keys[i]), str_get(split_key)) == 0) {
            unsigned long long j = 0;
            for (j = i; j <= parent_ptr->num_of_keys - 1; j++) {
                parent_ptr->keys[j] = parent_ptr->keys[j+1];
            }
            break;
        }
    }

    pmemobj_persist(pop, parent_ptr, sizeof(struct bpt_node));
    // parent has enough keys
    parent_ptr->num_of_keys--;
    unsigned long long num_of_keys = parent_ptr->num_of_keys;

    if (num_of_keys >= DEGREE / 2 || (bpt_is_root(parent) && num_of_keys >= 1))
        return 1;

    if (bpt_is_root(parent) && num_of_keys == 0) {
        D_RW(*t)->root = parent_ptr->children[0];
        D_RW(parent_ptr->children[0])->parent = NULL_BPT_NODE;
        TOID(struct bpt_node) p = *parent;
        bpt_free_non_leaf(&p);
        pmemobj_persist(pop, D_RW(*t), sizeof(struct bpt));
        return 1;
    }
    // now we have to find a split key for parent

    TOID(struct bpt_node) *grandparent = &parent_ptr->parent;
    struct bpt_node *gparent_ptr = D_RW(*grandparent);

    for (i = 0; i < gparent_ptr->num_of_children; i++) {
        if (TOID_EQUALS(gparent_ptr->children[i], *parent))
            break;
    }

    unsigned long long idx_left = (i == 0) ? 0 : i - 1;
    unsigned long long idx_right =
        (i == gparent_ptr->num_of_children - 1) ? i : i + 1;
    unsigned long long split = i-1; // merge to left
    TOID(struct bpt_node) *left = &gparent_ptr->children[idx_left];
    TOID(struct bpt_node) *right = &gparent_ptr->children[idx_right];
    struct bpt_node *l_ptr = D_RW(*left);
    struct bpt_node *r_ptr = D_RW(*right);
    if (i == 0 || l_ptr->num_of_keys > DEGREE / 2) {
        split = i;
    }

    if (i == gparent_ptr->num_of_children-1 || r_ptr->num_of_keys > DEGREE/2) {
        split = i -1;
    }

    split_key = &gparent_ptr->keys[split];

    // 1 means everything is done, no more recursion
    if (merge_internal(pop, t, parent, split_key) == 1)
        return 1;
    merge(pop, t, grandparent, key, split_key);
    return 1;
}

// this functin first finish leaf merging
// then call merge to see if it is necessary to merge parent
/*
  procedure:
      1. find leaf and internal node which contains the key to be deleted
      2. find split key
      3. replace the key in the internal node with the split key
      4. merge leaf and its chosen sibling, 
      5. remove the split key in parent
      6. check if we need to merge parent recursively, if so , we have to 
         incorporate the split key which split parent and parent's sibling
      7. if parent is root and we have to remove the last key during merging,
         just remove the key and delete this root. Delegate root to the newly
         merged node
 */
static int
bpt_complex_delete(PMEMobjpool *pop, TOID(struct bpt) *t,
                   TOID(struct bpt_node) *leaf, const char *key)
{
    // find internal node which contains the keys to be deleted
    TOID(struct bpt_node) *non_leaf = find_non_leaf(t, key);
    
    // find split key
    struct bpt_node *leaf_ptr = D_RW(*leaf);
    TOID(struct bpt_node) *parent = &leaf_ptr->parent; // impossible to be NULL
    struct bpt_node *parent_ptr = D_RW(*parent);
    unsigned long long i = 0;
    for (i = 0; i < parent_ptr->num_of_children; i++) {
        if (TOID_EQUALS(parent_ptr->children[i], *leaf))
            break;
    }
    unsigned long long split = (i == 0) ? 0 : i - 1;


    // merge
    TOID(struct bpt_node) *merged = merge_leaves(pop, t, leaf, key);
    
    // replace the key
    const TOID(struct string) replace_key = (D_RW(*merged)->keys[0]);
    if (non_leaf) {
        struct bpt_node *non_leaf_ptr = D_RW(*non_leaf);
        for (unsigned long long j = 0; j < non_leaf_ptr->num_of_keys; j++)
            if (strcmp(str_get(&non_leaf_ptr->keys[j]), key) == 0) {
                // str_write(pop, &non_leaf_ptr->keys[j], replace_key);
                non_leaf_ptr->keys[j] = replace_key;
                break;
            }
    }
    TOID(struct string) *split_key = &parent_ptr->keys[split];
    // TOID(struct bpt_node) child = parent_ptr->children[0];

    merge(pop, t, parent, key, split_key);
    // struct bpt_node *child_ptr = D_RW(child);
    str_free(&D_RW(*t)->free_key);
    
    D_RW(*t)->free_key = NULL_STR;
    return 1;
}

int
bpt_delete(PMEMobjpool *pop, TOID(struct bpt) *t, const char *key)
{
    if (TOID_IS_NULL(D_RO(*t)->root))
        return 1;
    unsigned long long  i = 0; 
    TOID(struct bpt_node) *leaf = find_leaf(t, key);
    struct bpt_node *leaf_ptr = D_RW(*leaf);
    for (i = 0; i < leaf_ptr->num_of_keys; i++) {
        if (strcmp(str_get(&leaf_ptr->keys[i]), key) == 0)
            break;
    }

    if (i == leaf_ptr->num_of_keys)
        return 1;
    
    TOID(struct bpt_node) *parent = &leaf_ptr->parent;
        
    if (leaf_ptr->num_of_keys > DEGREE / 2 || bpt_is_root(leaf)) {
        bpt_simple_delete(pop, t, leaf, key);

        // tree is destroyed
        if (leaf_ptr->num_of_keys == 0) {
            bpt_free_leaf(leaf);
            D_RW(*t)->root = NULL_BPT_NODE;
        }
        // index key should be replaced it is resides in parent's key list
        if (!TOID_IS_NULL(*parent)) {
            struct bpt_node *parent_ptr = D_RW(*parent);
            for (i = 0; i < parent_ptr->num_of_keys; i++) {
                if (strcmp(str_get(&parent_ptr->keys[i]), key) == 0) {
                    parent_ptr->keys[i] = leaf_ptr->keys[0];
                    return 1;
                }
            }
        }
    } else {
        // if one of leaf's nearest siblings has enough keys to share
        // borrow a key from this sibling
        if (bpt_check_redistribute(leaf))
            return redistribute_leaf(pop, t, leaf, key);
        else
            // no sibling can offer us a key
            // merge is required
            return bpt_complex_delete(pop, t, leaf, key);
    }
    return 1;
}
