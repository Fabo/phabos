/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <phabos/vector.h>
#include <phabos/assert.h>
#include <phabos/utils.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int vector_init(struct vector *vector, size_t elem_size, size_t size)
{
    RET_IF_FAIL(vector, -EINVAL);
    RET_IF_FAIL(elem_size == 0, -EINVAL);
    RET_IF_FAIL(size == 0, -EINVAL);

    memset(vector, 0, sizeof(*vector));
    vector->size = size * elem_size;
    vector->count = size;
    vector->elem_size = elem_size;

    vector->buffer = zalloc(vector->size);
    RET_IF_FAIL(vector->buffer, -ENOMEM);

    return 0;
}

int vector_resize(struct vector *vector, size_t size)
{
    void *buffer;

    RET_IF_FAIL(vector, -EINVAL);
    RET_IF_FAIL(size == 0, -EINVAL);

    buffer = zalloc(size * vector->elem_size);
    RET_IF_FAIL(buffer, -ENOMEM);

    memcpy(buffer, vector->buffer, MIN(vector->size, size * vector->elem_size));
    free(vector->buffer);

    vector->size = size * vector->elem_size;
    vector->buffer = buffer;

    return 0;
}

int vector_push_back(struct vector *vector, const void *data)
{
#if 0
    RET_IF_FAIL(vector, -EINVAL);

//    if (vector->size == vector->real_size)
        vector_resize(vector, newsize);

    vector->buffer[i] = data;
#endif
    return 0;
}

void vector_destroy(struct vector *vector)
{
    free(vector->buffer);
}
