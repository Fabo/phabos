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
        int retval;
        fs_init();

        extern struct fs ramfs_fs;
        fs_register(&ramfs_fs);

        retval = mount(NULL, NULL, "ramfs", 0, NULL);
        if (retval < 0)
            printf("failed to mount the ramfs: %s\n", strerror(errno));

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

        retval = open("/", 0);
        if (retval < 0)
            printf("open failed: %s\n", strerror(errno));
        else
            printf("allocated fd: %d\n", retval);

        retval = open("/test", 0);
        if (retval < 0)
            printf("open failed: %s\n", strerror(errno));
        else
            printf("allocated fd: %d\n", retval);

        retval = close(retval);
        if (retval < 0)
            printf("close failed: %s\n", strerror(errno));
        else
            printf("closed fd\n");
    }
#endif

    CONFIG_INIT_TASK_NAME(1, argv);

    while (1);
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
}
