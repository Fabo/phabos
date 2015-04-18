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

#ifndef _TSB_TCA6408_H_
#define _TSB_TCA6408_H_

#include <stdint.h>
#include <phabos/gpio.h>
#include <phabos/i2c.h>

struct tca6408_dev {
    struct gpio_device dev;
    struct i2c_adapter *adapter;
};

struct tca6408_dev *tca6408_init(struct i2c_adapter *adapter);
void tca6408_exit(struct tca6408_dev *tca6408);

void tca6408_set_direction_in(struct gpio_device *device, unsigned int which);
int tca6408_set_default_outputs(struct gpio_device *device, uint8_t dflt);
void tca6408_set_direction_out(struct gpio_device *device, unsigned int which,
                               unsigned int value);
int tca6408_get_direction(struct gpio_device *device, unsigned int which);
int tca6408_set_polarity_inverted(struct gpio_device *device,
                                  unsigned int which, uint8_t inverted);
int tca6408_get_polarity_inverted(struct gpio_device *device,
                                  unsigned int which);
int tca6408_set(struct gpio_device *device, unsigned int which,
                unsigned int val);
int tca6408_get(struct gpio_device *driver, unsigned int which);

#endif
