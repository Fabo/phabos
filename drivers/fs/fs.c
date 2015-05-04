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

static struct inode *root;
static struct hashtable fs_table;

void fs_init(void)
{
    hashtable_init_string(&fs_table);

    root = zalloc(sizeof(*root));
    RET_IF_FAIL(root,);

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

    if (pathname[0] == '\0')
        return cwd;

    while (pathname[i] != '/' && pathname[i] != '\0')
        i++;

    for (j = i; pathname[j] == '/'; j++)
        ;

    if (pathname[j] == '\0') {
        return cwd->fs->find(cwd, pathname);
    }

    pathname[i] = '\0';
    return _fs_walk(cwd->fs->find(cwd, pathname), pathname + j);
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
    node = fs_walk(cwd, path);
    free(path);

    return node;
}

int sys_mount(const char *source, const char *target, const char *filesystemtype,
              unsigned long mountflags, const void *data)
{
    struct fs *fs;
    struct inode *cwd = root;

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

    printf("mounting '%s' fs to '%s'\n", fs->name, target ? target : "/");
    cwd->mounted_inode = ; // FIXME moutn

    return 0;
}
DEFINE_SYSCALL(SYS_MOUNT, mount, 5);

int mount(const char *source, const char *target, const char *filesystemtype,
          unsigned long mountflags, const void *data)
{
    int retval =
        syscall(SYS_MOUNT, source, target, filesystemtype, mountflags, data);

    if (retval) {
        errno = -retval;
        return -1;
    }

    return 0;
}

static int fs_get_last_component_offset(const char *pathname)
{
    int i;

    RET_IF_FAIL(pathname, -EINVAL);

    i = strlen(pathname);
    if (i <= 0)
        return -EINVAL;

    while (i >= 0 && pathname[i] == '/')
        i--;
    if (i < 0)
        return i;

    while (i >= 0 && pathname[i] != '/')
        i--;

    return i;
}

#if 0
int mkdir(struct inode *cwd, const char *pathname, mode_t mode)
{
    int i;

    RET_IF_FAIL(pathname, -EINVAL);

    i = strlen(pathname);
    if (i == 0)
        return -EINVAL;

    while (i > 0 && pathname[i] == '/')
        i--;

    if (i <= 0)
        return -EEXIST;

    while (i > 0 && pathname[i] != '/')
        i--;

    if (i) {
        size_t length;
        char *path;

        length = strlen(pathname) + 1;
        path = malloc(length);

        RET_IF_FAIL(path, -ENOMEM);

        memcpy(path, pathname, length);
        cwd = fs_walk(cwd, path);
        free(path);
    }

    if (!cwd || !is_directory(cwd))
        return -ENOTDIR;

    cwd->fs->mkdir(cwd, pathname + 1);

    return 0;
}
#endif

int open(const char *pathname, int flags, ...)
{
    return -1;
}

#if 0
int fs_mount(struct inode *node, struct fs *fs)
{
    struct inode *cwd = node;

    RET_IF_FAIL(fs, -EINVAL);

    if (!cwd && root)
        return -EBUSY;

    if (!cwd) {
        root = malloc(sizeof(*root));
        memset(root, 0, sizeof(*root));

        RET_IF_FAIL(root, -ENOMEM);
        cwd = root;
    }

    RET_IF_FAIL(cwd->fs != NULL, -EBUSY);
#if 0 // FIXME
    RET_IF_FAIL(fs_is_directory(node), -EACCES);
    RET_IF_FAIL(directory_is_empty(node), -EACCES);
#endif

    cwd->fs = fs;

    return 0;
}

int fs_umount(struct inode *node)
{
    node->fs = NULL;

    if (node == root)
        root = NULL;

    return 0;
}

int fs_mkdir(struct inode *cwd, const char *pathname, mode_t mode)
{
    if (pathname[0] == '/')
        cwd = root;

    
}
struct inode *fs_walk(struct inode *cwd, const char *pathname)
{
    int i = 0;
    struct inode *dest = NULL;

    RET_IF_FAIL(cwd, NULL);
    RET_IF_FAIL(cwd->fs, NULL);
    RET_IF_FAIL(cwd->opendir, NULL);
    RET_IF_FAIL(cwd->readdir, NULL);
    RET_IF_FAIL(cwd->closedir, NULL);

    for (i = 0; pathname[i] != '/' && pathname[i] != '\0'; i++)
        ;

    struct dir *dir = cwd->fs->opendir();

    for (struct dirent *ent = cwd->fs->readdir(dir);
         ent;
         cwd->fs->readdir(dir)) {
        if (!strcmp(ent->filename, ))
    }

    if (dest)
        dest = walk(dest, pathname + i);

    cwd->fs->closedir();
    return dest;
}

int open(const char *pathname, int flags, ...)
{
    struct inode *inode;

    //inode = fs_find(pathname);

    if (!inode && !(flags & O_CREAT)) {
        errno = ENOENT;
        return -1;
    }

    if (inode && flags & O_CREAT && flags & O_EXCL) {
        errno = EEXIST;
        return -1;
    }

    return 0;
}
#endif
