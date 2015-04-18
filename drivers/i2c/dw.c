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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <asm/hwio.h>
#include <asm/spinlock.h>
#include <asm/tsb-irq.h>
#include <asm/machine.h>

#include <phabos/sleep.h>
#include <phabos/mutex.h>
#include <phabos/semaphore.h>
#include <phabos/utils.h>

#include <phabos/i2c.h>
#include <phabos/i2c/dw.h>

#include "dw.h"

/* disable the verbose debug output */
#undef lldbg
#define lldbg(x...)

/* I2C controller configuration */
#define DW_I2C_CONFIG (DW_I2C_CON_RESTART_EN | \
                       DW_I2C_CON_MASTER | \
                       DW_I2C_CON_SLAVE_DISABLE | \
                       DW_I2C_CON_SPEED_FAST)

#define DW_I2C_TX_FIFO_DEPTH   8
#define DW_I2C_RX_FIFO_DEPTH   8

/* IRQs handle by the driver */
#define DW_I2C_INTR_DEFAULT_MASK (DW_I2C_INTR_RX_FULL | \
                                  DW_I2C_INTR_TX_EMPTY | \
                                  DW_I2C_INTR_TX_ABRT | \
                                  DW_I2C_INTR_STOP_DET)

#define TIMEOUT                     20              /* 20 ms */
#define DW_I2C_TIMEOUT              10000000        /* 1sec in usec */


void dw_mach_initialize(struct i2c_adapter *dev);
void dw_mach_destroy(void);

static uint32_t dw_read(struct dw_dev *dev, int offset)
{
    assert(dev);
    return read32(dev->base + offset);
}

static void dw_write(struct dw_dev *dev, int offset, uint32_t b)
{
    assert(dev);
    write32(dev->base + offset, b);
}

static void tsb_i2c_clear_int(struct dw_dev *dev)
{
    dw_read(dev, DW_I2C_CLR_INTR);
}

static void tsb_i2c_disable_int(struct dw_dev *dev)
{
    dw_write(dev, DW_I2C_INTR_MASK, 0);
}

static void dw_set_enable(struct dw_dev *dev, int enable)
{
    int i;

    for (i = 0; i < 50; i++) {
        dw_write(dev, DW_I2C_ENABLE, enable);

        if ((dw_read(dev, DW_I2C_ENABLE_STATUS) & 0x1) == enable)
            return;

        usleep(25);
    }

    lldbg("timeout!");
}

/* Enable the controller */
static void tsb_i2c_enable(struct dw_dev *dev)
{
    dw_set_enable(dev, 1);
}

/* Disable the controller */
static void tsb_i2c_disable(struct dw_dev *dev)
{
    dw_set_enable(dev, 0);

    tsb_i2c_disable_int(dev);
    tsb_i2c_clear_int(dev);
}

/**
 * Initialize the TSB I2 controller
 */
static void tsb_i2c_init(struct dw_dev *dev)
{
    /* Disable the adapter */
    tsb_i2c_disable(dev);

    /* Set timings for Standard and Fast Speed mode */
    dw_write(dev, DW_I2C_SS_SCL_HCNT, 28);
    dw_write(dev, DW_I2C_SS_SCL_LCNT, 52);
    dw_write(dev, DW_I2C_FS_SCL_HCNT, 47);
    dw_write(dev, DW_I2C_FS_SCL_LCNT, 65);

    /* Configure Tx/Rx FIFO threshold levels */
    dw_write(dev, DW_I2C_TX_TL, DW_I2C_TX_FIFO_DEPTH - 1);
    dw_write(dev, DW_I2C_RX_TL, 0);

    /* configure the i2c master */
    dw_write(dev, DW_I2C_CON, DW_I2C_CONFIG);
}

static int tsb_i2c_wait_bus_ready(struct dw_dev *dev)
{
    int timeout = TIMEOUT;

    while (dw_read(dev, DW_I2C_STATUS) & DW_I2C_STATUS_ACTIVITY) {
        if (timeout <= 0) {
            lldbg("timeout\n");
            return -ETIMEDOUT;
        }
        timeout--;
        usleep(1000);
    }

    return 0;
}

static void tsb_i2c_start_transfer(struct dw_dev *dev)
{
    lldbg("\n");

    /* Disable the adapter */
    tsb_i2c_disable(dev);

    /* write target address */
    dw_write(dev, DW_I2C_TAR, dev->msgs[dev->tx_index].addr);

    /* Disable the interrupts */
    tsb_i2c_disable_int(dev);

    /* Enable the adapter */
    tsb_i2c_enable(dev);

    /* Clear interrupts */
    tsb_i2c_clear_int(dev);

    /* Enable interrupts */
    dw_write(dev, DW_I2C_INTR_MASK, DW_I2C_INTR_DEFAULT_MASK);
}

/**
 * Internal function that handles the read or write transfer
 * It is called from the IRQ handler.
 */
static void tsb_i2c_transfer_msg(struct dw_dev *dev)
{
    uint32_t intr_mask;
    uint32_t addr = dev->msgs[dev->tx_index].addr;
    uint8_t *buffer = dev->tx_buffer;
    uint32_t length = dev->tx_length;

    bool need_restart = false;

    int tx_avail;
    int rx_avail;

    lldbg("tx_index %d\n", dev->tx_index);

    /* loop over the i2c message array */
    for (; dev->tx_index < dev->msgs_count; dev->tx_index++) {

        if (dev->msgs[dev->tx_index].addr != addr) {
            lldbg("invalid target address\n");
            dev->msg_err = -EINVAL;
            break;
        }

        if (dev->msgs[dev->tx_index].length == 0) {
            lldbg("invalid message length\n");
            dev->msg_err = -EINVAL;
            break;
        }

        if (!(dev->status & DW_I2C_STATUS_WRITE_IN_PROGRESS)) {
            /* init a new msg transfer */
            buffer = dev->msgs[dev->tx_index].buffer;
            length = dev->msgs[dev->tx_index].length;

            /* force a restart between messages */
            if (dev->tx_index > 0)
                need_restart = true;
        }

        /* Get the amount of free space in the internal buffer */
        tx_avail = DW_I2C_TX_FIFO_DEPTH - dw_read(dev, DW_I2C_TXFLR);
        rx_avail = DW_I2C_RX_FIFO_DEPTH - dw_read(dev, DW_I2C_RXFLR);

        /* loop until one of the fifo is full or buffer is consumed */
        while (length > 0 && tx_avail > 0 && rx_avail > 0) {
            uint32_t cmd = 0;

            if (dev->tx_index == dev->msgs_count - 1 && length == 1) {
                /* Last msg, issue a STOP */
                cmd |= (1 << 9);
                lldbg("STOP\n");
            }

            if (need_restart) {
                cmd |= (1 << 10); /* RESTART */
                need_restart = false;
                lldbg("RESTART\n");
            }

            if (dev->msgs[dev->tx_index].flags & I2C_READ) {
                if (rx_avail - dev->rx_outstanding <= 0)
                    break;

                dw_write(dev, DW_I2C_DATA_CMD, cmd | 1 << 8); /* READ */
                lldbg("READ\n");

                rx_avail--;
                dev->rx_outstanding++;
            } else {
                dw_write(dev, DW_I2C_DATA_CMD, cmd | *buffer++);
                lldbg("WRITE\n");
            }

            tx_avail--;
            length--;
        }

        dev->tx_buffer = buffer;
        dev->tx_length = length;

        if (length > 0) {
            dev->status |= DW_I2C_STATUS_WRITE_IN_PROGRESS;
            break;
        } else {
            dev->status &= ~DW_I2C_STATUS_WRITE_IN_PROGRESS;
        }
    }

    intr_mask = DW_I2C_INTR_DEFAULT_MASK;

    /* No more data to write. Stop the TX IRQ */
    if (dev->tx_index == dev->msgs_count)
        intr_mask &= ~DW_I2C_INTR_TX_EMPTY;

    /* In case of error, mask all the IRQs */
    if (dev->msg_err)
        intr_mask = 0;

    dw_write(dev, DW_I2C_INTR_MASK, intr_mask);
}

static void tsb_dw_read(struct dw_dev *dev)
{
    int rx_valid;

    lldbg("rx_index %d\n", dev->rx_index);

    for (; dev->rx_index < dev->msgs_count; dev->rx_index++) {
        uint32_t len;
        uint8_t *buffer;

        if (!(dev->msgs[dev->rx_index].flags & I2C_READ))
            continue;

        if (!(dev->status & DW_I2C_STATUS_READ_IN_PROGRESS)) {
            len = dev->msgs[dev->rx_index].length;
            buffer = dev->msgs[dev->rx_index].buffer;
        } else {
            len = dev->rx_length;
            buffer = dev->rx_buffer;
        }

        rx_valid = dw_read(dev, DW_I2C_RXFLR);

        for (; len > 0 && rx_valid > 0; len--, rx_valid--) {
            *buffer++ = dw_read(dev, DW_I2C_DATA_CMD);
            dev->rx_outstanding--;
        }

        if (len > 0) {
            /* start the read process */
            dev->status |= DW_I2C_STATUS_READ_IN_PROGRESS;
            dev->rx_length = len;
            dev->rx_buffer = buffer;

            return;
        } else {
            dev->status &= ~DW_I2C_STATUS_READ_IN_PROGRESS;
        }
    }
}

static int tsb_i2c_handle_tx_abort(struct dw_dev *dev)
{
    unsigned long abort_source = dev->abort_source;

    lldbg("%s: 0x%x\n", __func__, abort_source);

    if (abort_source & DW_I2C_TX_ABRT_NOACK) {
        lldbg("%s: DW_I2C_TX_ABRT_NOACK 0x%x\n", __func__, abort_source);
        return -EIO;
    }

    if (abort_source & DW_I2C_TX_ARB_LOST)
        return -EAGAIN;
    else if (abort_source & DW_I2C_TX_ABRT_GCALL_READ)
        return -EINVAL; /* wrong dev->msgs[] data */
    else
        return -EIO;
}

/* Perform a sequence of I2C transfers */
static int dw_transfer(struct i2c_adapter *adapter, struct i2c_msg *msg,
                       size_t count)
{
    int ret;
    struct dw_dev *dev = containerof(adapter, struct dw_dev, adapter);

    lldbg("msgs: %d\n", count);

    mutex_lock(&dev->mutex);

    dev->msgs = msg;
    dev->msgs_count = count;
    dev->tx_index = 0;
    dev->rx_index = 0;
    dev->rx_outstanding = 0;

    dev->cmd_err = 0;
    dev->msg_err = 0;
    dev->status = DW_I2C_STATUS_IDLE;
    dev->abort_source = 0;

    ret = tsb_i2c_wait_bus_ready(dev);
    if (ret < 0)
        goto done;

    /*
     * start a watchdog to timeout the transfer if
     * the bus is locked up...
     */
    watchdog_start(&dev->timeout, DW_I2C_TIMEOUT);

    /* start the transfers */
    tsb_i2c_start_transfer(dev);

    semaphore_lock(&dev->wait);

    watchdog_cancel(&dev->timeout);

    if (dev->status == DW_I2C_STATUS_TIMEOUT) {
        lldbg("controller timed out\n");

        /* Re-init the adapter */
        tsb_i2c_init(dev);
        ret = -ETIMEDOUT;
        goto done;
    }

    tsb_i2c_disable(dev);

    if (dev->msg_err) {
        ret = dev->msg_err;
        lldbg("error msg_err %x\n", dev->msg_err);
        goto done;
    }

    if (!dev->cmd_err) {
        ret = 0;
        lldbg("no error %d\n", count);
        goto done;
    }

    /* Handle abort errors */
    if (dev->cmd_err == DW_I2C_ERR_TX_ABRT) {
        ret = tsb_i2c_handle_tx_abort(dev);
        goto done;
    }

    /* default error code */
    ret = -EIO;
    lldbg("unknown error %x\n", ret);

done:
    mutex_unlock(&dev->mutex);

    return ret;
}

static uint32_t tsb_dw_read_clear_intrbits(struct dw_dev *dev)
{
    uint32_t stat = dw_read(dev, DW_I2C_INTR_STAT);

    if (stat & DW_I2C_INTR_RX_UNDER)
        dw_read(dev, DW_I2C_CLR_RX_UNDER);
    if (stat & DW_I2C_INTR_RX_OVER)
        dw_read(dev, DW_I2C_CLR_RX_OVER);
    if (stat & DW_I2C_INTR_TX_OVER)
        dw_read(dev, DW_I2C_CLR_TX_OVER);
    if (stat & DW_I2C_INTR_RD_REQ)
        dw_read(dev, DW_I2C_CLR_RD_REQ);
    if (stat & DW_I2C_INTR_TX_ABRT) {
        /* IC_TX_ABRT_SOURCE reg is cleared upon read, store it */
        dev->abort_source = dw_read(dev, DW_I2C_TX_ABRT_SOURCE);
        dw_read(dev, DW_I2C_CLR_TX_ABRT);
    }
    if (stat & DW_I2C_INTR_RX_DONE)
        dw_read(dev, DW_I2C_CLR_RX_DONE);
    if (stat & DW_I2C_INTR_ACTIVITY)
        dw_read(dev, DW_I2C_CLR_ACTIVITY);
    if (stat & DW_I2C_INTR_STOP_DET)
        dw_read(dev, DW_I2C_CLR_STOP_DET);
    if (stat & DW_I2C_INTR_START_DET)
        dw_read(dev, DW_I2C_CLR_START_DET);
    if (stat & DW_I2C_INTR_GEN_CALL)
        dw_read(dev, DW_I2C_CLR_GEN_CALL);

    return stat;
}

/* I2C interrupt service routine */
static void dw_interrupt(int irq, void *data)
{
    uint32_t stat, enabled;
    struct dw_dev *dev = data;

    enabled = dw_read(dev, DW_I2C_ENABLE);
    stat = dw_read(dev, DW_I2C_RAW_INTR_STAT);

    lldbg("enabled=0x%x stat=0x%x\n", enabled, stat);

    if (!enabled || !(stat & ~DW_I2C_INTR_ACTIVITY))
        return;

    stat = tsb_dw_read_clear_intrbits(dev);

    if (stat & DW_I2C_INTR_TX_ABRT) {
        lldbg("abort\n");
        dev->cmd_err |= DW_I2C_ERR_TX_ABRT;
        dev->status = DW_I2C_STATUS_IDLE;

        tsb_i2c_disable_int(dev);
        goto tx_aborted;
    }

    if (stat & DW_I2C_INTR_RX_FULL)
        tsb_dw_read(dev);

    if (stat & DW_I2C_INTR_TX_EMPTY)
        tsb_i2c_transfer_msg(dev);

tx_aborted:
    if (stat & DW_I2C_INTR_TX_ABRT)
        lldbg("aborted %x %x\n", stat, dev->abort_source);

    if ((stat & (DW_I2C_INTR_TX_ABRT | DW_I2C_INTR_STOP_DET)) || dev->msg_err) {
        lldbg("release sem\n");
        semaphore_unlock(&dev->wait);
    }
}

/**
 * Watchdog handler for timeout of I2C operation
 */
static void dw_timeout(struct watchdog *wd)
{
    struct dw_dev *dev = wd->user_priv;
    struct spinlock timeout_lock = SPINLOCK_INIT(timeout_lock);

    lldbg("\n");

    spinlock_lock(&timeout_lock);

    if (dev->status != DW_I2C_STATUS_IDLE)
    {
        lldbg("finished\n");
        /* Mark the transfer as finished */
        dev->status = DW_I2C_STATUS_TIMEOUT;
        semaphore_unlock(&dev->wait);
    }

    spinlock_unlock(&timeout_lock);
}

struct fops dw_fops;

struct dw_dev *dw_init(void)
{
    int retval;
    struct dw_dev *dw;

    dw = malloc(sizeof(*dw));
    if (!dw)
        return NULL;
    memset(dw, 0, sizeof(*dw));

    dw->adapter.dev.name = "dw-i2c";
    dw->adapter.dev.description = "I2C adapter";
    dw->adapter.dev.class = DEVICE_CLASS_I2C;
    dw->adapter.dev.ops = dw_fops;
    dw->adapter.transfer = dw_transfer;
    dw->adapter.irq = dw_interrupt;

    mutex_init(&dw->mutex);
    semaphore_init(&dw->wait, 0);

    /* Allocate a watchdog timer */
    watchdog_init(&dw->timeout);
    dw->timeout.timeout = dw_timeout;
    dw->timeout.user_priv = dw;

    retval =  i2c_adapter_register(&dw->adapter);
    GOTO_IF_FAIL(!retval, adapter_register_error);

    return dw;

adapter_register_error:
    watchdog_delete(&dw->timeout);
    free(dw);

    return NULL;
}

void dw_exit(struct dw_dev *dw)
{
    RET_IF_FAIL(dw,);

    i2c_adapter_unregister(&dw->adapter);
    watchdog_delete(&dw->timeout);
    free(dw);
}

static int dw_open(struct device_driver *dev)
{
    struct i2c_adapter *adapter = containerof(dev, struct i2c_adapter, dev);
    struct dw_dev *dw_dev = containerof(adapter, struct dw_dev, adapter);

    /* Initialize the I2C controller */
    tsb_i2c_init(dw_dev);
    return 0;
}

static void dw_close(struct device_driver *dev)
{
    struct i2c_adapter *adapter = containerof(dev, struct i2c_adapter, dev);
    struct dw_dev *dw = containerof(adapter, struct dw_dev, adapter);

    lldbg("\n");

    watchdog_cancel(&dw->timeout);
}

struct fops dw_fops = {
    .open       = dw_open,
    .close      = dw_close,
};
