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
 */

#include "scm.h"
#include "chip.h"
#include "gpio.h"

#include <asm/irq.h>
#include <asm/tsb-irq.h>
#include <lib/utils.h>
#include <lib/sleep.h>
#include <phabos/i2c.h>
#include <phabos/gpio/tca6408.h>
#include <errno.h>

#define TCA6408_U72_RST_GPIO 4
#define TCA6408_TW      1 /* 1us (datasheet: reset pulse duration (Tw) is 4ns */
#define TCA6408_TRESET  1 /* 1us (datasheet: time to reset (Treset) is 600ns */

static int tca6408_probe(struct driver *driver)
{
    struct tca6408_dev *tca6408;
    struct driver *i2c_driver;
    struct i2c_adapter *adapter;

    i2c_driver = driver_lookup("tsb-i2c");
    RET_IF_FAIL(i2c_driver && i2c_driver->dev, -ENODEV);

    adapter = containerof(i2c_driver->dev, struct i2c_dev, dev);

    tca6408 = tca6408_init(adapter);
    if (!tca6408)
        return -ENODEV;

    tsb_gpio_direction_out(NULL, TCA6408_U72_RST_GPIO, 0);

    /* datasheet: reset pulse minimum duration (Tw) is 4ns */
    usleep(TCA6408_TW);

    tsb_gpio_direction_out(NULL, TCA6408_U72_RST_GPIO, 1);

    /* datasheet: time to reset (Treset) is 600ns */
    usleep(TCA6408_TRESET);

    driver->dev = &tca6408->dev.dev;

    return 0;
}

static int tca6408_remove(struct driver *driver)
{
    RET_IF_FAIL(driver, -EINVAL);
    RET_IF_FAIL(driver->dev, -EINVAL);

    struct device_driver *dev = driver->dev;
    struct gpio_device *gpio_dev = containerof(dev, struct gpio_device, dev);
    struct tca6408_dev *tca6408 = containerof(gpio_dev, struct tca6408_dev, dev);

    tca6408_exit(tca6408);

    return 0;
}

struct driver tca6408_driver = {
    .name = "tsb-tca6408",
    .probe = tca6408_probe,
    .remove = tca6408_remove,
};
