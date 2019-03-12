#include "nvbptree.h"
#include "en_debug.h"
#include "debug.h"

int
nv_bpt_new(PMEMobjpool *pop, nv_bpt_t *t)
{
    DEBUG_ENT();
    DEBUG_MESG("values of nv_bpt_t *t: \n"
               "address: %p, pool_uuid_lo: %ld, off: %ld\n",
               D_RO(t->t), t->t.oid.pool_uuid_lo, t->t.oid.off)
    int rev = bpt_new(pop, &t->t);
    DEBUG_MESG("values of nv_bpt_t *t: \n"
               "address: %p, pool_uuid_lo: %ld, off: %ld\n",
               D_RO(t->t), t->t.oid.pool_uuid_lo, t->t.oid.off)

    DEBUG_LEA();
    return rev;
}

int nv_bpt_retrieve(PMEMobjpool *pop, nv_bpt_t *t)
{
    return bpt_retrieve(pop, &t->t);
}


int
nv_bpt_insert(PMEMobjpool *pop, nv_bpt_t *t, const char *k, const char *v)
{
    return bpt_insert(pop, &t->t, k, v);
}


int
nv_bpt_delete(PMEMobjpool *pop, nv_bpt_t *t, const char *key)
{
    return bpt_delete(pop, &t->t, key);
}

int
nv_bpt_get(nv_bpt_t *t, const char *key, char *buffer)
{
    return bpt_get(&t->t, key, buffer);   
}

int
nv_bpt_scan(nv_bpt_t *t, const char *start, const char *end, char **buffer)
{
    return bpt_range(&t->t, start, end, buffer);
}

int
nv_bpt_destroy(nv_bpt_t *t)
{
    return bpt_destroy(&t->t);
}

void
nv_bpt_print(nv_bpt_t *t)
{
    bpt_print(&t->t);
}

void
nv_bpt_print_leaves(nv_bpt_t *t)
{
    bpt_print_leaves(&t->t);
}
