#include "string.h"
static int
string_constr(PMEMobjpool *pop, void *ptr, void *arg)
{
    str_t *str_ptr = ptr;
    str_ptr->data = (ptr + sizeof(struct __pad));
    str_t *a = (str_t *)arg;

    strcpy(str_ptr->data, a->data);
    str_ptr->len = a->len;
    pmemobj_persist(pop, str_ptr, sizeof(struct string) + str_ptr->len.l + 1);
    return 0;
}

int
str_new(PMEMobjpool *pop, TOID(struct string) *str, const char *data)
{
    size_t len = strlen(data);
    str_t arg = {
        .data = data,
        .len.l = len
    };
    POBJ_ALLOC(pop, str, struct string,
               sizeof(struct string) + len + 1, string_constr, &arg);
    return 1;
}

const char *
str_get(const TOID(struct string) *str)
{
    const str_t *str_ptr = D_RO(*str);
    return (str_ptr == NULL) ? 0 : ((char*)str_ptr + sizeof(struct __pad));
}

size_t
str_len(const TOID(struct string) *str)
{
    const str_t *str_ptr = D_RO(*str);
    return (str_ptr == 0) ? 0 : str_ptr->len.l;
}

void
str_write(PMEMobjpool *pop, TOID(struct string) *str, const char *data)
{
    size_t new_len = strlen(data);
    str_t *str_ptr = D_RW(*str);
    if (str_ptr == NULL) {
        str_new(pop, str, data);
        return;
    }

    if (str_ptr->len.l < new_len) {
        POBJ_REALLOC(pop, str, struct string, sizeof(struct string) + new_len + 1);
        str_ptr = D_RW(*str);
        strcpy(((char*)str_ptr) + sizeof(struct __pad), data);
        str_ptr->len.l = new_len;
    } else {
        strcpy(((char *)str_ptr) + sizeof(struct __pad), data);
        str_ptr->len.l = new_len;
    }
    pmemobj_persist(pop, str_ptr, sizeof(struct string) + new_len + 1);
}

void str_free(TOID(struct string) *str) {
    POBJ_FREE(str);
}
