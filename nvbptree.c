#include "nvbptree.h"
int nv_bpt_new(PMEMobjpool *pop, nv_bpt_t *t)
{
    return bpt_new(pop, t->t);
}


int
nv_bpt_insert(PMEMobjpool *pop, nv_bpt_t *t, const char *k, const char *v)
{
    return bpt_insert(pop, t->t, k, v);
}


int
nv_bpt_delete(PMEMobjpool *pop, nv_bpt_t *t, const char *key)
{
    return bpt_delete(pop, t->t, key);
}

int
nv_bpt_get(nv_bpt_t *t, const char *key, char *buffer)
{
    return bpt_get(t->t, key, buffer);   
}

int
nv_bpt_range(nv_bpt_t *t, const char *start, const char *end, char **buffer)
{
    return bpt_range(t->t, start, end, buffer);
}

int
nv_bpt_destroy(nv_bpt_t *t)
{
    return bpt_destroy(t->t);
}
