#ifndef __FROST_STRING__
#define __FROST_STRING__

#include <libpmemobj.h>
#include <string.h>
POBJ_LAYOUT_BEGIN(string);
POBJ_LAYOUT_ROOT(string, struct string);
POBJ_LAYOUT_END(string);

#define NULL_STR (TOID_NULL(struct string))

struct __pad {
    size_t l;
};

typedef struct string {
    struct __pad len;
    char *data;
} str_t;

int
str_new(PMEMobjpool *pop, TOID(struct string) *str, const char *data);

const char *
str_get(const TOID(struct string) *str);

size_t
str_len(const TOID(struct string) *str);

void
str_write(PMEMobjpool *pop, TOID(struct string) *str, const char *data);

void str_free(TOID(struct string) *str);
#endif
