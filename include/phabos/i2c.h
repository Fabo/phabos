/*
 * Copyright (C) 2014-2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#ifndef __I2C_H__
#define __I2C_H__

#define I2C_READ    (1 << 1)

#include <assert.h>
#include <stdint.h>
#include <stddef.h>

struct i2c_dev;
struct i2c_msg;

struct i2c_ops {
    struct i2c_dev* (*init)(int port);
    void (*destroy)(struct i2c_dev *dev);
    int (*transfer)(struct i2c_dev *dev, struct i2c_msg *msg, size_t count);
    void (*irq)(int irq, void *context);
};

struct i2c_dev {
    unsigned long base;
    struct i2c_ops *ops;
};

struct i2c_msg {
    uint16_t addr;
    uint8_t *buffer;
    size_t length;
    unsigned long flags;
};

static inline void i2c_destroy(struct i2c_dev *dev)
{
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->destroy);
    dev->ops->destroy(dev);
}

static inline struct i2c_dev *i2c_initialize(int port)
{
    extern struct i2c_ops dev_i2c_ops;
    return dev_i2c_ops.init(0); // FIXME
}

static inline int i2c_transfer(struct i2c_dev *dev, struct i2c_msg *msg,
                               size_t count)
{
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->transfer);
    return dev->ops->transfer(dev, msg, count);
}

#endif /* __I2C_H__ */

