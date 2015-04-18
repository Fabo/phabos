
#define MPOOL_BUFSIZE 4096

struct mpool_buffer {
    void *object;
    struct list_head list;
};

struct mpool {
    size_t chunk_size;

    struct list_head free;
    struct list_head partial;
    struct list_head full;
};

struct mpool *mpool_create(size_t chunk_size)
{
    struct mpool *pool = malloc(sizeof(*pool));
    RET_IF_FAIL(pool, NULL);

    pool->chunk_size = chunk_size;
    list_init(&pool->buffer);
    list_init(&pool->free);
    list_init(&pool->full);

    return pool;
}

void mpool_destroy(struct mpool *pool)
{
    struct mpool_buffer *buffer;

    if (!pool)
        return;

    list_foreach_safe(&pool->buffer, iter) {
        buffer = list_entry(iter, struct mpool_buffer, list);
        free(buffer);
    }

    free(pool);
}

static size_t mpool_get_nbr_obj_per_buffer(size_t chunk_size)
{
    return MPOOL_BUFSIZE / chunk_size + MPOOL_BUFSIZE % chunk_size ? 1 : 0;
}

static size_t mpool_get_buf_size(size_t chunk_size)
{
    return MPOOL_BUFSIZE +
           (mpool_get_nbr_obj_per_buffer(chunk_size) + 1) *
                sizeof(struct mpool_buffer);
}

static int mpool_grow(struct mpool *pool)
{
    uint32_t buf_addr;
    void *buf;
    struct mpool_buffer *buffer;

    RET_IF_FAIL(pool, -EINVAL);

    buf = malloc(mpool_get_buf_size(pool));
    RET_IF_FAIL(buf, -ENOMEM);

    buffer = buf;
    buf_addr = (uint32_t buf);

    list_init(&buffer->list);
    list_add(&pool->buffer, &buffer->list);

    for (unsigned int i = 0; i < mpool_get_nbr_obj_per_buffer(pool->chunk_size);
         i++) {
        buffer = (struct mpool_buffer*) buf_addr + sizeof(struct mpool_buffer) +
                 i * (sizeof(struct mpool_buffer) + pool->chunk_size);
        buffer->object = buffer + 1;
        list_init(&buffer->list);
        list_add(&pool->free, &buffer->list);
    }

    return 0;
}

static int mpool_shrink(struct mpool *pool)
{
    

    return 0;
}

void *mpool_alloc(struct mpool *pool)
{
    int retval;
    struct mpool_buffer *buffer;

    RET_IF_FAIL(pool, NULL);

    if (list_is_empty(pool->free)) {
        retval = mpool_grow();
        RET_IF_FAIL(!retval, NULL);
    }

    RET_IF_FAIL(!list_is_empty(&pool->free), NULL);

    buffer = list_first_entry(&pool->free, struct mpool_buffer, list);
    list_del(&buffer->list);
    list_add(&pool->full, &buffer->list);

    return buffer->object;
}

void mpool_free(struct mpool *pool, void *object)
{
    RET_IF_FAIL(pool,);
    RET_IF_FAIL(object,);

    struct mpool_buffer *buf = containerof(object, struct mpool_buffer, object);
    list_del(&buf->list);
    list_add(&pool->free, &buf->list);
}
