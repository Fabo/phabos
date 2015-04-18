/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <phabos/i2c.h>
#include <phabos/bus.h>
#include <lib/utils.h>
#include <asm/spinlock.h>

#include <stdlib.h>
#include <string.h>

static struct list_head adapters = LIST_INIT(adapters);
static struct spinlock adapters_lock = SPINLOCK_INIT(adapters_lock);

int i2c_adapter_register(struct i2c_adapter *adapter)
{
    int retval;
    struct bus *bus;

    RET_IF_FAIL(adapter, -EINVAL);

    bus = malloc(sizeof(*bus));
    RET_IF_FAIL(bus, -ENOMEM);

    memset(bus, 0, sizeof(*bus));
    bus->class = DEVICE_CLASS_I2C;

    retval = bus_register(bus);
    GOTO_IF_FAIL(retval, err_bus_registration);

    retval = bus_add_device(bus, &adapter->dev);
    GOTO_IF_FAIL(!retval, err_bus_device_registration);

    retval = device_register(&adapter->dev);
    GOTO_IF_FAIL(!retval, err_device_registration);

    spinlock_lock(&adapters_lock);
    list_add(&adapters, &adapter->list);
    spinlock_unlock(&adapters_lock);

    adapter->dev.bus = bus;

    return 0;

err_device_registration:
    bus_unregister(bus);
err_bus_device_registration:
    bus_rm_device(&adapter->dev);
err_bus_registration:
    free(bus);

    return retval;
}

int i2c_adapter_unregister(struct i2c_adapter *adapter)
{
    RET_IF_FAIL(adapter, -EINVAL);

    spinlock_lock(&adapters_lock);
    list_del(&adapter->list);
    spinlock_unlock(&adapters_lock);

    device_unregister(&adapter->dev);
    bus_rm_device(&adapter->dev);
    bus_unregister(adapter->dev.bus);

    free(adapter->dev.bus);
    adapter->dev.bus = NULL;

    return 0;
}

struct i2c_adapter *i2c_adapter_get(unsigned int id)
{
    list_foreach(&adapters, iter) {
        if (id-- == 0)
            return containerof(iter, struct i2c_adapter, list);
    }

    return NULL;
}
