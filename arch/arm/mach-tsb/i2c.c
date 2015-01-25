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

#include <asm/irq.h>
#include <asm/tsb-irq.h>
#include <phabos/utils.h>
#include <phabos/i2c.h>

/**
 * Initialise an I2C device
 */
void dw_mach_initialize(struct i2c_dev *dev)
{
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->irq);

    /* enable I2C pins */
    tsb_clr_pinshare(TSB_PIN_SDIO);

    /* enable I2C clocks */
    tsb_clk_enable(TSB_CLK_I2CP);
    tsb_clk_enable(TSB_CLK_I2CS);

    /* reset I2C module */
    tsb_reset(TSB_RST_I2CP);
    tsb_reset(TSB_RST_I2CS);

    /* Attach Interrupt Handler */
    irq_attach(TSB_IRQ_I2C, dev->ops->irq, dev);

    /* Enable Interrupt Handler */
    irq_enable_line(TSB_IRQ_I2C);

    dev->base = I2C_BASE;
}

/**
 * Uninitialise an I2C device
 */
void dw_mach_destroy(void)
{
    irq_disable_line(TSB_IRQ_I2C);
    irq_detach(TSB_IRQ_I2C);
}
