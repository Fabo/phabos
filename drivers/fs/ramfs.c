#include <phabos/list.h>
#include <phabos/hashtable.h>
#include <phabos/utils.h>
#include <phabos/assert.h>
#include <phabos/fs.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define RAMFS_MAX_FILENAME_SIZE     255
#define RAMFS_DATA_BLOCK_SIZE       256

struct ramfs_data_block {
    struct list_head list;
    uint8_t data[RAMFS_DATA_BLOCK_SIZE];
};

struct ramfs_inode {
    char *name;
    struct hashtable files;
    struct hashtable blocks;
};

struct file_table {
    unsigned int flags;
    off_t offset;
    struct inode *inode;
};

static struct ramfs_inode *ramfs_create_inode(const char *name)
{
    struct ramfs_inode *inode;
    size_t name_length = MAX(RAMFS_MAX_FILENAME_SIZE, strlen(name)) + 1;

    inode = zalloc(sizeof(*inode));
    RET_IF_FAIL(inode, NULL);

    inode->name = zalloc(name_length);
    GOTO_IF_FAIL(inode->name, error_filename_alloc);

    memcpy(inode->name, name, name_length -1);
    inode->name[name_length - 1] = '\0';

    hashtable_init_uint(&inode->files);
    hashtable_init_string(&inode->files);

    return inode;

error_filename_alloc:
    free(inode);
    return NULL;
}

static int ramfs_creat(struct inode *cwd, const char *name, mode_t mode)
{
    struct ramfs_inode *cwd_inode = cwd->inode;
    struct ramfs_inode *inode;

    RET_IF_FAIL(cwd, -EINVAL);
    RET_IF_FAIL(name, -EINVAL);



    inode = ramfs_create_inode(inode->name);
    return 0;
}

static int ramfs_mkdir(struct inode *cwd, const char *name, mode_t mode)
{
    struct ramfs_inode *cwd_inode = cwd->inode;
    struct ramfs_inode *inode;

    RET_IF_FAIL(cwd, -EINVAL);
//    RET_IF_FAIL(cwd->flags & FS_DIRECTORY, -EINVAL);
    RET_IF_FAIL(name, -EINVAL);

    inode = ramfs_create_inode(inode->name);
    hashtable_add(&cwd_inode->files, inode->name, inode);

error_filename_alloc:
    free(inode);
    return 0;
}

#if 0
static ssize_t ramfs_read(struct file_table *ftable, void *buf, size_t count)
{
    RET_IF_FAIL(ftable, -EINVAL);
    RET_IF_FAIL(ftable->inode, -EINVAL);
    RET_IF_FAIL(ftable->inode->inode, -EINVAL);

    struct ramfs_inode *inode = ftable->inode->inode;
    struct ramfs_data_block *block;

    int block_id = ftable->offset / RAMFS_DATA_BLOCK_SIZE;
    off_t offset = ftable->offset % RAMFS_DATA_BLOCK_SIZE;
    size_t rcount;

    while (count) {
        size_t transfer_size = MIN(RAMFS_DATA_BLOCK_SIZE - offset, count);

        block = hashtable_get(&inode->blocks, (void*) block_id);
        if (block)
            memcpy(buf, &block->data, transfer_size);
        else
            memset(buf, 0, transfer_size);

        count -= transfer_size;
        rcount += transfer_size;
        ftable->offset += transfer_size;
        offset = 0;
        block_id++;
    }

    return rcount;
}

static ssize_t ramfs_write(struct file_table *ftable, const void *buf,
                           size_t count)
{
    RET_IF_FAIL(ftable, -EINVAL);
    RET_IF_FAIL(ftable->inode, -EINVAL);
    RET_IF_FAIL(ftable->inode->inode, -EINVAL);

    struct ramfs_inode *inode = ftable->inode->inode;
    struct ramfs_data_block *block;

    int block_id = ftable->offset / RAMFS_DATA_BLOCK_SIZE;
    off_t offset = ftable->offset % RAMFS_DATA_BLOCK_SIZE;
    size_t wcount;

    while (count) {
        size_t transfer_size = MIN(RAMFS_DATA_BLOCK_SIZE - offset, count);

        block = hashtable_get(&inode->blocks, (void*) block_id);
        if (!block) {
            block = zalloc(sizeof(*block));
            RET_IF_FAIL(block, wcount == 0 ? -ENOSPC : wcount);

            list_init(&block->list);
            list_add(&inode->blocks, &block->list);
        }

        memcpy(&block->data, buf, transfer_size);

        count -= transfer_size;
        wcount += transfer_size;
        ftable->offset += transfer_size;
        offset = 0;
        block_id++;
    }

    return wcount;
}
#endif

static struct inode *ramfs_find(struct inode *cwd, const char *name)
{
    struct ramfs_inode *cwd_inode = cwd->inode;
    struct ramfs_inode *inode;

    RET_IF_FAIL(cwd, -EINVAL);
    RET_IF_FAIL(name, -EINVAL);

    // TODO: check name size and truncate if necessary
    return hashtable_get(&inode->files, name);
}

#if 0
static something *ramfs_opendir(struct inode *cwd, const char *name)
{
    
}

static int ramfs_get(something *dir, struct dirent *entry, struct dirent **result)
{

}
#endif

struct fs ramfs_fs = {
    .name = "ramfs",
    .mkdir = ramfs_mkdir,
    .opendir = ramfs_opendir,
    .readdir = ramfs_readdir,
    .closedir = ramfs_closedir,
    .find = ramfs_find,
};

