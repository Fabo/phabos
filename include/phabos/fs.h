#ifndef __FS_H__
#define __FS_H__

#include <sys/stat.h>
#include <sys/types.h>
#include <phabos/assert.h>

struct inode;

struct fs {
    const char *name;

    int (*mkdir)(struct inode *cwd, const char *name, mode_t mode);

    struct inode *(*find)(struct inode *cwd, const char *name);
};

struct inode {
    struct fs *fs;
    void *inode;
    unsigned long flags;

    struct inode *mounted_inode;
};

struct file {
    struct inode *inode;
    unsigned long flags;
    off_t offset;
};

struct fd {
    unsigned long flags;
    struct file *file;
};

void fs_init(void);
int fs_register(struct fs *fs);

int mount(const char *source, const char *target, const char *filesystemtype,
          unsigned long mountflags, const void *data);

static inline bool is_directory(struct inode *inode)
{
    RET_IF_FAIL(inode, false);
    return inode->flags & S_IFDIR;
}

#endif /* __FS_H__ */

