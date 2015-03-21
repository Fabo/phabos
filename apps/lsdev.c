/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string.h>

#include <kernel/shell.h>
#include <phabos/driver.h>
#include <phabos/gpio.h>
#include <lib/assert.h>
#include <lib/utils.h>

static void lsdev_gpio(struct device_driver *dev)
{
    struct gpio_device *gpio = containerof(dev, struct gpio_device, dev);

    assert(dev);
    assert(gpio->ops);

    printf("\t\t- %u lines\n", gpio->ops->line_count(gpio));
}

int lsdev_main(int argc, char **argv)
{
    struct list_head *head;

    head = device_get_list();
    RET_IF_FAIL(head, -1);

    printf("Devices:");
    list_foreach(head, iter) {
        struct device_driver *dev =
            containerof(iter, struct device_driver, list);

        printf("\t* %s: %s\n", dev->name, dev->description);
        if (dev->class && !strcmp(dev->class, "gpio")) {
            lsdev_gpio(dev);
        }
    }

    return 0;
}

__shell_command__ struct shell_command lsdev_command = { "lsdev", "",
    lsdev_main
};
