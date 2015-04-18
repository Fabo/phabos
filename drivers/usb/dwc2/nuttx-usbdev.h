/************************************************************************************
 * include/nuttx/usb/usbdev.h
 *
 *   Copyright (C) 2008-2010, 2012-2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * NOTE:  This interface was inspired by the Linux gadget interface by
 * David Brownell. That work was very helpful in determining a usable
 * partitioning of functionality between standard class drivers and various
 * implementations of USB controller drivers.  This work, however, does
 * not derive directly from that work and is licensed differently.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************************/

#ifndef _INCLUDE_NUTTX_USB_USBDEV_H
#define _INCLUDE_NUTTX_USB_USBDEV_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#if 0
#include <nuttx/usb/pl2303.h>
#include <nuttx/usb/cdcacm.h>
#include <nuttx/usb/usbmsc.h>
#include <nuttx/usb/composite.h>
#endif

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/* Endpoint helpers *****************************************************************/

/* Configure endpoint, making it usable.  The class driver may deallocate or re-use
 * the 'desc' structure after returning:
 *
 * ep   - the struct usbdev_ep instance obtained from allocep()
 * desc - A struct usb_epdesc instance describing the endpoint
 * last - true if this this last endpoint to be configured.  Some hardware needs
 *        to take special action when all of the endpoints have been configured.
 */

#define EP_CONFIGURE(ep,desc,last) (ep)->ops->configure(ep,desc,last)

/* The endpoint will no longer be used */

#define EP_DISABLE(ep)             (ep)->ops->disable(ep)

/* Allocate/free I/O requests.  Should not be called from interrupt processing! */

#define EP_ALLOCREQ(ep)            (ep)->ops->allocreq(ep)
#define EP_FREEREQ(ep,req)         (ep)->ops->freereq(ep,req)

/* Allocate/free an I/O buffer.  Should not be called from interrupt processing! */

#ifdef CONFIG_USBDEV_DMA
#  define EP_ALLOCBUFFER(ep,nb)    (ep)->ops->allocbuffer(ep,nb)
#  define EP_FREEBUFFER(ep,buf)    (ep)->ops->freebuffer(ep,buf)
#else
#  define EP_ALLOCBUFFER(ep,nb)    malloc(nb)
#  define EP_FREEBUFFER(ep,buf)    free(buf)
#endif

/* Submit an I/O request to the endpoint */

#define EPUBMIT(ep,req)          (ep)->ops->submit(ep,req)

/* Cancel an I/O request previously sent to an endpoint */

#define EP_CANCEL(ep,req)          (ep)->ops->cancel(ep,req)

/* Stall or resume an endpoint */

#define EPTALL(ep)               (ep)->ops->stall(ep,false)
#define EP_RESUME(ep)              (ep)->ops->stall(ep,true)

/* USB Device Driver Helpers ********************************************************/

/* Allocate an endpoint:
 *
 *   ep     - 7-bit logical endpoint number (direction bit ignored).  Zero means
 *            that any endpoint matching the other requirements will suffice.  The
 *            assigned endpoint can be found in the eplog field.
 *   in     - true: IN (device-to-host) endpoint requested
 *   eptype - Endpoint type.  One of {USB_EP_ATTR_XFER_ISOC, USB_EP_ATTR_XFER_BULK,
 *            USB_EP_ATTR_XFER_INT}
 */

#define DEV_ALLOCEP(dev,ep,in,type) (dev)->ops->allocep(dev,ep,in,type)

/* Release an endpoint */

#define DEV_FREEEP(dev,ep)         (dev)->ops->freeep(dev,ep)

/* Returns the current frame number */

#define DEV_GETFRAME(dev)          (dev)->ops->getframe(dev)

/* Tries to wake up the host connected to this device */

#define DEV_WAKEUP(dev)            (dev)->ops->wakeup(dev)

/* Sets the device selfpowered feature */

#define DEVETSELFPOWERED(dev)    (dev)->ops->selfpowered(dev,true)

/* Clears the device selfpowered feature */

#define DEV_CLRSELFPOWERED(dev)    (dev)->ops->selfpowered(dev, false)

/* Software-controlled connect to USB host. All USB class drivers need to call
 * DEV_CONNECT() when they are ready to be enumerated.  That is, (1) initially when
 * bound to the USB driver, and (2) after a USB reset.
 */

#define DEV_CONNECT(dev)           (dev)->ops->pullup ? (dev)->ops->pullup(dev,true) : -EOPNOTSUPP

/* Software-controlled disconnect from USB host */

#define DEV_DISCONNECT(dev)        (dev)->ops->pullup ? (dev)->ops->pullup(dev,false) : -EOPNOTSUPP

/* USB Class Driver Helpers *********************************************************/
/* All may be called from interupt handling logic except bind() and unbind() */

/* Invoked when the driver is bound to a USB device driver. */

#define CLASS_BIND(drvr,dev)      (drvr)->ops->bind(drvr,dev)

/* Invoked when the driver is unbound from a USB device driver */

#define CLASS_UNBIND(drvr,dev)    (drvr)->ops->unbind(drvr,dev)

/* Invoked after all transfers have been stopped, when the host is disconnected. */

#define CLASS_DISCONNECT(drvr,dev) (drvr)->ops->disconnect(drvr,dev)

/* Invoked for ep0 control requests */

#define CLASS_SETUP(drvr,dev,ctrl,dataout,outlen) \
  (drvr)->ops->setup(drvr,dev,ctrl,dataout,outlen)

/* Invoked on USB suspend. */

#define CLASS_SUSPEND(drvr,dev)   \
  do { if ((drvr)->ops->suspend) (drvr)->ops->suspend(drvr,dev); } while (0)

/* Invoked on USB resume */

#define CLASS_RESUME(drvr,dev)  \
  do { if ((drvr)->ops->resume) (drvr)->ops->resume(drvr,dev); } while (0)

/* Device speeds */

#define USBPEED_UNKNOWN         0 /* Transfer rate not yet set */
#define USBPEED_LOW             1 /* USB 1.1 */
#define USBPEED_FULL            2 /* USB 1.1 */
#define USBPEED_HIGH            3 /* USB 2.0 */
#define USBPEED_VARIABLE        4 /* Wireless USB 2.5 */

/* Maximum size of a request buffer */

#define USBDEV_MAXREQUEUST        UINT16_MAX

/* Request flags */

#define USBDEV_REQFLAGS_NULLPKT   1 /* Bit 0: Terminate w/short packet; null packet if necessary */
                                    /* Bits 1-7: Available */

/************************************************************************************
 * Public Types
 ************************************************************************************/

/* USB Controller Structures ********************************************************/

/* struct usbdev_req - describes one i/o request */

struct usbdev_ep;
struct usbdev_req
{
  uint8_t *buf;    /* Call: Buffer used for data; Return: Unchanged */
  uint8_t  flags;  /* See USBDEV_REQFLAGS_* definitions */
  uint16_t len;    /* Call: Total length of data in buf; Return: Unchanged */
  uint16_t xfrd;   /* Call: zero; Return: Bytes transferred so */
  int16_t  result; /* Call: zero; Return: Result of transfer (O or -errno) */

  /* Callback when the transfer completes */

  void   (*callback)(struct usbdev_ep *ep, struct usbdev_req *req);
  void    *priv;    /* Used only by callee */
};

/* Endpoint-specific interface to USB controller hardware. */

struct usbdev_epops
{
  /* Configure/enable and disable endpoint */

  int (*configure)(struct usbdev_ep *ep, const struct usb_epdesc *desc,
                   bool last);
  int (*disable)(struct usbdev_ep *ep);

  /* Allocate and free I/O requests */

  struct usbdev_req *(*allocreq)(struct usbdev_ep *ep);
  void (*freereq)(struct usbdev_ep *ep, struct usbdev_req *req);

  /* Allocate and free I/O buffers */

#ifdef CONFIG_USBDEV_DMA
  void *(*allocbuffer)(struct usbdev_ep *ep, uint16_t nbytes);
  void (*freebuffer)(struct usbdev_ep *ep, void *buf);
#endif

  /* Submit and cancel I/O requests */

  int (*submit)(struct usbdev_ep *ep, struct usbdev_req *req);
  int (*cancel)(struct usbdev_ep *ep, struct usbdev_req *req);

  /* Stall or resume an endpoint */

  int (*stall)(struct usbdev_ep *ep, bool resume);
};

/* Representation of one USB endpoint */

struct usbdev_ep
{
  const struct usbdev_epops *ops; /* Endpoint operations */
  uint8_t  eplog;                   /* Logical endpoint address */
  uint16_t maxpacket;               /* Maximum packet size for this endpoint */
  void    *priv;                    /* For use by class driver */
};

/* struct usbdev represents a usb device */

struct usbdev;
struct usbdev_ops
{
  /* Allocate and free endpoints */

  struct usbdev_ep *(*allocep)(struct usbdev *dev, uint8_t epphy,
                                     bool in, uint8_t eptype);
  void (*freeep)(struct usbdev *dev, struct usbdev_ep *ep);

  /* Get the frame number from the last SOF */

  int (*getframe)(struct usbdev *dev);

  /* Hardware specific features */

  int (*wakeup)(struct usbdev *dev);
  int (*selfpowered)(struct usbdev *dev, bool selfpowered);
  int (*pullup)(struct usbdev *dev,  bool enable);

  /* Device-specific I/O command support */

  int (*ioctl)(struct usbdev *dev, unsigned code, unsigned long param);
};

struct usbdev
{
  const struct usbdev_ops *ops; /* Access to hardware specific features */
  struct usbdev_ep *ep0;        /* Endpoint zero */
  uint8_t speed;                  /* Current speed of the host connection */
  uint8_t dualspeed:1;            /* 1:supports high and full speed operation */
};

/* USB Device Class Implementations *************************************************/

struct usbdevclass_driver;
struct usbdevclass_driverops
{
  int  (*bind)(struct usbdevclass_driver *driver, struct usbdev *dev);
  void (*unbind)(struct usbdevclass_driver *driver, struct usbdev *dev);
  int  (*setup)(struct usbdevclass_driver *driver, struct usbdev *dev,
          const struct usb_ctrlreq *ctrl, uint8_t *dataout, size_t outlen);
  void (*disconnect)(struct usbdevclass_driver *driver,
          struct usbdev *dev);
  void (*suspend)(struct usbdevclass_driver *driver, struct usbdev *dev);
  void (*resume)(struct usbdevclass_driver *driver, struct usbdev *dev);
};

struct usbdevclass_driver
{
  const struct usbdevclass_driverops *ops;
  uint8_t speed;                  /* Highest speed that the driver handles */
};

/************************************************************************************
 * Public Data
 ************************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#  define EXTERN extern "C"
extern "C"
{
#else
#  define EXTERN extern
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: usbdevclass_register
 *
 * Description:
 *   Register a USB device class driver. The class driver's bind() method will be
 *   called to bind it to a USB device driver.
 *
 ************************************************************************************/

int usbdev_register(struct usbdevclass_driver *driver);

/************************************************************************************
 * Name: usbdev_unregister
 *
 * Description:
 *   Un-register usbdev class driver.If the USB device is connected to a USB host,
 *   it will first disconnect().  The driver is also requested to unbind() and clean
 *   up any device state, before this procedure finally returns.
 *
 ************************************************************************************/

int usbdev_unregister(struct usbdevclass_driver *driver);

/****************************************************************************
 * Name: usbdev_dma_alloc and usbdev_dma_free
 *
 * Description:
 *   The USB class driver allocates packet I/O buffers for data transfer by
 *   calling the driver allocbuffer() and freebuffer() methods.  Those
 *   methods are only available if CONFIG_USBDEV_DMA is defined in the
 *   system configuration.
 *
 *   If CONFIG_USBDEV_DMAMEMORY is also defined in the NuttX configuration,
 *   then the driver implementations of the allocbuffer() and freebuffer()
 *   methods may use board-specific usbdev_dma_alloc() and usbdev_dma_free().
 *   If CONFIG_USBDEV_DMA and CONFIG_USBDEV_DMAMEMORY are both defined,
 *   then the board-specific logic must provide the functions
 *   usbdev_dma_alloc() and usbdev_dma_free() as prototyped below:
 *   usbdev_dma_alloc() will allocate DMA-capable memory of the specified
 *   size; usbdev_dma_free() is the corresponding function that will be
 *   called to free the DMA-capable memory.
 *
 *   This functions may be simple wrappers around gran_alloc() and
 *   gran_free() (See nuttx/mm/gran.h).  Note that the gran_free() function
 *   does require the size of the allocation to be freed; that would need
 *   to be managed in the board-specific logic.
 *
 ****************************************************************************/

#if defined(CONFIG_USBDEV_DMA) && defined(CONFIG_USBDEV_DMAMEMORY)
void *usbdev_dma_alloc(size_t size);
void usbdev_dma_free(void *memory);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* _INCLUDE_NUTTX_USB_USBDEV_H */