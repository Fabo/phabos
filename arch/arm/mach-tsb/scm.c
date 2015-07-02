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
 * Author: Benoit Cousson <bcousson@baylibre.com>
 *         Fabien Parent <fparent@baylibre.com>
 */

#include <stdint.h>
#include <stdio.h>

#include <asm/hwio.h>
#include <phabos/utils.h>
#include <phabos/kprintf.h>

#include "scm.h"
#include "chip.h"

#define TSB_SCM_SOFTRESET0              0x00000000
#define TSB_SCM_SOFTRESETRELEASE0       0x00000100
#define TSB_SCM_CLOCKGATING0            0x00000200
#define TSB_SCM_CLOCKENABLE0            0x00000300
#define TSB_SCM_PINSHARE                0x00000800

static uint32_t scm_read(uint32_t offset)
{
    return read32(SYSCTL_BASE + offset);
}

static void scm_write(uint32_t offset, uint32_t v)
{
    write32(SYSCTL_BASE + offset, v);
}

void tsb_clk_init(void)
{
    scm_write(TSB_SCM_CLOCKGATING0, INIT_PERIPHERAL_CLOCK_BITS);
}

void tsb_clk_enable(uint32_t clk)
{
    scm_write(TSB_SCM_CLOCKENABLE0 + CLK_OFFSET(clk), CLK_MASK(clk));
}

void tsb_clk_disable(uint32_t clk)
{
    scm_write(TSB_SCM_CLOCKGATING0 + CLK_OFFSET(clk), CLK_MASK(clk));
}

uint32_t tsb_clk_status(uint32_t clk)
{
    return scm_read(TSB_SCM_CLOCKENABLE0 + CLK_OFFSET(clk)) & CLK_MASK(clk);
}

void tsb_reset(uint32_t rst)
{
    scm_write(TSB_SCM_SOFTRESET0 + CLK_OFFSET(rst), CLK_MASK(rst));
    scm_write(TSB_SCM_SOFTRESETRELEASE0 + CLK_OFFSET(rst), CLK_MASK(rst));
}

void tsb_set_pinshare(uint32_t bits)
{
    uint32_t r = scm_read(TSB_SCM_PINSHARE);
    scm_write(TSB_SCM_PINSHARE, r | bits);
}

void tsb_clr_pinshare(uint32_t bits)
{
    uint32_t r = scm_read(TSB_SCM_PINSHARE);
    scm_write(TSB_SCM_PINSHARE, r & ~bits);
}

/* Debug code for command line tool usage */
struct clk_info {
    uint32_t    clk;
    const char  name[16];
};

struct clk_info clk_names[] = {
    { TSB_CLK_WORKRAM,      "workram" },
    { TSB_CLK_SDIOSYS,      "sdio_sys" },
    { TSB_CLK_SDIOSD,       "sdio_sd" },
    { TSB_CLK_BUFRAM,       "bufram" },
    { TSB_CLK_MBOX,         "mbox" },
    { TSB_CLK_GDMA,         "gdma" },
    { TSB_CLK_TMR,          "tmr" },
    { TSB_CLK_WDT,          "wdt" },
    { TSB_CLK_TIMER,        "timer" },
    { TSB_CLK_PWMODP,       "pwmod_p" },
    { TSB_CLK_PWMODS,       "pwmod_s" },
    { TSB_CLK_I2CP,         "i2c_p" },
    { TSB_CLK_I2CS,         "i2c_s" },
    { TSB_CLK_UARTP,        "uart_p" },
    { TSB_CLK_UARTS,        "uart_s" },
    { TSB_CLK_SPIP,         "spi_p" },
    { TSB_CLK_SPIS,         "spi_s" },
    { TSB_CLK_GPIO,         "gpio" },
    { TSB_CLK_I2SSYS,       "i2ssys" },
    { TSB_CLK_I2SBIT,       "i2sbit" },
    { TSB_CLK_UNIPROSYS,    "unipro_sys" },
    { TSB_CLK_UNIPROREF,    "unipro_ref" },
    { TSB_CLK_HSIC480,      "hsic480" },
    { TSB_CLK_HSICBUS,      "hsicbus" },
    { TSB_CLK_HSICREF,      "hsicref" },
    { TSB_CLK_CDSI0_TX_SYS, "cdsi0_tx_sys" },
    { TSB_CLK_CDSI0_TX_APB, "cdsi0_tx_apb" },
    { TSB_CLK_CDSI0_RX_SYS, "cdsi0_rx_sys" },
    { TSB_CLK_CDSI0_RX_APB, "cdsi0_rx_apb" },
    { TSB_CLK_CDSI0_REF,    "cdsi0_ref" },
};

void tsb_clk_dump(void)
{
    for (int i = 0; i < ARRAY_SIZE(clk_names); i++) {
        kprintf("%12s: %s\n", clk_names[i].name,
                tsb_clk_status(clk_names[i].clk) ? "ON" : "OFF");
    }
}
