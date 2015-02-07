/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#ifndef __BUS_H__
#define __BUS_H__

#include <phabos/driver.h>
#include <lib/list.h>

struct bus {
    const char *class;

    struct list_head list;
    struct list_head devices;
};

int bus_register(struct bus *bus);
int bus_unregister(struct bus *bus);

int bus_add_device(struct bus *bus, struct device_driver *dev);
int bus_rm_device(struct device_driver *dev);

#endif /* __BUS_H__ */

