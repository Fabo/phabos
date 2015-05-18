#ifndef __FS_H__
#define __FS_H__

#include <sys/stat.h>
#include <sys/types.h>
#include <phabos/assert.h>
#include <phabos/mutex.h>

struct inode;
struct file;
struct phabos_dirent;

struct fs {
    const char *name;

    int (*mount)(struct inode *cwd);
    int (*mkdir)(struct inode *cwd, const char *name, mode_t mode);

    int (*getdents)(struct file *file, struct phabos_dirent *dirp,
                    size_t count);

    struct inode *(*lookup)(struct inode *cwd, const char *name);
};

struct inode {
    struct fs *fs;
    void *inode;
    unsigned long flags;

    struct inode *mounted_inode;
    struct mutex dlock;
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

struct phabos_dirent {
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    char d_name[];
};

void fs_init(void);
int fs_register(struct fs *fs);

int open(const char *pathname, int flags, ...);
int close(int fd);
int mount(const char *source, const char *target, const char *filesystemtype,
          unsigned long mountflags, const void *data);
int mkdir(const char *pathname, mode_t mode);
int getdents(int fd, struct phabos_dirent *dirp, size_t count);

static inline bool is_directory(struct inode *inode)
{
    RET_IF_FAIL(inode, false);
    return inode->flags & S_IFDIR;
}

#endif /* __FS_H__ */

