#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <phabos/assert.h>
#include <phabos/hashtable.h>
#include <phabos/syscall.h>
#include <phabos/fs.h>
#include <phabos/utils.h>
#include <phabos/string.h>
#include <phabos/scheduler.h>

#define O_CLOEXEC 0x80000

static struct inode *root;
static struct hashtable fs_table;

void fs_init(void)
{
    hashtable_init_string(&fs_table);

    root = zalloc(sizeof(*root));
    RET_IF_FAIL(root,);

    mutex_init(&root->dlock);
    root->flags = S_IFDIR;
}

int fs_register(struct fs *fs)
{
    hashtable_add(&fs_table, (void*) fs->name, fs);
    return 0;
}

struct inode *_fs_walk(struct inode *cwd, char *pathname)
{
    int i = 0;
    int j;

    RET_IF_FAIL(cwd, NULL);
    RET_IF_FAIL(pathname, NULL);

    if (!is_directory(cwd))
        return NULL;

    while (pathname[i] == '/')
        i++;

    if (i) {
        pathname += i;
        cwd = root;
        i = 0;
    }

    if (cwd->mounted_inode) {
        cwd = cwd->mounted_inode;
    }

    if (pathname[0] == '\0')
        return cwd;

    while (pathname[i] != '/' && pathname[i] != '\0')
        i++;

    for (j = i; pathname[j] == '/'; j++)
        ;

    if (pathname[j] == '\0') {
        return cwd->fs->lookup(cwd, pathname);
    }

    pathname[i] = '\0';
    return _fs_walk(cwd->fs->lookup(cwd, pathname), pathname + j);
}

struct inode *fs_walk(struct inode *cwd, const char *pathname)
{
    size_t length;
    char *path;
    struct inode *node;

    RET_IF_FAIL(pathname, NULL);

    length = strlen(pathname) + 1;
    path = malloc(length);

    RET_IF_FAIL(path, NULL);

    memcpy(path, pathname, length);
    node = _fs_walk(cwd, path);
    free(path);

    return node;
}

int sys_mount(const char *source, const char *target, const char *filesystemtype,
              unsigned long mountflags, const void *data)
{
    struct fs *fs;
    struct inode *cwd = root;
    int retval;

    RET_IF_FAIL(root || !target, -EINVAL);

    fs = hashtable_get(&fs_table, (char*) filesystemtype);
    if (!fs)
        return -ENODEV;

    if (target)
        fs_walk(root, target);

    /* Cannot find target or target is not a directory */
    if (!cwd || !is_directory(cwd))
        return -ENOTDIR;

    /* Directory already mounted */
    if (cwd->mounted_inode)
        return -EBUSY;

    // TODO: check that the directory is empty

    kprintf("mounting '%s' fs to '%s'\n", fs->name, target ? target : "/");

    cwd->mounted_inode = zalloc(sizeof(*cwd->mounted_inode));
    if (!cwd->mounted_inode)
        return -ENOMEM;

    cwd->mounted_inode->fs = fs;
    cwd->mounted_inode->flags = S_IFDIR;
    mutex_init(&cwd->mounted_inode->dlock);

    retval = fs->mount(cwd->mounted_inode);
    if (retval)
        goto error_mount;

    return 0;

error_mount:
    free(cwd->mounted_inode);
    cwd->mounted_inode = NULL;

    return retval;
}
DEFINE_SYSCALL(SYS_MOUNT, mount, 5);

int mount(const char *source, const char *target, const char *filesystemtype,
          unsigned long mountflags, const void *data)
{
    long retval =
        syscall(SYS_MOUNT, source, target, filesystemtype, mountflags, data);

    if (retval) {
        errno = -retval;
        return -1;
    }

    return 0;
}

int sys_mkdir(const char *pathname, mode_t mode)
{
    struct inode *inode;
    char *name;
    char *path;
    int retval;

    if (!pathname)
        return -EINVAL;

    name = abasename(pathname);
    path = adirname(pathname);

    kprintf("creating directory '%s' in %s\n", name, path);

    inode = _fs_walk(root, path);
    if (!inode) {
        retval = -ENOENT;
        goto exit;
    }

    if (inode->mounted_inode) {
        inode = inode->mounted_inode;
    }

    RET_IF_FAIL(inode->fs, -EINVAL);
    RET_IF_FAIL(inode->fs->lookup, -EINVAL);
    RET_IF_FAIL(inode->fs->mkdir, -EINVAL);

    if (inode->fs->lookup(inode, name)) {
        retval = -EEXIST;
        goto exit;
    }

    retval = inode->fs->mkdir(inode, name, mode);

exit:
    free(name);
    free(path);

    return retval;
}
DEFINE_SYSCALL(SYS_MKDIR, mkdir, 5);

int mkdir(const char *pathname, mode_t mode)
{
    long retval = syscall(SYS_MKDIR, pathname, mode);

    if (retval) {
        errno = -retval;
        return -1;
    }

    return 0;
}

#define TASK_FD_MAX 20

static int allocate_fdnum(void)
{
    struct task *task;
    struct fd *fd;

    task = task_get_running();
    RET_IF_FAIL(task, -1);

    for (int i = 0; i < TASK_FD_MAX; i++) {
        fd = hashtable_get(&task->fd, (void*) i);
        if (fd)
            continue;

        fd = zalloc(sizeof(*fd));
        hashtable_add(&task->fd, (void*) i, fd);
        return i;
    }

    return -1;
}

static int free_fdnum(int fdnum)
{
    struct task *task;
    struct fd *fd;

    task = task_get_running();
    RET_IF_FAIL(task,-1);

    fd = hashtable_get(&task->fd, (void*) fdnum);
    if (!fd)
        return -EBADF;

    mutex_unlock(&fd->file->inode->dlock); // FIXME safety checks
    hashtable_remove(&task->fd, (void*) fdnum);

    free(fd->file);
    free(fd);

    return 0;
}

int sys_open(const char *pathname, int flags, mode_t mode)
{
    struct task *task;
    struct inode *inode;
    struct fd *fd;
    int fdnum;
    int retval;

    if (!pathname)
        return -EINVAL;

    kprintf("opening %s ...\n", pathname);

    inode = fs_walk(root, pathname);

    if (inode && flags & O_CREAT && flags & O_EXCL)
        return -EEXIST;

    fdnum = allocate_fdnum();
    if (fdnum < 0)
        return fdnum;

    task = task_get_running();
    RET_IF_FAIL(task, -1);

    fd = hashtable_get(&task->fd, (void*) fdnum);

    if (flags & O_CLOEXEC)
        fd->flags |= O_CLOEXEC;
    flags &= ~O_CLOEXEC;

    fd->file = zalloc(sizeof(*fd->file));
    if (!fd->file) {
        retval = -ENOMEM;
        goto error_file_alloc;
    }

    fd->file->inode = inode;
    fd->file->flags = flags;

    mutex_lock(&inode->dlock);

    return fdnum;

error_file_alloc:
    free_fdnum(fdnum);
    return retval;
}
DEFINE_SYSCALL(SYS_OPEN, open, 3);

int open(const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    long retval;
    va_list vl;

    if (flags & O_CREAT) {
        va_start(vl, flags);
        mode = va_arg(vl, typeof(mode));
        va_end(vl);
    }

    retval = syscall(SYS_OPEN, pathname, flags, mode);

    if (retval < 0) {
        errno = -retval;
        return -1;
    }

    return retval;
}

int sys_close(int fd)
{
    kprintf("closing %d ...\n", fd);
    return free_fdnum(fd);
}
DEFINE_SYSCALL(SYS_CLOSE, close, 1);

int close(int fd)
{
    long retval =
        syscall(SYS_CLOSE, fd);

    if (retval < 0) {
        errno = -retval;
        return -1;
    }

    return 0;
}

int sys_getdents(int fdnum, struct phabos_dirent *dirp, size_t count)
{
    struct task *task;
    struct fd *fd;

    if (fdnum < 0)
        return -EBADF;

    task = task_get_running();
    RET_IF_FAIL(task, -1);

    fd = hashtable_get(&task->fd, (void*) fdnum);
    if (!fd)
        return -EBADF;

    RET_IF_FAIL(fd->file, -EINVAL);
    RET_IF_FAIL(fd->file->inode, -EINVAL);

//    if (count < sizeof()
//        return -EINVAL;

    printf("reading directory entries: %s\n", __func__);

    RET_IF_FAIL(fd->file->inode, -EINVAL);
    RET_IF_FAIL(fd->file->inode->fs, -EINVAL);
    RET_IF_FAIL(fd->file->inode->fs->getdents, -EINVAL);
    return fd->file->inode->fs->getdents(fd->file, dirp, count);
}
DEFINE_SYSCALL(SYS_GETDENTS, getdents, 3);

int getdents(int fd, struct phabos_dirent *dirp, size_t count)
{
    long retval =
        syscall(SYS_GETDENTS, fd);

    if (retval < 0) {
        errno = -retval;
        return -1;
    }

    return 0;
}
