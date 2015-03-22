/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <stdio.h>
#include <stdint.h>

#include <kernel/shell.h>

int i2c_main(int argc, char **argv)
{
    return 0;
}

__shell_command__ struct shell_command i2c_command = { "i2c", "",
    i2c_main
};
