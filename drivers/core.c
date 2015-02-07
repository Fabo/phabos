/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <errno.h>
#include <string.h>
#include <lib/assert.h>
#include <lib/utils.h>
#include <phabos/driver.h>

static struct list_head drivers = LIST_INIT(drivers);
static struct list_head devices = LIST_INIT(devices);

int device_register(struct device_driver *dev)
{
    RET_IF_FAIL(dev, -EINVAL);

    list_init(&dev->list);
    list_add(&devices, &dev->list);

    return 0;
}

int device_unregister(struct device_driver *dev)
{
    RET_IF_FAIL(dev, -EINVAL);

    list_del(&dev->list);

    return 0;
}

int driver_register(struct driver *driver)
{
    int retval;

    RET_IF_FAIL(driver, -EINVAL);

    retval = driver->probe(driver);
    RET_IF_FAIL(!retval, retval);

    list_init(&driver->list);
    list_add(&drivers, &driver->list);

    return 0;
}

int driver_unregister(struct driver *driver)
{
    RET_IF_FAIL(driver, -EINVAL);

    list_del(&driver->list);

    return driver->remove(driver);
}

struct driver *driver_lookup(const char *name)
{
    RET_IF_FAIL(name, NULL);

    list_foreach(&drivers, iter) {
        struct driver *driver = containerof(iter, struct driver, list);
        if (!strcmp(name, driver->name)) {
            return driver;
        }
    }

    return NULL;
}

struct list_head *device_get_list(void)
{
    return &devices;
}
