/*
 * Copyright (C) 2014-2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#ifndef __I2C_H__
#define __I2C_H__

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#include <phabos/driver.h>
#include <lib/assert.h>
#include <lib/list.h>

#define DEVICE_CLASS_I2C "i2c"
#define I2C_READ    (1 << 1)

struct i2c_adapter;
struct i2c_msg;

struct i2c_dev {
    struct device_driver dev;
};

struct i2c_adapter {
    struct device_driver dev;
    struct list_head list;

    int (*transfer)(struct i2c_adapter *adapter,
                    struct i2c_msg *msg, size_t count);
    void (*irq)(int irq, void *context);
};

struct i2c_msg {
    uint16_t addr;
    uint8_t *buffer;
    size_t length;
    unsigned long flags;
};

int i2c_adapter_register(struct i2c_adapter *adapter);
int i2c_adapter_unregister(struct i2c_adapter *adapter);

struct i2c_adapter *i2c_adapter_get(unsigned int id);

static inline int i2c_open(struct i2c_adapter *adapter)
{
    int retval;

    RET_IF_FAIL(adapter, -EINVAL);
    RET_IF_FAIL(adapter->dev.ops.open, -EINVAL);

    retval = adapter->dev.ops.open(&adapter->dev);
    return retval;
}

static inline void i2c_close(struct i2c_adapter *adapter)
{
    RET_IF_FAIL(adapter,);
    RET_IF_FAIL(adapter->dev.ops.close,);

    adapter->dev.ops.close(&adapter->dev);
}

static inline int i2c_transfer(struct i2c_adapter *adapter, struct i2c_msg *msg,
                               size_t count)
{
    assert(adapter);
    assert(adapter->transfer);
    return adapter->transfer(adapter, msg, count);
}

#endif /* __I2C_H__ */

