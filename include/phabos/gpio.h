/*
 * Copyright (C) 2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include <phabos/driver.h>
#include <lib/list.h>

#define DEVICE_CLASS_GPIO "gpio"

#define IRQ_TYPE_NONE           0x00000000
#define IRQ_TYPE_EDGE_RISING    0x00000001
#define IRQ_TYPE_EDGE_FALLING   0x00000002
#define IRQ_TYPE_EDGE_BOTH      (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH     0x00000004
#define IRQ_TYPE_LEVEL_LOW      0x00000008

struct gpio_device;

typedef void (*gpio_irq_handler_t)(int pin);

struct gpio_ops {
    int (*get_direction)(struct gpio_device *dev, unsigned int line);
    void (*direction_in)(struct gpio_device *dev, unsigned int line);
    void (*direction_out)(struct gpio_device *dev, unsigned int line,
                          unsigned int value);

    void (*activate)(struct gpio_device *dev, unsigned int line);
    void (*deactivate)(struct gpio_device *dev, unsigned int line);

    int (*get_value)(struct gpio_device *dev, unsigned int line);
    int (*set_value)(struct gpio_device *dev, unsigned int line,
                     unsigned int value);
    int (*set_debounce)(struct gpio_device *dev, unsigned int line,
                        uint16_t delay);

    int (*irqattach)(struct gpio_device *dev, unsigned int line,
                     gpio_irq_handler_t handler);
    int (*set_triggering)(struct gpio_device *dev, unsigned int line,
                          int trigger);
    int (*mask_irq)(struct gpio_device *dev, unsigned int line);
    int (*unmask_irq)(struct gpio_device *dev, unsigned int line);
    int (*clear_interrupt)(struct gpio_device *dev, unsigned int line);

    unsigned int (*line_count)(struct gpio_device *dev);
};

struct gpio_device {
    struct device_driver dev;
    struct gpio_ops *ops;
    struct list_head list;
    void *priv;
};

int gpio_device_register(struct gpio_device *gpio);
int gpio_device_unregister(struct gpio_device *gpio);

struct gpio_device *gpio_device_get(unsigned int id);

#endif /* __GPIO_H__ */

