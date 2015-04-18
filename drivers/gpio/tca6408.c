/**
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
 * @brief TCA6408 GPIO Expander Driver
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <phabos/bus.h>
#include <phabos/gpio.h>
#include <phabos/gpio/tca6408.h>
#include <phabos/i2c.h>

#include <lib/utils.h>
#include <asm/gpio.h>

#define TCA6408_NR_GPIO 8

#define TCA6408_INPUT_REG       0x00
#define TCA6408_OUTPUT_REG      0x01
#define TCA6408_POLARITY_REG    0x02
#define TCA6408_CONFIG_REG      0x03

/* disable the verbose debug output */
#undef lldbg
#define lldbg(x...)

static int i2c_get(struct gpio_device *device, uint8_t regaddr, uint8_t *val)
{
    int ret;
    struct tca6408_dev *tca6408 = containerof(device, struct tca6408_dev, dev);
    uint8_t addr = (uint8_t) device->priv;
    struct i2c_msg msg[] = {
        {
            .addr = addr,
            .flags = 0,
            .buffer = &regaddr,
            .length = 1,
        }, {
            .addr = addr,
            .flags = I2C_READ,
            .buffer = val,
            .length = 1,
        },
    };

    RET_IF_FAIL(device, -EINVAL);
    RET_IF_FAIL(tca6408->adapter, -EINVAL);

    ret = i2c_transfer(tca6408->adapter, msg, 2);

    if (ret == 0) {
        lldbg("addr=0x%02hhX, regaddr=0x%02hhX: read 0x%02hhX\n",
              addr, regaddr, *val);
    } else {
        lldbg("addr=0x%02hhX, regaddr=0x%02hhX: failed!\n", addr, regaddr);
    }

    return ret;
}

static int i2c_set(struct gpio_device *device, uint8_t regaddr, uint8_t val)
{
    struct tca6408_dev *tca6408 = containerof(device, struct tca6408_dev, dev);
    uint8_t cmd[2] = {regaddr, val};
    uint8_t data8;
    uint8_t addr = (uint8_t) device->priv;
    struct i2c_msg msg[] = {
        {
            .addr = addr,
            .flags = 0,
            .buffer = cmd,
            .length = 2,
        }, {
            .addr = addr,
            .flags = I2C_READ,
            .buffer = &data8,
            .length = 1,
        },
    };

    lldbg("addr=0x%02hhX: regaddr=0x%02hhX, val=0x%02hhX\n",
          addr, regaddr, val);

    RET_IF_FAIL(device, -EINVAL);
    RET_IF_FAIL(tca6408->adapter, -EINVAL);

    return i2c_transfer(tca6408->adapter, msg, 2);
}

void tca6408_set_direction_in(struct gpio_device *device, unsigned int which)
{
    uint8_t reg;
    int ret;

    /* Configure pin as input
     *
     * The Configuration Register (register 3) configures the direction of
     * the I/O pins. If a bit in this register is set to 1,
     * the corresponding port pin is enabled as an input with a
     * high-impedance output driver. If a bit in this register is
     * cleared to 0, the corresponding port pin is enabled as an output.
     */
    ret = i2c_get(device, TCA6408_CONFIG_REG, &reg);
    if (ret != 0)
        return;
    lldbg("current cfg=0x%02X\n", reg);
    reg |= (1 << which);
    lldbg("new cfg=0x%02X\n", reg);
    ret = i2c_set(device, TCA6408_CONFIG_REG, reg);
    if (ret != 0)
        return;
}

int tca6408_set_default_outputs(struct gpio_device *device, uint8_t dflt)
{
    /* Set output pins default value (before configuring it as output
     *
     * The Output Port Register (register 1) shows the outgoing logic
     * levels of the pins defined as outputs by the Configuration Register.
     */
    return i2c_set(device, TCA6408_OUTPUT_REG, dflt);
}

void tca6408_set_direction_out(struct gpio_device *device, unsigned int which,
                               unsigned int value)
{
    uint8_t reg;
    int ret;

    /* Configure pin as output
     *
     * The Configuration Register (register 3) configures the direction of
     * the I/O pins. If a bit in this register is set to 1,
     * the corresponding port pin is enabled as an input with a
     * high-impedance output driver. If a bit in this register is
     * cleared to 0, the corresponding port pin is enabled as an output.
     */
    ret = i2c_get(device, TCA6408_CONFIG_REG, &reg);
    if (ret != 0)
        return;
    lldbg("current cfg=0x%02X\n", reg);
    reg &= ~(1 << which);
    lldbg("new cfg=0x%02X\n", reg);
    ret = i2c_set(device, TCA6408_CONFIG_REG, reg);
    if (ret != 0)
        return;

    tca6408_set(device, which, value);
}

int tca6408_get_direction(struct gpio_device *device, unsigned int which)
{
    uint8_t direction;
    int ret;

    /*
     * The Configuration Register (register 3) configures the direction of
     * the I/O pins. If a bit in this register is set to 1,
     * the corresponding port pin is enabled as an input with a
     * high-impedance output driver. If a bit in this register is
     * cleared to 0, the corresponding port pin is enabled as an output.
     */
    ret = i2c_get(device, TCA6408_CONFIG_REG, &direction);
    if (ret != 0)
        return -EIO;
    return (direction & (1 << which)) >> which;
}


int tca6408_set_polarity_inverted(struct gpio_device *device, unsigned int which,
                                  uint8_t inverted)
{
    uint8_t polarity;
    int ret;

    /* Configure pin polarity inversion
     *
     * The Polarity Inversion Register (register 2) allows
     * polarity inversion of pins defined as inputs by the Configuration
     * Register. If a bit in this register is set (written with 1),
     * the corresponding port pin's polarity is inverted. If a bit in
     * this register is cleared (written with a 0), the corresponding
     * port pin's original polarity is retained.
     */
    ret = i2c_get(device, TCA6408_POLARITY_REG, &polarity);
    if (ret != 0)
        return -EIO;
    lldbg("current polarity reg=0x%02hhX\n", polarity);
    if (inverted) {
        polarity |= (1 << which);
    } else {
        polarity &= ~(1 << which);
    }
    lldbg("new polarity reg=0x%02hhX\n", polarity);
    return i2c_set(device, TCA6408_POLARITY_REG, polarity);
}


int tca6408_get_polarity_inverted(struct gpio_device *device, unsigned int which)
{
    uint8_t polarity;
    int ret;

    /*
     * The Configuration Register (register 3) configures the direction of
     * the I/O pins. If a bit in this register is set to 1,
     * the corresponding port pin is enabled as an input with a
     * high-impedance output driver. If a bit in this register is
     * cleared to 0, the corresponding port pin is enabled as an output.
     */
    ret = i2c_get(device, TCA6408_POLARITY_REG, &polarity);
    if (ret != 0)
        return -EIO;
    lldbg("polarity reg=0x%02hhX\n", polarity);
    polarity = (polarity & (1 << which)) >> which;
    lldbg("polarity=0x%hhu\n", polarity);

    return polarity;
}


int tca6408_set(struct gpio_device *device, unsigned int which,
                unsigned int val)
{
    uint8_t reg;
    int ret;

    /* Set output pins default value (before configuring it as output
     *
     * The Output Port Register (register 1) shows the outgoing logic
     * levels of the pins defined as outputs by the Configuration Register.
     */
    ret = i2c_get(device, TCA6408_OUTPUT_REG, &reg);
    if (ret != 0)
        return -EIO;
    lldbg("current reg=0x%02hhX\n", reg);
    if (val) {
        reg |= (1 << which);
    } else {
        reg &= ~(1 << which);
    }
    lldbg("new reg=0x%02hhX\n", reg);
    return i2c_set(device, TCA6408_OUTPUT_REG, reg);
}

int tca6408_get(struct gpio_device *device, unsigned int which)
{
    uint8_t in;
    int ret;

    /*
     * The Input Port Register (register 0) reflects the incoming logic
     * levels of the pins, regardless of whether the pin is defined as an
     * input or an output by the Configuration Register. They act only on
     * read operation.
     */
    ret = i2c_get(device, TCA6408_INPUT_REG, &in);
    if (ret != 0)
        return -EIO;
    lldbg("input reg=0x%02hhX\n", in);
    in &= (1 << which);
    in = !!in;
    lldbg("in=%hhu\n", in);

    return in;
}

unsigned int tca6408_line_count(struct gpio_device *device)
{
    return TCA6408_NR_GPIO;
}

static struct gpio_ops gpio_ops;

/* FIXME no error handling in this function */
struct tca6408_dev *tca6408_init(struct i2c_adapter *adapter)
{
    struct tca6408_dev *tca6408;

    RET_IF_FAIL(adapter, NULL);
    RET_IF_FAIL(adapter->dev.bus, NULL);

    tca6408 = malloc(sizeof(*tca6408));
    memset(tca6408, 0, sizeof(*tca6408));

    tca6408->adapter = adapter;
    tca6408->dev.dev.name = "tca6408";
    tca6408->dev.dev.description = "GPIO controller";
    tca6408->dev.dev.class = DEVICE_CLASS_GPIO;
    tca6408->dev.ops = &gpio_ops;

    gpio_device_register(&tca6408->dev);
    bus_add_device(adapter->dev.bus, &tca6408->dev.dev);

    return tca6408;
}

void tca6408_exit(struct tca6408_dev *tca6408)
{
    bus_rm_device(&tca6408->dev.dev);
    gpio_device_unregister(&tca6408->dev);
    free(tca6408);
}

static struct gpio_ops gpio_ops = {
    .get_direction = tca6408_get_direction,
    .direction_out = tca6408_set_direction_out,
    .direction_in = tca6408_set_direction_in,

    .get_value = tca6408_get,
    .set_value = tca6408_set,

    .line_count = tca6408_line_count,
};
