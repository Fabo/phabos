/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <stdio.h>
#include <assert.h>

#include <kernel/shell.h>
#include <kernel/scheduler.h>
#include <lib/utils.h>

static const char *task_state_to_string(uint16_t state)
{
    if (state & TASK_RUNNING)
        return "RUNNING";
    return "STOPPED";
}

int ps_main(int argc, char **argv)
{
    struct list_head *tasks;

    tasks = sched_get_task_list();
    assert(tasks);

    printf("  PID        STATE\n");
    list_foreach(tasks, iter) {
        struct task *task = containerof(iter, struct task, all);
        printf("%.5d %.12s\n", task->id, task_state_to_string(task->state));
        
    }

    return 0;
}

__shell_command__ struct shell_command ps_command = { "ps", "",
    ps_main
};
