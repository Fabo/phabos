#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stddef.h>

typedef struct vector {
    void **buffer;
    size_t size;
    size_t elem_size;
    size_t count;
} vector_t;

int vector_init(struct vector *vector, size_t elem_size, size_t size);
int vector_resize(struct vector *vector, size_t size);
int vector_push_back(struct vector *vector, const void *data);
void vector_destroy(struct vector *vector);

static inline void vector_put(struct vector *vector, unsigned int i, void *data)
{
    vector->buffer[i] = data;
}

static inline void *vector_get(struct vector *vector, unsigned int i)
{
    return vector->buffer[i];
}

static inline size_t vector_get_size(struct vector *vector)
{
    return vector->count;
}

static inline void **vector_get_data(struct vector *vector)
{
    return vector->buffer;
}

#endif /* __VECTOR_H__ */

