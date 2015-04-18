/*
 * Copyright (c) 2014-2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Fabien Parent <fparent@baylibre.com>
 *          Benoit Cousson <bcousson@baylibre.com>
 */

#ifndef __TSB_GPIO_H__
#define __TSB_GPIO_H__

#include <stdint.h>
#include <phabos/gpio.h>

struct tsb_gpio_dev {
    struct gpio_device dev;
};

#define TSB_IRQ_TYPE_LEVEL_LOW      0x0
#define TSB_IRQ_TYPE_LEVEL_HIGH     0x1
#define TSB_IRQ_TYPE_EDGE_FALLING   0x2
#define TSB_IRQ_TYPE_EDGE_RISING    0x3
#define TSB_IRQ_TYPE_EDGE_BOTH      0x7

int tsb_gpio_get_direction(struct gpio_device *dev, unsigned int line);
void tsb_gpio_direction_in(struct gpio_device *dev, unsigned int line);
void tsb_gpio_direction_out(struct gpio_device *dev, unsigned int line,
                            unsigned int value);
int tsb_gpio_get_value(struct gpio_device *dev, unsigned int line);
int tsb_gpio_set_value(struct gpio_device *dev, unsigned int line,
                        unsigned int value);
void tsb_gpio_activate(struct gpio_device *dev, unsigned int line);
void tsb_gpio_deactivate(struct gpio_device *dev, unsigned int line);
int tsb_gpio_set_debounce(struct gpio_device *dev, unsigned int line,
                          uint16_t delay);
int tsb_gpio_mask_irq(struct gpio_device *dev, unsigned int line);
int tsb_gpio_unmask_irq(struct gpio_device *dev, unsigned int line);
int tsb_gpio_clear_interrupt(struct gpio_device *dev, unsigned int line);
int tsb_gpio_irqattach(struct gpio_device *dev, unsigned int irq,
                       gpio_irq_handler_t isr);
int tsb_gpio_set_triggering(struct gpio_device *dev, unsigned int line,
                             int trigger);
uint32_t tsb_gpio_get_raw_interrupt(unsigned int line);
uint32_t tsb_gpio_get_interrupt(void);

void tsb_gpio_initialize(void);
void tsb_gpio_uninitialize(void);

#endif /* __TSB_GPIO_H__ */

