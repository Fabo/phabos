/*
 * Copyright (C) 2014-2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <config.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <phabos/kprintf.h>
#include <phabos/scheduler.h>
#include <phabos/syscall.h>
#include <phabos/fs.h>

#include <asm/hwio.h>

int CONFIG_INIT_TASK_NAME(int argc, char **argv);

#define xstr(s) str(s)
#define str(s) #s

void init(void *data)
{
    char* argv[] = {
        xstr(CONFIG_INIT_TASK_NAME),
        NULL
    };

#if 1

    {
        int fd;
        int retval;
        struct phabos_dirent dirent;

        fs_init();

        extern struct fs ramfs_fs;
        fs_register(&ramfs_fs);

        retval = mount(NULL, NULL, "ramfs", 0, NULL);
        if (retval < 0)
            printf("failed to mount the ramfs: %s\n", strerror(errno));

#if 1
        retval = mkdir("/test/", 0);
        if (retval < 0)
            printf("mkdir failed: %s\n", strerror(errno));

        retval = mkdir("/toto/", 0);
        if (retval < 0)
            printf("mkdir failed: %s\n", strerror(errno));

        retval = mkdir("/test/tata", 0);
        if (retval < 0)
            printf("mkdir failed: %s\n", strerror(errno));

        retval = mkdir("/test/tata/tutu", 0);
        if (retval < 0)
            printf("mkdir failed: %s\n", strerror(errno));

        retval = mkdir("/toto/tata/tutu", 0);
        if (retval < 0)
            printf("mkdir failed: %s\n", strerror(errno));

        fd = open("/", 0);
        if (fd < 0)
            printf("open failed: %s\n", strerror(errno));
        else
            printf("allocated fd: %d\n", fd);

        do {
            retval = getdents(fd, &dirent, 1);
            if (retval < 0)
                printf("getdents failed: %s\n", strerror(errno));

            if (retval <= 0)
                break;

            //printf("");
        } while (retval > 0);

        fd = close(fd);
        if (fd < 0)
            printf("close failed: %s\n", strerror(errno));
        else
            printf("closed fd\n");
#endif
    }
#endif

    CONFIG_INIT_TASK_NAME(1, argv);

    while (1);
}

#include <asm/delay.h>

static inline void go_to_user_mode(void)
{
    asm volatile (
        "push {r0}\n"
        "mrs r0, control\n"
        "orr r0, #1\n"
        "msr control, r0\n"
        "pop {r0}\n"
    );
}

void task1(void *data)
{
    int retval;

    go_to_user_mode();
    fs_init();

    extern struct fs ramfs_fs;
    fs_register(&ramfs_fs);

    kprintf("%s\n", __func__);

    retval = mount(NULL, NULL, "ramfs", 0, NULL);
    if (retval < 0)
        printf("failed to mount the ramfs: %s\n", strerror(errno));

    while (1) {
        kprintf("%s\n", __func__);

        udelay(10000);
    }
}

void task2(void *data)
{
    go_to_user_mode();

    while (1) {
        kprintf("%s\n", __func__);

        udelay(10000);
    }
}

static void clear_screen(void)
{
    kprintf("\r%c[2J",27);
}

void main(void)
{
    clear_screen();
    kprintf("booting phabos...\n");

    syscall_init();
    scheduler_init();
    task_run(init, NULL, 0);
//    task_run(task1, NULL, 0);
//    task_run(task2, NULL, 0);
}
