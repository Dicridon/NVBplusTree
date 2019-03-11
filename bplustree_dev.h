#ifndef __FROST_BPTREE_DEV__
#define __FROST_BPTREE_DEV__

#include <stdio.h>
#include <libpmemobj.h>
#include "list.h"
#include "string.h"


#define DEGREE (3)
#define MIN_ENTRIES (DEGREE)
#define NULL_BPT_NODE (TOID_NULL(struct bpt_node))

POBJ_LAYOUT_BEGIN(bptree);
POBJ_LAYOUT_ROOT(bptree, struct bpt);
POBJ_LAYOUT_TOID(bptree, struct bpt_node);
POBJ_LAYOUT_END(bptree);

enum bpt_node_type {
    NON_LEAF,
    LEAF
};

// Each node may have at most DEGREE children and DEGREE - 1 keys
// keys and children are paired
struct bpt_node {
    union {
        struct {
            // put link at the beginning of the structure
            // so we can traverse all the leaves
            TOID(struct list_node) link;
            TOID(struct string) data[DEGREE];
        };
        struct {
            unsigned long long num_of_children;
            // keys and children are paired during insertion
            TOID(struct bpt_node) children[DEGREE + 1];
        };
    };
    enum bpt_node_type type;
    unsigned long long num_of_keys;
    // one extra key will reudce overhead during insertion
    TOID(struct string) keys[DEGREE];
    TOID(struct bpt_node) parent;
};

struct bpt {
    int level;
    TOID(struct bpt_node) root;
    // linked children
    TOID(struct list) list;   
    // key and data to be freed, so I may avoid accessing freed memory
    TOID(struct string)* free_key;
};

int
bpt_new(PMEMobjpool *pop, TOID(struct bpt) *t);

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

// int
// bpt_serialize_f(bpt_t *t, char *file_name);
// 
// int
// bpt_serialize_fp(bpt_t *t, FILE *fp);

void
bpt_print(const TOID(struct bpt) *t);

void
bpt_print_leaves(const TOID(struct bpt) *t);

#endif
