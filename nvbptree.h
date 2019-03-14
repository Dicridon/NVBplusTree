#ifndef __FORST_BPTREE__
#define __FORST_BPTREE__
#include <libpmemobj.h>
#include "bplustree_dev.h"

typedef struct {
    TOID(struct bpt) t;
} nv_bpt_t;

int
nv_bpt_new(PMEMobjpool *pop, nv_bpt_t *t);

int
nv_bpt_insert(PMEMobjpool *pop, nv_bpt_t *t, const char *k, const char *v);



int
nv_bpt_delete(PMEMobjpool *pop, nv_bpt_t *t, const char *key);

int
nv_bpt_get(nv_bpt_t *t, const char *key, char *buffer);

int
nv_bpt_scan(nv_bpt_t *t, const char *start, const char *end, char **buffer);

int
nv_bpt_scan_test(nv_bpt_t *t, const char *start, unsigned long long n);

int
nv_bpt_destroy(nv_bpt_t *t);

void
nv_bpt_print(nv_bpt_t *t);

void
nv_bpt_print_leaves(nv_bpt_t *t);
#endif
