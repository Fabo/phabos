/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <errno.h>

#include <phabos/bus.h>
#include <lib/assert.h>

static struct list_head buses = LIST_INIT(buses);

int bus_register(struct bus *bus)
{
    RET_IF_FAIL(bus, -EINVAL);

    list_init(&bus->list);
    list_init(&bus->devices);
    list_add(&buses, &bus->list);

    return 0;
}

int bus_unregister(struct bus *bus)
{
    RET_IF_FAIL(bus, -EINVAL);

    list_del(&bus->list);

    return 0;
}

int bus_add_device(struct bus *bus, struct device_driver *dev)
{
    RET_IF_FAIL(bus, -EINVAL);
    RET_IF_FAIL(dev, -EINVAL);

    list_add(&bus->devices, &dev->bus_node);

    return 0;
}

int bus_rm_device(struct device_driver *dev)
{
    RET_IF_FAIL(dev, -EINVAL);

    list_del(&dev->bus_node);

    return 0;
}
