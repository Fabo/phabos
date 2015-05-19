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

#include <config.h>
#include <asm/hwio.h>
#include <phabos/driver.h>

#define UART_RBR_THR_DLL            (UART_BASE + 0x0)
#define UART_IER_DLH                (UART_BASE + 0x4)
#define UART_FCR_IIR                (UART_BASE + 0x8)
#define UART_LCR                    (UART_BASE + 0xc)

#define UART_115200_BPS             26
#define UART_DLL_115200             ((UART_115200_BPS >> 0) & 0xff)
#define UART_DLH_115200             ((UART_115200_BPS >> 8) & 0xff)
#define UART_LCR_DLAB               (1 << 7)
#define UART_LCR_DLS_8              3

#define UART_FCR_IIR_IID0_FIFOE     (1 << 0)
#define UART_FCR_IIR_IID1_RFIFOR    (1 << 1)
#define UART_FCR_IIR_IID1_XFIFOR    (1 << 2)

struct bridge {
    const char name[16];
    void (*init)(void);
};

static void apb2_init(void)
{
#ifdef CONFIG_I2C_DW
    extern struct driver dw_driver;
    driver_register(&dw_driver);
#endif

#ifdef CONFIG_TCA6408
    extern struct driver tca6408_driver;
    driver_register(&tca6408_driver);
#endif

#ifdef CONFIG_UNIPRO
    extern struct driver tsb_unipro_driver;
    driver_register(&tsb_unipro_driver);
#endif
}

static void early_uart_init(void)
{
    tsb_set_pinshare(TSB_PIN_UART_RXTX | TSB_PIN_UART_CTSRTS);

    tsb_clk_enable(TSB_CLK_UARTP);
    tsb_clk_enable(TSB_CLK_UARTS);

    tsb_reset(TSB_RST_UARTP);
    tsb_reset(TSB_RST_UARTS);

    for (int i = 0; i < 3; i++) {
        write32(UART_LCR, UART_LCR_DLAB | UART_LCR_DLS_8);
        write32(UART_RBR_THR_DLL, UART_DLL_115200);
        write32(UART_IER_DLH, UART_DLH_115200);
        write32(UART_LCR, UART_LCR_DLS_8);
        write32(UART_IER_DLH, 1);
        write32(UART_FCR_IIR, UART_FCR_IIR_IID0_FIFOE |
                UART_FCR_IIR_IID1_RFIFOR | UART_FCR_IIR_IID1_XFIFOR);
    }
}

static struct bridge bridge = {
#if defined(CONFIG_TSB_APB2)
    .name = "APB2",
    .init = apb2_init,
#else
    #error "TSB: Bad bridge configuration"
#endif
};

void machine_init(void)
{
    tsb_gpio_initialize();
    early_uart_init();
    bridge.init();
}
