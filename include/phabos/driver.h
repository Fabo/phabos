/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <lib/list.h>

struct device_driver;
struct bus;

struct fops {
    int (*open)(struct device_driver*);
    void (*close)(struct device_driver *dev);
};

struct device_driver {
    const char *name;
    const char *description;
    const char *class;
    struct bus *bus;

    void *priv;

    struct list_head list;
    struct list_head bus_node;
    struct fops ops;
};

struct driver {
    const char *name;
    void *priv;
    struct device_driver *dev;
    struct list_head list;

    int (*probe)(struct driver *driver);
    int (*remove)(struct driver *driver);
};

int driver_register(struct driver *driver);
int driver_unregister(struct driver *driver);
struct driver *driver_lookup(const char *name);

int device_register(struct device_driver *driver);
int device_unregister(struct device_driver *dev);
struct list_head *device_get_list(void);

#endif /* __DRIVER_H__ */

