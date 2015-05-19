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

#include <asm/gpio.h>
#include <asm/hwio.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/spinlock.h>
#include <asm/tsb-irq.h>

#include <stdint.h>
#include <errno.h>

#include "scm.h"

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

static void gpio_unexpected_irq(int pin)
{
}

int gpio_get_direction(uint8_t which)
{
    uint32_t dir = read32(GPIO_DIR);
    return !(dir & (1 << which));
}

void gpio_direction_in(uint8_t which)
{
    write32(GPIO_DIRIN, 1 << which);
}

void gpio_direction_out(uint8_t which, uint8_t value)
{
    gpio_set_value(which, value);
    write32(GPIO_DIROUT, 1 << which);
}

void gpio_activate(uint8_t which)
{
    gpio_initialize();
}

uint8_t gpio_get_value(uint8_t which)
{
    return !!(read32(GPIO_DATA) & (1 << which));
}

void gpio_set_value(uint8_t which, uint8_t value)
{
    write32(value ? GPIO_ODATASET : GPIO_ODATACLR, 1 << which);
}

int gpio_set_debounce(uint8_t which, uint16_t delay)
{
    uint8_t initial_reading;

    initial_reading = gpio_get_value(which);
    udelay(delay);
    return initial_reading == gpio_get_value(which);
}

void gpio_deactivate(uint8_t which)
{
    gpio_uninitialize();
}

uint8_t gpio_line_count(void)
{
    return NR_GPIO_IRQS;
}

void gpio_mask_irq(uint8_t which)
{
    write32(GPIO_INTMASKSET, 1 << which);
}

void gpio_unmask_irq(uint8_t which)
{
    write32(GPIO_INTMASKCLR, 1 << which);
}

void gpio_clear_interrupt(uint8_t which)
{
    write32(GPIO_RAWINTSTAT, 1 << which);
}

uint32_t gpio_get_raw_interrupt(uint8_t which)
{
    return read32(GPIO_RAWINTSTAT);
}

uint32_t gpio_get_interrupt(void)
{
    return read32(GPIO_INTSTAT);
}

void set_gpio_triggering(uint8_t which, int trigger)
{
    uint32_t reg = GPIO_INTCTRL0 + ((which >> 1) & 0xfc);
    uint32_t shift = 4 * (which & 0x7);

    read32(reg) |= trigger << shift;
}

static void gpio_irq_handler(int irq, void *data)
{
    /*
     * Handle each pending GPIO interrupt.
     *     "The GPIO MIS register is the masked interrupt status register.
     *      Bits read High in GPIO MIS reflect the status of input lines
     *      triggering an interrupt. Bits read as Low indicate that either no
     *      interrupt has been generated, or the interrupt is masked."
     */
    uint32_t irqstat = gpio_get_interrupt();
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

int gpio_irqattach(int irq, gpio_irq_handler_t isr)
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
        isr = gpio_unexpected_irq;
    }

    /* Save the new ISR in the table. */
    gpio_irq_vector[irq] = isr;
    spinlock_unlock(&gpio_lock);

    return 0;
}

static void gpio_irqinitialize(void)
{
    /* Point all interrupt vectors to the unexpected interrupt */
    for (int i = 0; i < NR_GPIO_IRQS; i++) {
        gpio_irq_vector[i] = gpio_unexpected_irq;
    }
}

void gpio_initialize(void)
{
    spinlock_lock(&gpio_lock);
    refcount++;

    tsb_clk_enable(TSB_CLK_GPIO);
    tsb_reset(TSB_RST_GPIO);

    gpio_irqinitialize();

    /* Attach Interrupt Handler */
    irq_attach(TSB_IRQ_GPIO, gpio_irq_handler, NULL);

    /* Enable Interrupt Handler */
    irq_enable_line(TSB_IRQ_GPIO);

    spinlock_unlock(&gpio_lock);
}

void gpio_uninitialize(void)
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
