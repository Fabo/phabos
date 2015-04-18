/*
 * Copyright (c) 2015 Google Inc.
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

#ifndef __PHABOS_I2C_DW_H__
#define __PHABOS_I2C_DW_H__

#include <lib/mutex.h>
#include <lib/semaphore.h>
#include <phabos/watchdog.h>

struct dw_dev {
    struct i2c_adapter  adapter;   /* Generic I2C adapter */
    unsigned long       base;

    struct i2c_msg      *msgs;     /* Generic messages array */
    struct mutex        mutex;     /* Only one thread can access at a time */
    struct semaphore    wait;      /* Wait for state machine completion */
    struct watchdog     timeout;   /* Watchdog to timeout when bus hung */

    unsigned int        tx_index;
    unsigned int        tx_length;
    uint8_t             *tx_buffer;

    unsigned int        rx_index;
    unsigned int        rx_length;
    uint8_t             *rx_buffer;

    unsigned int        msgs_count;
    int                 cmd_err;
    int                 msg_err;
    unsigned int        status;
    uint32_t            abort_source;
    unsigned int        rx_outstanding;
};

struct dw_dev *dw_init(void);
void dw_exit(struct dw_dev *dw);

#endif /* __PHABOS_I2C_DW_H__ */

