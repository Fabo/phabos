#include <asm/spinlock.h>
#include <phabos/assert.h>
#include <phabos/list.h>
#include <phabos/mm.h>
#include <phabos/utils.h>

#include <errno.h>

#define PAGE_ORDER 12
#define PAGE_SIZE (1 << PAGE_ORDER)
#define MIN_REGION_ORDER PAGE_ORDER
#define MAX_ADDRESSABLE_MEM_ORDER 31

static volatile unsigned int region;
static struct spinlock mm_lock = SPINLOCK_INIT(mm_lock);
static struct list_head mm_bucket[MAX_ADDRESSABLE_MEM_ORDER + 1];

struct mm_buffer {
    uint8_t bucket;
    uint8_t region;
    uint16_t flags;
    struct list_head list;

    // size of struct must be a multiple of 4 bytes
};

static size_t order_to_size(int order)
{
    return 1 << order;
}

int size_to_order(size_t size)
{
    int order = -1;
    size_t bit_set_count = 0;

    for (; size; size >>= 1) {
        order++;

        if (size & 1)
            bit_set_count++;
    }

    return bit_set_count == 1 ? order : order + 1;
}

int mm_add_region(unsigned long addr, unsigned order, unsigned int flags)
{
    static bool is_initialized = false;
    struct mm_buffer *buffer;

    RET_IF_FAIL(order >= MIN_REGION_ORDER, -EINVAL);
    RET_IF_FAIL(addr >= PAGE_SIZE, -EINVAL);
    RET_IF_FAIL(!(addr & 0x3), -EINVAL);
    RET_IF_FAIL(order < MAX_ADDRESSABLE_MEM_ORDER, -EINVAL);
    RET_IF_FAIL(region < ~0, -ENOMEM);

    spinlock_lock(&mm_lock);

    if (region >= ~0)
        goto exit;

    if (!is_initialized) {
        for (int i = 0; i < ARRAY_SIZE(mm_bucket); i++)
            list_init(&mm_bucket[i]);

        is_initialized = true;
    }

    buffer = (struct mm_buffer*) addr;
    buffer->region = region++;
    buffer->bucket = order;
    buffer->flags = flags;
    list_init(&buffer->list);

    list_add(&mm_bucket[order], &buffer->list);

exit:
    spinlock_unlock(&mm_lock);
    return 0;
}

static struct mm_buffer *find_buffer_in_bucket(int order, unsigned int flags)
{
    struct mm_buffer *buffer;

    if (list_is_empty(&mm_bucket[order]))
        return NULL;

    list_foreach(&mm_bucket[order], iter) {
        buffer = list_entry(iter, struct mm_buffer, list);
        if (buffer->flags == flags)
            return buffer;
    }

    return NULL;
}

static int fill_bucket(int order, unsigned int flags)
{
    struct mm_buffer *buffer1;
    struct mm_buffer *buffer2;
    int retval;

    if (order >= 15)
        return -ENOMEM;

    buffer1 = find_buffer_in_bucket(order + 1, flags);
    if (!buffer1) {
        retval = fill_bucket(order + 1, flags);
        if (retval)
            return retval;

        buffer1 = find_buffer_in_bucket(order + 1, flags);
        RET_IF_FAIL(buffer1, -ENOMEM);
    }

    list_del(&buffer1->list);

    buffer2 = (struct mm_buffer*) ((char*) buffer1 + order_to_size(order));
    buffer2->flags = buffer1->flags;
    buffer2->region = buffer1->region;
    list_init(&buffer2->list);

    buffer1->bucket = order;
    buffer2->bucket = order;

    list_add(&mm_bucket[order], &buffer1->list);
    list_add(&mm_bucket[order], &buffer2->list);

    return 0;
}

static void defragment(struct mm_buffer *buffer)
{
    struct mm_buffer *buffer_low;
    struct mm_buffer *buffer_high;

    struct mm_buffer *buffer2;
    struct mm_buffer *buffer3;

    RET_IF_FAIL(!list_is_empty(&mm_bucket[buffer->bucket]),);

    if (((unsigned long) buffer) & (1 << buffer->bucket)) {
        buffer_low = buffer2 = (struct mm_buffer*)
            ((char*) buffer - order_to_size(buffer->bucket));
        buffer_high = buffer;
    } else {
        buffer_low = buffer;
        buffer_high = buffer2 = (struct mm_buffer*)
            ((char*) buffer + order_to_size(buffer->bucket));
    }

    list_foreach(&mm_bucket[buffer->bucket], iter) {
        buffer3 = list_entry(iter, struct mm_buffer, list);
        if (buffer3 != buffer2)
            continue;

        if (buffer_low->region != buffer_high->region)
            return;

        list_del(&buffer_low->list);
        list_del(&buffer_high->list);

        buffer_low->bucket++;
        list_add(&mm_bucket[buffer_low->bucket], &buffer_low->list);

        defragment(buffer_low);

        return;
    }
}

static struct mm_buffer *get_buffer(int order, unsigned int flags)
{
    struct mm_buffer *buffer;
    int retval;

    buffer = find_buffer_in_bucket(order, flags);
    if (buffer)
        return buffer;

    retval = fill_bucket(order, flags);
    if (!retval) {
        buffer = find_buffer_in_bucket(order, flags);
        RET_IF_FAIL(buffer, NULL);
        return buffer;
    }

    if (flags & MM_DMA)
        return NULL;

    return get_buffer(order, MM_DMA);
}

void *kmalloc(size_t size, unsigned int flags)
{
    int order;
    struct mm_buffer *buffer;

    if (!size)
        return NULL;

    size += sizeof(*buffer);

    order = size_to_order(size);
    if (order < 0)
        return NULL;

    if (order > MAX_ADDRESSABLE_MEM_ORDER)
        return NULL;

    spinlock_lock(&mm_lock);

    buffer = get_buffer(order, flags);
    if (!buffer)
        goto error;

    list_del(&buffer->list);

    spinlock_unlock(&mm_lock);

    return (char*) buffer + sizeof(*buffer);

error:
    spinlock_unlock(&mm_lock);
    return NULL;
}

void kfree(void *ptr)
{
    struct mm_buffer *buffer;

    if (!ptr)
        return;

    buffer = (struct mm_buffer*) ((char *) ptr - sizeof(*buffer));
    RET_IF_FAIL(buffer->list.prev == buffer->list.next,);

    spinlock_lock(&mm_lock);

    list_add(&mm_bucket[buffer->bucket], &buffer->list);
    defragment(buffer);

    spinlock_unlock(&mm_lock);
}

void *page_alloc(unsigned int flags, int order)
{
    size_t size = ((1 << order) << PAGE_ORDER) - sizeof(struct mm_buffer);
    char *buffer = kmalloc(size, flags);
    return buffer - sizeof(struct mm_buffer);
}

void page_free(void *ptr, int order)
{
    struct mm_buffer *buffer = ptr;

    list_init(&buffer->list);
    buffer->bucket = size_to_order((1 << order) * PAGE_SIZE);

    kfree((char *) buffer + sizeof(*buffer));
}
