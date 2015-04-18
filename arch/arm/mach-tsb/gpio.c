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

#include <asm/hwio.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/spinlock.h>
#include <asm/tsb-irq.h>

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "gpio.h"

#define GPIO_BASE           0x40003000
#define GPIO_DATA           (GPIO_BASE)
#define GPIO_ODATA          (GPIO_BASE + 0x4)
#define GPIO_ODATASET       (GPIO_BASE + 0x8)
#define GPIO_ODATACLR       (GPIO_BASE + 0xc)
#define GPIO_DIR            (GPIO_BASE + 0x10)
#define GPIO_DIROUT         (GPIO_BASE + 0x14)
#define GPIO_DIRIN          (GPIO_BASE + 0x18)
#define GPIO_INTMASK        (GPIO_BASE + 0x1c)
#define GPIO_INTMASKSET     (GPIO_BASE + 0x20)
#define GPIO_INTMASKCLR     (GPIO_BASE + 0x24)
#define GPIO_RAWINTSTAT     (GPIO_BASE + 0x28)
#define GPIO_INTSTAT        (GPIO_BASE + 0x2c)
#define GPIO_INTCTRL0       (GPIO_BASE + 0x30)
#define GPIO_INTCTRL1       (GPIO_BASE + 0x34)
#define GPIO_INTCTRL2       (GPIO_BASE + 0x38)
#define GPIO_INTCTRL3       (GPIO_BASE + 0x3c)

#define NR_GPIO_IRQS 16



/* A table of handlers for each GPIO interrupt */
static gpio_irq_handler_t gpio_irq_vector[NR_GPIO_IRQS];
static volatile uint32_t refcount;
static struct spinlock gpio_lock = SPINLOCK_INIT(gpio_lock);

static void tsb_gpio_unexpected_irq(int pin)
{
}

int tsb_gpio_get_direction(struct gpio_device *dev, unsigned int line)
{
    uint32_t dir = read32(GPIO_DIR);
    return !(dir & (1 << line));
}

void tsb_gpio_direction_in(struct gpio_device *dev, unsigned int line)
{
    write32(GPIO_DIRIN, 1 << line);
}

void tsb_gpio_direction_out(struct gpio_device *dev, unsigned int line,
                            unsigned int value)
{
    tsb_gpio_set_value(dev, line, value);
    write32(GPIO_DIROUT, 1 << line);
}

void tsb_gpio_activate(struct gpio_device *dev, unsigned int line)
{
    tsb_gpio_initialize();
}

void tsb_gpio_deactivate(struct gpio_device *dev, unsigned int line)
{
    tsb_gpio_uninitialize();
}

int tsb_gpio_get_value(struct gpio_device *dev, unsigned int line)
{
    return !!(read32(GPIO_DATA) & (1 << line));
}

int tsb_gpio_set_value(struct gpio_device *dev, unsigned int line,
                       unsigned int value)
{
    write32(value ? GPIO_ODATASET : GPIO_ODATACLR, 1 << line);
    return 0;
}

int tsb_gpio_set_debounce(struct gpio_device *dev, unsigned int line,
                          uint16_t delay)
{
    uint8_t initial_reading;

    initial_reading = tsb_gpio_get_value(dev, line);
    udelay(delay);
    return initial_reading == tsb_gpio_get_value(dev, line);
}

unsigned int tsb_gpio_line_count(struct gpio_device *dev)
{
    return NR_GPIO_IRQS;
}

int tsb_gpio_mask_irq(struct gpio_device *dev, unsigned int line)
{
    write32(GPIO_INTMASKSET, 1 << line);
    return 0;
}

int tsb_gpio_unmask_irq(struct gpio_device *dev, unsigned int line)
{
    write32(GPIO_INTMASKCLR, 1 << line);
    return 0;
}

int tsb_gpio_clear_interrupt(struct gpio_device *dev, unsigned int line)
{
    write32(GPIO_RAWINTSTAT, 1 << line);
    return 0;
}

uint32_t tsb_gpio_get_raw_interrupt(unsigned int line)
{
    return read32(GPIO_RAWINTSTAT);
}

uint32_t tsb_gpio_get_interrupt(void)
{
    return read32(GPIO_INTSTAT);
}

int tsb_gpio_set_triggering(struct gpio_device *dev, unsigned int line,
                            int trigger)
{
    uint32_t reg = GPIO_INTCTRL0 + ((line >> 1) & 0xfc);
    uint32_t shift = 4 * (line & 0x7);

    switch(trigger) {
    case IRQ_TYPE_EDGE_RISING:
        trigger = TSB_IRQ_TYPE_EDGE_RISING;
        break;
    case IRQ_TYPE_EDGE_FALLING:
        trigger = TSB_IRQ_TYPE_EDGE_FALLING;
        break;
    case IRQ_TYPE_EDGE_BOTH:
        trigger = TSB_IRQ_TYPE_EDGE_BOTH;
        break;
    case IRQ_TYPE_LEVEL_HIGH:
        trigger = TSB_IRQ_TYPE_LEVEL_HIGH;
        break;
    case IRQ_TYPE_LEVEL_LOW:
        trigger = TSB_IRQ_TYPE_LEVEL_LOW;
        break;
    default:
        return -EINVAL;
    }

    read32(reg) |= trigger << shift;
    return 0;
}

static void tsb_gpio_irq_handler(int irq, void *data)
{
    /*
     * Handle each pending GPIO interrupt.
     *     "The GPIO MIS register is the masked interrupt status register.
     *      Bits read High in GPIO MIS reflect the status of input lines
     *      triggering an interrupt. Bits read as Low indicate that either no
     *      interrupt has been generated, or the interrupt is masked."
     */
    uint32_t irqstat = tsb_gpio_get_interrupt();
    int pin;

    /*
     * Clear all GPIO interrupts that we are going to process.
     *     "The GPIO_RAWINTSTAT register is the interrupt clear register.
     *      Writing a 1 to a bit in this register clears the corresponding
     *      interrupt edge detection logic register. Writing a 0 has no effect."
     */
    write32(GPIO_RAWINTSTAT, irqstat);

    /* Now process each IRQ pending in the GPIO */
    for (pin = 0; pin < NR_GPIO_IRQS && irqstat != 0; pin++, irqstat >>= 1) {
        if ((irqstat & 1) != 0) {
            gpio_irq_vector[pin](pin);
        }
    }
}

int tsb_gpio_irqattach(struct gpio_device *dev, unsigned int irq,
                       gpio_irq_handler_t isr)
{
    if (irq >= NR_GPIO_IRQS)
        return -EINVAL;

    spinlock_lock(&gpio_lock);

    /*
     * If the new ISR is NULL, then the ISR is being detached.
     * In this case, disable the ISR and direct any interrupts
     * to the unexpected interrupt handler.
     */
    if (isr == NULL) {
        isr = tsb_gpio_unexpected_irq;
    }

    /* Save the new ISR in the table. */
    gpio_irq_vector[irq] = isr;
    spinlock_unlock(&gpio_lock);

    return 0;
}

static void tsb_gpio_irqinitialize(void)
{
    /* Point all interrupt vectors to the unexpected interrupt */
    for (int i = 0; i < NR_GPIO_IRQS; i++) {
        gpio_irq_vector[i] = tsb_gpio_unexpected_irq;
    }
}

void tsb_gpio_initialize(void)
{
    spinlock_lock(&gpio_lock);
    refcount++;

    tsb_clk_enable(TSB_CLK_GPIO);
    tsb_reset(TSB_RST_GPIO);

    tsb_gpio_irqinitialize();

    /* Attach Interrupt Handler */
    irq_attach(TSB_IRQ_GPIO, tsb_gpio_irq_handler, NULL);

    /* Enable Interrupt Handler */
    irq_enable_line(TSB_IRQ_GPIO);

    spinlock_unlock(&gpio_lock);
}

void tsb_gpio_uninitialize(void)
{
    spinlock_lock(&gpio_lock);
    refcount--;
    if (refcount > 0) {
        spinlock_unlock(&gpio_lock);
        return;
    }

    tsb_clk_disable(TSB_CLK_GPIO);

    /* Detach Interrupt Handler */
    irq_detach(TSB_IRQ_GPIO);

    spinlock_unlock(&gpio_lock);
}

static struct gpio_ops gpio_ops;

struct tsb_gpio_dev *tsb_gpio_init(void)
{
    struct tsb_gpio_dev *tsb_gpio;

    tsb_gpio = malloc(sizeof(*tsb_gpio));
    memset(tsb_gpio, 0, sizeof(*tsb_gpio));

    tsb_gpio->dev.dev.name = "tsb-gpio";
    tsb_gpio->dev.dev.description = "GPIO controller";
    tsb_gpio->dev.dev.class = DEVICE_CLASS_GPIO;
    tsb_gpio->dev.ops = &gpio_ops;

    gpio_device_register(&tsb_gpio->dev);

    return tsb_gpio;
}

void tsb_gpio_exit(struct tsb_gpio_dev *tsb_gpio)
{
    gpio_device_unregister(&tsb_gpio->dev);
    free(tsb_gpio);
}

static struct gpio_ops gpio_ops = {
    .get_direction = tsb_gpio_get_direction,
    .direction_out = tsb_gpio_direction_out,
    .direction_in = tsb_gpio_direction_in,

    .get_value = tsb_gpio_get_value,
    .set_value = tsb_gpio_set_value,
    .set_debounce = tsb_gpio_set_debounce,

    .activate = tsb_gpio_activate,
    .deactivate = tsb_gpio_deactivate,

    .irqattach = tsb_gpio_irqattach,
    .set_triggering = tsb_gpio_set_triggering,
    .mask_irq = tsb_gpio_mask_irq,
    .unmask_irq = tsb_gpio_unmask_irq,
    .clear_interrupt= tsb_gpio_clear_interrupt,

    .line_count = tsb_gpio_line_count,
};
