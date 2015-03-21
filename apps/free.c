/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <stdio.h>

#include <kernel/shell.h>

uint32_t _sbrk(int incr);

int free_main(int argc, char **argv)
{
    extern uint32_t _sheap;
    extern uint32_t _eor;
    uint32_t heap_pos = _sbrk(0);

    printf("Memory:");
    printf("\ttotal: unknown\n");
    printf("\tused: unknown\n");
    printf("\tfree: %u\n", (uint32_t) &_eor - heap_pos);

    return 0;
}

__shell_command__ struct shell_command free_command = { "free", "",
    free_main
};
