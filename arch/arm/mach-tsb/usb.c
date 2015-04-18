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

#include <errno.h>
#include <asm/irq.h>
#include <asm/tsb-irq.h>
#include <lib/utils.h>
#include <phabos/device.h>

static int dwc2_probe(struct device_driver *device)
{
    int retval;

    tsb_clk_enable(TSB_CLK_HSIC480);
    tsb_clk_enable(TSB_CLK_HSICREF);
    tsb_clk_enable(TSB_CLK_HSICBUS);

    tsb_reset(TSB_RST_HSIC);
    tsb_reset(TSB_RST_HSICPHY);
    tsb_reset(TSB_RST_HSICPOR);

    retval = dwc_init(device, HSIC_BASE, 0xFFFFFFFF);
    if (retval) {
        tsb_clk_disable(TSB_CLK_HSIC480);
        tsb_clk_disable(TSB_CLK_HSICREF);
        tsb_clk_disable(TSB_CLK_HSICBUS);
        return retval;
    }

    return 0;
}

static int dwc2_remove(struct device_driver *device)
{
    int retval;

    irq_disable_line(TSB_IRQ_HSIC);
    irq_detach(TSB_IRQ_HSIC);

    retval = dwc2_exit(device);

    tsb_clk_disable(TSB_CLK_HSIC480);
    tsb_clk_disable(TSB_CLK_HSICREF);
    tsb_clk_disable(TSB_CLK_HSICBUS);

    return retval;
}

struct device_driver dwc2_driver = {
    .bus = BUS_USB,
    .probe = dwc2_probe,
    .remove = dwc2_remove,
};
