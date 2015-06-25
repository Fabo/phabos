/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <phabos/gpio.h>
#include <phabos/assert.h>
#include <phabos/utils.h>
#include <asm/spinlock.h>
#include <errno.h>

static struct list_head devices = LIST_INIT(devices);
static struct spinlock dev_lock = SPINLOCK_INIT(dev_lock);

#if 0
struct gpio_device {
    unsigned base;
    size_t count;
};
#endif

int gpio_device_register(struct gpio_device *gpio)
{
    int retval;

    RET_IF_FAIL(gpio, -EINVAL);

    retval = device_register(&gpio->dev);
    RET_IF_FAIL(!retval, retval);

    spinlock_lock(&dev_lock);
    list_add(&devices, &gpio->list);
    spinlock_unlock(&dev_lock);

    return 0;
}

int gpio_device_unregister(struct gpio_device *gpio)
{
    RET_IF_FAIL(gpio, -EINVAL);

    spinlock_lock(&dev_lock);
    list_del(&gpio->list);
    spinlock_unlock(&dev_lock);

    return device_unregister(&gpio->dev);
}

struct gpio_device *gpio_device_get(unsigned int id)
{
    list_foreach(&devices, iter) {
        if (id-- == 0)
            return containerof(iter, struct gpio_device, list);
    }

    return NULL;
}
