#ifndef __FS_H__
#define __FS_H__

#include <sys/types.h>
#include <phabos/assert.h>

#define S_IFREG     0x100000
#define S_IFDIR     0x040000

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

