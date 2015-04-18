/************************************************************************************
 * include/nuttx/usb/usb.h
 *
 *   Copyright (C) 2008, 2009-2010, 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#ifndef __INCLUDE_NUTTX_USB_USB_H
#define __INCLUDE_NUTTX_USB_USB_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <stdint.h>

/************************************************************************************
 * Preprocessor Definitions
 ************************************************************************************/
/* A packet identifier (PID) immediately follows the SYNC field of every USB packet.
 * A PID consists of a four-bit packet type field followed by a four-bit check field
 * USB Tokens (See Table 8-1 in the USB specification)
 */

#define USB_PID_OUT_TOKEN                       (0x01) /* Tokens */
#define USB_PID_IN_TOKEN                        (0x09)
#define USB_PIDOF_TOKEN                       (0x05)
#define USB_PIDETUP_TOKEN                     (0x0d)

#define USB_PID_DATA0                           (0x03) /* Data */
#define USB_PID_DATA1                           (0x0b)
#define USB_PID_DATA2                           (0x07)
#define USB_PID_MDATA                           (0x0f)

#define USB_PID_ACK                             (0x02) /* Handshake */
#define USB_PID_NAK                             (0x0a)
#define USB_PIDTALL                           (0x0e)
#define USB_PID_NYET                            (0x06)

#define USB_PID_PRE_TOKEN                       (0x0c) /* Special */
#define USB_PID_ERR                             (0x0c)
#define USB_PIDPLIT_TOKEN                     (0x08)
#define USB_PID_PING_TOKEN                      (0x04)
#define USB_PID_RESERVED                        (0x00)

/* All 16-bit values must be little-endian */

#define MSBYTE(u16)                             ((u16) >> 8)     /* Get MS byte from uint16_t */
#define LSBYTE(u16)                             ((u16) & 0xff)   /* Get LS byte from uint16_t */

#define GETUINT16(p)                            (((uint16_t)p[1]<<8)|(uint16_t)p[0])
#define GETUINT32(p)                            (((uint32_t)p[3]<<24)|((uint32_t)p[2]<<16)|\
                                                 ((uint32_t)p[1]<<8)|(uint32_t)p[0])

/* USB directions (in endpoint addresses) */

#define USB_DIR_MASK                            (0x80)
#define USB_EPNO_MASK                           (0x7f)
#define USB_DIR_OUT                             (0x00) /* host-to-device */
#define USB_DIR_IN                              (0x80) /* device-to-host */

#define USB_EPNO(addr)                          ((addr) & USB_EPNO_MASK)
#define USB_EPOUT(addr)                         ((addr) | USB_DIR_OUT)
#define USB_EPIN(addr)                          ((addr) | USB_DIR_IN)
#define USB_ISEPIN(addr)                        (((addr) & USB_DIR_MASK) == USB_DIR_IN)
#define USB_ISEPOUT(addr)                       (((addr) & USB_DIR_MASK) == USB_DIR_OUT)

/* Control Setup Packet.  Byte 0 = Request type */

#define USB_REQ_DIR_MASK                        (1 << 7) /* Bit 7=1: Direction bit */
#define USB_REQ_DIR_IN                          (1 << 7) /* Bit 7=1: Device-to-host */
#define USB_REQ_DIR_OUT                         (0 << 7) /* Bit 7=0: Host-to-device */

#define USB_REQ_ISIN(type)                      (((type) & USB_REQ_DIR_MASK) != 0)
#define USB_REQ_ISOUT(type)                     (((type) & USB_REQ_DIR_MASK) == 0)

#define USB_REQ_TYPEHIFT                      (5) /* Bits 5:6: Request type */
#  define USB_REQ_TYPE_MASK                     (3 << USB_REQ_TYPEHIFT)
#  define USB_REQ_TYPETANDARD                 (0 << USB_REQ_TYPEHIFT)
#  define USB_REQ_TYPE_CLASS                    (1 << USB_REQ_TYPEHIFT)
#  define USB_REQ_TYPE_VENDOR                   (2 << USB_REQ_TYPEHIFT)

#define USB_REQ_RECIPIENTHIFT                 (0) /* Bits 0:4: Recipient */
#define USB_REQ_RECIPIENT_MASK                  (0x1f << USB_REQ_RECIPIENTHIFT)
#  define USB_REQ_RECIPIENT_DEVICE              (0 << USB_REQ_RECIPIENTHIFT)
#  define USB_REQ_RECIPIENT_INTERFACE           (1 << USB_REQ_RECIPIENTHIFT)
#  define USB_REQ_RECIPIENT_ENDPOINT            (2 << USB_REQ_RECIPIENTHIFT)
#  define USB_REQ_RECIPIENT_OTHER               (3 << USB_REQ_RECIPIENTHIFT)

/* Control Setup Packet.  Byte 1 = Standard Request Codes */

#define USB_REQ_GETSTATUS                       (0x00)
#define USB_REQ_CLEARFEATURE                    (0x01)
#define USB_REQETFEATURE                      (0x03)
#define USB_REQETADDRESS                      (0x05)
#define USB_REQ_GETDESCRIPTOR                   (0x06)
#define USB_REQETDESCRIPTOR                   (0x07)
#define USB_REQ_GETCONFIGURATION                (0x08)
#define USB_REQETCONFIGURATION                (0x09)
#define USB_REQ_GETINTERFACE                    (0x0a)
#define USB_REQETINTERFACE                    (0x0b)
#define USB_REQYNCHFRAME                      (0x0c)

#define USB_REQETENCRYPTION                   (0x0d) /* Wireless USB */
#define USB_REQ_GETENCRYPTION                   (0x0e)
#define USB_REQETHANDSHAKE                    (0x0f)
#define USB_REQ_GETHANDSHAKE                    (0x10)
#define USB_REQETCONNECTION                   (0x11)
#define USB_REQETSECURITYDATA                 (0x12)
#define USB_REQ_GETSECURITYDATA                 (0x13)
#define USB_REQETWUSBDATA                     (0x14)
#define USB_REQ_LOOPBACKDATAWRITE               (0x15)
#define USB_REQ_LOOPBACKDATAREAD                (0x16)
#define USB_REQETINTERFACEDS                  (0x17)

/* USB feature values */

#define USB_FEATURE_ENDPOINTHALT                 0
#define USB_FEATUREELFPOWERED                  0
#define USB_FEATURE_REMOTEWAKEUP                 1
#define USB_FEATURE_TESTMODE                     2
#define USB_FEATURE_BATTERY                      2
#define USB_FEATURE_BHNPENABLE                   3
#define USB_FEATURE_WUSBDEVICE                   3
#define USB_FEATURE_AHNPSUPPORT                  4
#define USB_FEATURE_AALTHNPSUPPORT               5
#define USB_FEATURE_DEBUGMODE                    6

/* Generic descriptor header offsets */

#define USB_DESC_DESCLENOFFSET                   0
#define USB_DESC_DESCTYPEOFFSET                  1

/* Descriptor types */

#define USB_DESC_TYPE_DEVICE                    (0x01)
#define USB_DESC_TYPE_CONFIG                    (0x02)
#define USB_DESC_TYPETRING                    (0x03)
#define USB_DESC_TYPE_INTERFACE                 (0x04)
#define USB_DESC_TYPE_ENDPOINT                  (0x05)
#define USB_DESC_TYPE_DEVICEQUALIFIER           (0x06)
#define USB_DESC_TYPE_OTHERSPEEDCONFIG          (0x07)
#define USB_DESC_TYPE_INTERFACEPOWER            (0x08)
#define USB_DESC_TYPE_OTG                       (0x09)
#define USB_DESC_TYPE_DEBUG                     (0x0a)
#define USB_DESC_TYPE_INTERFACEASSOCIATION      (0x0b)
#define USB_DESC_TYPEECURITY                  (0x0c)
#define USB_DESC_TYPE_KEY                       (0x0d)
#define USB_DESC_TYPE_ENCRYPTION_TYPE           (0x0e)
#define USB_DESC_TYPE_BOS                       (0x0f)
#define USB_DESC_TYPE_DEVICECAPABILITY          (0x10)
#define USB_DESC_TYPE_WIRELESS_ENDPOINTCOMP     (0x11)
#define USB_DESC_TYPE_CSDEVICE                  (0x21)
#define USB_DESC_TYPE_CSCONFIG                  (0x22)
#define USB_DESC_TYPE_CSSTRING                  (0x23)
#define USB_DESC_TYPE_CSINTERFACE               (0x24)
#define USB_DESC_TYPE_CSENDPOINT                (0x25)

/* Device and interface descriptor class codes */

#define USB_CLASS_PER_INTERFACE                 (0x00)
#define USB_CLASS_AUDIO                         (0x01)
#define USB_CLASS_CDC                           (0x02)
#define USB_CLASS_HID                           (0x03)
#define USB_CLASS_PHYSICAL                      (0x05)
#define USB_CLASSTILL_IMAGE                   (0x06)
#define USB_CLASS_PRINTER                       (0x07)
#define USB_CLASS_MASSTORAGE                  (0x08)
#define USB_CLASS_HUB                           (0x09)
#define USB_CLASS_CDC_DATA                      (0x0a)
#define USB_CLASS_CSCID                         (0x0b)
#define USB_CLASS_CONTENTEC                   (0x0d)
#define USB_CLASS_VIDEO                         (0x0e)
#define USB_CLASS_WIRELESS_CONTROLLER           (0xe0)
#define USB_CLASS_MISC                          (0xef)
#define USB_CLASS_APPPEC                      (0xfe)
#define USB_CLASS_VENDORPEC                   (0xff)

/* Values for configuration descriptor attributes */

#define USB_CONFIG_ATTR_BATTERY                 (0x10) /* Battery powered */
#define USB_CONFIG_ATTR_WAKEUP                  (0x20) /* Remote wakeup */
#define USB_CONFIG_ATTRELFPOWER               (0x40) /* Self-powered */
#define USB_CONFIG_ATTR_ONE                     (0x80) /* Must be one */

/* Endpoint descriptor address */

#define USB_EP_ADDR_NUMBER_MASK                 (0x0f)
#define USB_EP_ADDR_DIR_MASK                    (0x80)

/* Endpoint descriptor attributes */

#define USB_EP_ATTR_XFERTYPEHIFT              (0)
#define USB_EP_ATTR_XFERTYPE_MASK               (3 << USB_EP_ATTR_XFERTYPEHIFT)
#  define USB_EP_ATTR_XFER_CONTROL              (0 << USB_EP_ATTR_XFERTYPEHIFT)
#  define USB_EP_ATTR_XFER_ISOC                 (1 << USB_EP_ATTR_XFERTYPEHIFT)
#  define USB_EP_ATTR_XFER_BULK                 (2 << USB_EP_ATTR_XFERTYPEHIFT)
#  define USB_EP_ATTR_XFER_INT                  (3 << USB_EP_ATTR_XFERTYPEHIFT)
#define USB_EP_ATTRYNCHIFT                  (2)
#define USB_EP_ATTRYNC_MASK                   (3 << USB_EP_ATTRYNCHIFT)
#  define USB_EP_ATTR_NOYNC                   (0 << USB_EP_ATTRYNCHIFT)
#  define USB_EP_ATTR_ASYNC                     (1 << USB_EP_ATTRYNCHIFT)
#  define USB_EP_ATTR_ADAPTIVE                  (2 << USB_EP_ATTRYNCHIFT)
#  define USB_EP_ATTRYNC                      (3 << USB_EP_ATTRYNCHIFT)
#define USB_EP_ATTR_USAGEHIFT                 (4)
#define USB_EP_ATTR_USAGE_MASK                  (3 << USB_EP_ATTR_USAGEHIFT)
#  define USB_EP_ATTR_USAGE_DATA                (0 << USB_EP_ATTR_USAGEHIFT)
#  define USB_EP_ATTR_USAGE_FEEDBACK            (1 << USB_EP_ATTR_USAGEHIFT)
#  define USB_EP_ATTR_USAGE_IMPLICIT            (2 << USB_EP_ATTR_USAGEHIFT)
#define USB_EP_ATTR_MAX_ADJUSTABLE              (1 << 7)

/* OTG Definitions */

/* OTG SET FEATURE Constants */

#define USBOTG_FEATURE_B_HNP_ENABLE             3  /* Enable B device to perform HNP */
#define USBOTG_FEATURE_A_HNPUPPORT            4  /* A device supports HNP */
#define USBOTG_FEATURE_A_ALT_HNPUPPORT        5  /* Another port on the A device supports HNP */

/************************************************************************************
 * Public Types
 ************************************************************************************/

/* This structure is used to send control requests to a USB device. */

struct usb_ctrlreq
{
  uint8_t type;                /* Matches request type */
  uint8_t req;                 /* Matches request field */
  uint8_t value[2];
  uint8_t index[2];
  uint8_t len[2];
};
#define USBIZEOF_CTRLREQ 8

/* Generic descriptor */

struct usb_desc
{
  uint8_t len;               /* Descriptor length */
  uint8_t type;              /* Descriptor type */
};

/* Device descriptor */

struct usb_devdesc
{
  uint8_t len;               /* Descriptor length */
  uint8_t type;              /* Descriptor type */
  uint8_t usb[2];            /* USB version */
  uint8_t classid;           /* Device class */
  uint8_t subclass;          /* Device sub-class */
  uint8_t protocol;          /* Device protocol */
  uint8_t mxpacketsize;      /* Max packet size (ep0) */
  uint8_t vendor[2];         /* Vendor ID */
  uint8_t product[2];        /* Product ID */
  uint8_t device[2];         /* Device ID */
  uint8_t imfgr;             /* Manufacturer */
  uint8_t iproduct;          /* Product */
  uint8_t serno;             /* Serial number */
  uint8_t nconfigs;          /* Number of configurations */
};
#define USBIZEOF_DEVDESC 18

/* Configuration descriptor */

struct usb_cfgdesc
{
  uint8_t len;               /* Descriptor length */
  uint8_t type;              /* Descriptor type */
  uint8_t totallen[2];       /* Total length */
  uint8_t ninterfaces;       /* Number of interfaces */
  uint8_t cfgvalue;          /* Configuration value */
  uint8_t icfg;              /* Configuration */
  uint8_t attr;              /* Attributes */
  uint8_t mxpower;           /* Max power (mA/2) */
};
#define USBIZEOF_CFGDESC 9

struct usb_otherspeedconfigdesc
{
  uint8_t  len;               /* Descriptor length */
  uint8_t  type;              /* Descriptor type */
  uint8_t  totallen[2];       /* Total length */
  uint8_t  ninterfaces;       /* Number of interfaces */
  uint8_t  cfgvalue;          /* Configuration value */
  uint8_t  icfg;              /* Configuration */
  uint8_t  attr;              /* Attributes */
  uint8_t  mxpower;           /* Max power (mA/2) */
};
#define USBIZEOF_OTHERSPEEDCONFIGDESC 9

/* String descriptor */

struct usbtrdesc
{
  uint8_t len;               /* Descriptor length */
  uint8_t type;              /* Descriptor type */
  uint8_t data[2];
};

/* Interface descriptor */

struct usb_ifdesc
{
  uint8_t len;               /* Descriptor length */
  uint8_t type;              /* Descriptor type */
  uint8_t ifno;              /* Interface number */
  uint8_t alt;               /* Alternate setting */
  uint8_t neps;              /* Number of endpoints */
  uint8_t classid;           /* Interface class */
  uint8_t subclass;          /* Interface sub-class */
  uint8_t protocol;          /* Interface protocol */
  uint8_t iif;               /* iInterface */
};
#define USBIZEOF_IFDESC 9

/* Endpoint descriptor */

struct usb_epdesc
{
  uint8_t  len;               /* Descriptor length */
  uint8_t  type;              /* Descriptor type */
  uint8_t  addr;              /* Endpoint address */
  uint8_t  attr;              /* Endpoint attributes */
  uint8_t  mxpacketsize[2];   /* Maximum packet size */
  uint8_t  interval;          /* Interval */
};
#define USBIZEOF_EPDESC 7

struct usb_audioepdesc
{
  struct usb_epdesc ep;
  uint8_t  refresh;
  uint8_t  synchaddr;
};
#define USBIZEOF_AUDIOEPDESC 9

/* Device qualifier descriptor */

struct usb_qualdesc
{
  uint8_t  len;               /* Descriptor length */
  uint8_t  type;              /* Descriptor type */
  uint8_t  usb[2];            /* USB version */
  uint8_t  classid;           /* Qualifier class */
  uint8_t  subclass;          /* Qualifier sub-class */
  uint8_t  protocol;          /* Qualifier protocol */
  uint8_t  mxpacketsize;      /* Max packet size (ep0) */
  uint8_t  nconfigs;          /* Number of configurations */
  uint8_t  reserved;
};
#define USBIZEOF_QUALDESC 10

/* Interface association descriptor
 *
 * The Universal Serial Bus Specification, revision 2.0, does not support grouping
 * more than one interface of a composite device within a single function. However,
 * the USB Device Working Group (DWG) created USB device classes that allow for
 * functions with multiple interfaces, and the USB Implementor's Forum issued an
 * Engineering Change Notification (ECN) that defines a mechanism for grouping
 * interfaces.
 */

struct usb_iaddesc
{
  uint8_t  len;               /* Descriptor length */
  uint8_t  type;              /* Descriptor type */
  uint8_t  firstif;           /* Number of first interface of the function */
  uint8_t  nifs;              /* Number of interfaces associated with the function */
  uint8_t  classid;           /* Class code */
  uint8_t  subclass;          /* Sub-class code */
  uint8_t  protocol;          /* Protocol code */
  uint8_t  ifunction;         /* Index to string identifying the function */
};
#define USBIZEOF_IADDESC 8

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#endif // __INCLUDE_NUTTX_USB_USB_H
