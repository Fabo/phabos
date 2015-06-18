#include <phabos/scheduler.h>
#include <phabos/syscall.h>
#include <phabos/fs.h>
#include <phabos/utils.h>

#include <fcntl.h>
#include <errno.h>
#include <spawn.h>

static int load_executable(const char *path)
{
    int fd;
    int retval;

    fd = sys_open(path, O_RDONLY, 0);
    if (fd < 0) {
        retval = fd;
        goto exit;
    }

    retval = elf_exec(fd);
    if (!retval)
        goto exit;

    retval = -ENOEXEC;

exit:
    sys_close(fd);
    return retval;
}

int sys_spawn(pid_t *restrict pid, const char *restrict path,
              const posix_spawn_file_actions_t *file_actions,
              const posix_spawnattr_t *restrict attrp,
              char *const argv[restrict], char *const envp[restrict])
{
    int retval = 0;

    struct task *task = task_create();
    if (!task)
        return -ENOMEM;

#if 0
    task->allocated_stack = zalloc(DEFAULT_STACK_SIZE);
    if (!task->allocated_stack) {
        retval = -ENOMEM;
        goto error_stack;
    }

    stack_addr = (uint32_t) task->allocated_stack + DEFAULT_STACK_SIZE;
#endif

    retval = load_executable(path);
    if (retval)
        goto error_load_executable;

    //task_init_registers(task, entry, data, stack_addr);
    task->state = TASK_RUNNING;

    //scheduler_add_to_runqueue(task);

    if (pid)
        *pid = task->id;

    return 0;

error_load_executable:
    kfree(task->allocated_stack);
error_stack:
    kfree(task);

    return retval;
}
DEFINE_SYSCALL(SYS_SPAWN, spawn, 6);

int posix_spawn(pid_t *restrict pid, const char *restrict path,
              const posix_spawn_file_actions_t *file_actions,
              const posix_spawnattr_t *restrict attrp,
              char *const argv[restrict], char *const envp[restrict])
{
    long retval = syscall(SYS_SPAWN, pid, path, file_actions,
                          attrp, argv, envp);
    if (retval < 0) {
        errno = -retval;
        return -1;
    }

    return retval;
}
