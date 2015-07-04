#include <asm/delay.h>
#include <phabos/usb/hcd.h>
#include <phabos/utils.h>

#include <errno.h>
#include <string.h>

static atomic_t dev_id;

enum usb_descritor_type {
    USB_DESCRIPTOR_HUB = 0x29,
};

enum usb_device_class {
    USB_DEVICE_CLASS_HUB = 0x9,
};

enum {
    USB_GET_STATUS          = 0,
    USB_CLEAR_FEATURE       = 1,
    USB_SET_FEATURE         = 3,
    USB_SET_ADDRESS         = 5,
    USB_GET_DESCRIPTOR      = 6,
    USB_SET_DESCRIPTOR      = 7,
    USB_GET_CONFIGURATION   = 8,
    USB_SET_CONFIGURATION   = 9,
    USB_GET_INTERFACE       = 10,
    USB_SET_INTERFACE       = 11,
    USB_SYNCH_FRAME         = 12,
};

enum usb_descriptor_type {
    USB_DESCRIPTOR_DEVICE                       = 1,
    USB_DESCRIPTOR_CONFIGURATION                = 2,
    USB_DESCRIPTOR_STRING                       = 3,
    USB_DESCRIPTOR_INTERFACE                    = 4,
    USB_DESCRIPTOR_ENDPOINT                     = 5,
    USB_DESCRIPTOR_DEVICE_QUALIFIER             = 6,
    USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION    = 7,
    USB_DESCRIPTOR_INTERFACE_POWER              = 8,
};

enum usb_hub_class_request {
    USB_CLEAR_HUB_FEATURE   = 0x2000 | USB_CLEAR_FEATURE,
    USB_CLEAR_PORT_FEATURE  = 0x2300 | USB_CLEAR_FEATURE,
    USB_CLEAR_TT_BUFFER     = 0x2308,
    USB_GET_HUB_DESCRIPTOR  = 0xa000 | USB_GET_DESCRIPTOR,
    USB_GET_HUB_STATUS      = 0xa000 | USB_GET_STATUS,
    USB_GET_PORT_STATUS     = 0xa300 | USB_GET_STATUS,
    USB_RESET_TT            = 0x2309,
    USB_SET_HUB_DESCRIPTOR  = 0x2000 | USB_SET_FEATURE,
    USB_SET_PORT_FEATURE    = 0x2300 | USB_SET_FEATURE,
    USB_GET_TT_STATE        = 0xa30a,
    USB_STOP_TT             = 0x230b,
};

enum usb_hub_feature_selector {
    C_HUB_LOCAL_POWER   = 0,
    C_HUB_OVER_CURRENT  = 1,
};

enum usb_hub_port_feature_selector {
    PORT_CONNECTION     = 0,
    PORT_ENABLE         = 1,
    PORT_SUSPEND        = 2,
    PORT_OVER_CURRENT   = 3,
    PORT_RESET          = 4,
    PORT_POWER          = 8,
    PORT_LOW_SPEED      = 9,
    C_PORT_CONNECTION   = 16,
    C_PORT_ENABLE       = 17,
    C_PORT_SUSPEND      = 18,
    C_PORT_OVER_CURRENT = 19,
    C_PORT_RESET        = 20,
    PORT_TEST           = 21,
    PORT_INDICATOR      = 22,
};

struct usb_hub_descriptor {
    uint8_t bDescLength;
    uint8_t bDescriptorType;
    uint8_t bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t bPwrOn2PwrGood;
    uint8_t bHubContrCurrent;
};

struct usb_device_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
};

struct usb_config_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
};

struct usb_string_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bString[0];
};

static void get_descriptor_callback(struct urb *urb)
{
    struct usb_device_descriptor *desc = urb->buffer;

    kprintf("%s() = %d\n", __func__, urb->status);

    if (urb->status)
        return;

    if (desc->bDeviceClass == USB_DEVICE_CLASS_HUB) {
        kprintf("usb: new hub found\n");
    } else {
        kprintf("usb: unknown device found\n");
    }
}

static void set_address_callback(struct urb *urb)
{
    kprintf("%s() = %d\n", __func__, urb->status);
}

static void set_configuration_callback(struct urb *urb)
{
    kprintf("%s() = %d\n", __func__, urb->status);
}

static int usb_get_descriptor(struct usb_hcd *hcd, int type, int speed,
                              int devnum, int ttport)
{
    struct urb *urb = urb_create();
    if (!urb)
        return -ENOMEM;

    urb->hcd = hcd;
    urb->buffer = malloc(64);
    urb->complete = get_descriptor_callback;
    urb->pipe = (USB_HOST_PIPE_CONTROL << 30) | (devnum << 8) | USB_HOST_DIR_IN;
    urb->dev_speed = speed;
    urb->devnum = devnum;
    urb->dev_ttport = ttport;
    urb->length = 64;
    urb->maxpacket = 0x40;

    urb->setup_packet[0] = 0x80,
    urb->setup_packet[1] = USB_GET_DESCRIPTOR,
    urb->setup_packet[2] = 0;
    urb->setup_packet[3] = type;
    urb->setup_packet[4] = 0;
    urb->setup_packet[5] = 0;
    urb->setup_packet[6] = 64;
    urb->setup_packet[7] = 0;

    return hcd->driver->urb_enqueue(hcd, urb);
}

static int usb_set_address(struct usb_hcd *hcd, int devnum, int speed,
                           int ttport)
{
    struct urb *urb = urb_create();
    if (!urb)
        return -ENOMEM;

    urb->complete = set_address_callback;
    urb->pipe = (USB_HOST_PIPE_CONTROL << 30) | USB_HOST_DIR_OUT;
    urb->dev_speed = speed;
    urb->devnum = 0;
    urb->dev_ttport = ttport;
    urb->length = 0;
    urb->maxpacket = 0x40;

    urb->setup_packet[0] = 0,
    urb->setup_packet[1] = USB_SET_ADDRESS,
    urb->setup_packet[2] = devnum;
    urb->setup_packet[3] = 0;
    urb->setup_packet[4] = 0;
    urb->setup_packet[5] = 0;
    urb->setup_packet[6] = 0;
    urb->setup_packet[7] = 0;

    return hcd->driver->urb_enqueue(hcd, urb);
}

static int usb_set_configuration(struct usb_hcd *hcd, int configuration,
                                 int devnum, int speed, int ttport)
{
    struct urb *urb = urb_create();
    if (!urb)
        return -ENOMEM;

    urb->complete = set_configuration_callback;
    urb->pipe = (USB_HOST_PIPE_CONTROL << 30) | (devnum << 8) | USB_HOST_DIR_OUT;
    urb->dev_speed = speed;
    urb->devnum = devnum;
    urb->dev_ttport = ttport;
    urb->length = 0;
    urb->maxpacket = 0x40;

    urb->setup_packet[0] = 0,
    urb->setup_packet[1] = USB_SET_CONFIGURATION,
    urb->setup_packet[2] = configuration;
    urb->setup_packet[3] = 0;
    urb->setup_packet[4] = 0;
    urb->setup_packet[5] = 0;
    urb->setup_packet[6] = 0;
    urb->setup_packet[7] = 0;

    return hcd->driver->urb_enqueue(hcd, urb);
}

void urb_enqueue_sync_callback(struct urb *urb)
{
    kprintf("%s() = %d\n", __func__, urb->status);
    semaphore_up(&urb->semaphore);
}

int urb_enqueue_sync(struct usb_hcd *hcd, struct urb *urb)
{
    int retval;

    urb->complete = urb_enqueue_sync_callback;

    retval = hcd->driver->urb_enqueue(hcd, urb);
    kprintf("%s() = %d\n", __func__, retval);

    semaphore_down(&urb->semaphore);
    return retval;
}

void get_hub_descriptor_callback(struct urb *urb);

static struct urb* usb_hub_control(struct usb_hcd *hcd, int devnum, int speed,
                                   int ttport, uint16_t typeReq, uint16_t wValue,
                                   uint16_t wIndex, uint16_t wLength, uint32_t *status)
{
    struct urb *urb = urb_create();
    if (!urb)
        return -ENOMEM;

    int pipedir = USB_HOST_DIR_OUT;
    if (wLength)
        pipedir = USB_HOST_DIR_IN;

    urb->hcd = hcd;
    urb->buffer = status;
    urb->complete = get_hub_descriptor_callback;
    urb->pipe = (USB_HOST_PIPE_CONTROL << 30) | (devnum << 8) | pipedir;
    urb->dev_speed = speed;
    urb->devnum = devnum;
    urb->dev_ttport = ttport;
    urb->length = wLength ? 64 : 0;
    urb->maxpacket = 0x40;
    urb->flags = 0;

    urb->setup_packet[0] = typeReq >> 8,
    urb->setup_packet[1] = typeReq & 0xff,
    urb->setup_packet[2] = wValue & 0xff;
    urb->setup_packet[3] = wValue >> 8;
    urb->setup_packet[4] = wIndex & 0xff;
    urb->setup_packet[5] = wIndex >> 8;
    urb->setup_packet[6] = wLength & 0xff;
    urb->setup_packet[7] = wLength >> 8;

    int retval = urb_enqueue_sync(hcd, urb);
    kprintf("%s() = %d\n", __func__, retval);
    return urb;
}

    static int usb_get_hub_descriptor(struct usb_hcd *hcd, int type, int speed,
                                int devnum, int ttport);

void get_hub_descriptor_callback(struct urb *urb)
{
    struct usb_hcd *hcd = urb->hcd;
    struct usb_hub_descriptor *hub_desc = urb->buffer;
    uint32_t status;
    int retval;

    if (urb->status)
        return;

    kprintf("%s: found new hub with %u ports.\n", hcd->device.name,
                                                  hub_desc->bNbrPorts);

#if 0
    USB_GET_STATUS          = 0,
    USB_CLEAR_FEATURE       = 1,
    USB_SET_FEATURE         = 3,
    USB_SET_ADDRESS         = 5,
    USB_GET_DESCRIPTOR      = 6,
    USB_SET_DESCRIPTOR      = 7,
    USB_GET_CONFIGURATION   = 8,
    USB_SET_CONFIGURATION   = 9,
    USB_GET_INTERFACE       = 10,
    USB_SET_INTERFACE       = 11,
    USB_SYNCH_FRAME         = 12,
#endif

    for (int i = 1; i <= hub_desc->bNbrPorts; i++) {
        usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_GET_HUB_STATUS, 0, i, 4, &status);

        kprintf("Yop\n");

        if (!(status & (1 << PORT_CONNECTION)))
            continue;

        kprintf("Port status: %X\n", status);

        usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_SET_PORT_FEATURE, PORT_ENABLE, i, 0, NULL);

        mdelay(hub_desc->bPwrOn2PwrGood * 2);

        usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_GET_PORT_STATUS, 0, i, 4, &status);
        kprintf("Port status: %X\n", status);

        usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_SET_PORT_FEATURE, PORT_POWER, i, 0, NULL);

        mdelay(hub_desc->bPwrOn2PwrGood * 2);

        usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_GET_PORT_STATUS, 0, i, 4, &status);
        kprintf("Port status: %X\n", status);

        usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_SET_PORT_FEATURE, PORT_RESET, i, 0, NULL);

        usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_GET_PORT_STATUS, 0, i, 4, &status);
        kprintf("Port status: %X\n", status);



#if 0
        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, &status);
        if (retval)
            continue;

        if (!(status & (1 << PORT_CONNECTION)))
            continue;

        kprintf("Port status: %X\n", status);

        retval = hcd->driver->hub_control(hcd, USB_SET_PORT_FEATURE, PORT_ENABLE, i, 0, NULL);

        mdelay(hub_desc->bPwrOn2PwrGood * 2);

        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, &status);
        kprintf("Port status: %X\n", status);

        retval = hcd->driver->hub_control(hcd, USB_SET_PORT_FEATURE, PORT_POWER, i, 0, NULL);

        mdelay(hub_desc->bPwrOn2PwrGood * 2);

        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, &status);
        kprintf("Port status: %X\n", status);

        retval = hcd->driver->hub_control(hcd, USB_SET_PORT_FEATURE, PORT_RESET, i, 0, NULL);

        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, &status);
        kprintf("Port status: %X\n", status);

        usb_enumerate_device(hcd);
#endif
    }
}

static int usb_get_hub_descriptor(struct usb_hcd *hcd, int type, int speed,
                              int devnum, int ttport)
{
    struct urb *urb = urb_create();
    if (!urb)
        return -ENOMEM;

    urb->hcd = hcd;
    urb->buffer = malloc(64);
    urb->complete = get_hub_descriptor_callback;
    urb->pipe = (USB_HOST_PIPE_CONTROL << 30) | (devnum << 8) | USB_HOST_DIR_IN;
    urb->dev_speed = speed;
    urb->devnum = devnum;
    urb->dev_ttport = ttport;
    urb->length = 64;
    urb->maxpacket = 0x40;

    urb->setup_packet[0] = 0xa0,
    urb->setup_packet[1] = USB_GET_DESCRIPTOR,
    urb->setup_packet[2] = 0;
    urb->setup_packet[3] = 0;
    urb->setup_packet[4] = 0;
    urb->setup_packet[5] = 0;
    urb->setup_packet[6] = 64;
    urb->setup_packet[7] = 0;

    return hcd->driver->urb_enqueue(hcd, urb);
}

static int usb_enumerate_hub(struct usb_hcd *hcd)
{
    struct urb *urb;
    //usb_get_hub_descriptor(hcd, 0, USB_SPEED_HIGH, 2, 0);
    char buf[64];
    urb = usb_hub_control(hcd, 2, USB_SPEED_HIGH, 0, USB_GET_HUB_DESCRIPTOR, 0, 0, 0x40, &buf);
    kprintf("cool\n");
    get_hub_descriptor_callback(urb);

    return 0;
}

static int usb_enumerate_device(struct usb_hcd *hcd)
{
    usb_get_descriptor(hcd, USB_DESCRIPTOR_DEVICE, USB_SPEED_HIGH, 0, 0);
    usb_set_address(hcd, 2, USB_SPEED_HIGH, 0);
    usb_set_configuration(hcd, 1, 2, USB_SPEED_HIGH, 0);
    usb_get_descriptor(hcd, USB_DESCRIPTOR_DEVICE, USB_SPEED_HIGH, 2, 0);

    usb_enumerate_hub(hcd);

    return 0;
}

static int usb_enumerate_bus(struct usb_hcd *hcd)
{
    int retval;
    char buf[64];
    struct usb_device_descriptor *device_desc =
        (struct usb_device_descriptor*) buf;
    struct usb_hub_descriptor *hub_desc = (struct usb_hub_descriptor*) buf;
    uint32_t status;

    RET_IF_FAIL(hcd, -EINVAL);
    RET_IF_FAIL(hcd->driver, -EINVAL);
    RET_IF_FAIL(hcd->driver->start, -EINVAL);
    RET_IF_FAIL(hcd->driver->hub_control, -EINVAL);

    retval = hcd->driver->start(hcd);
    if (retval)
        return retval;

    retval = hcd->driver->hub_control(hcd, USB_GET_HUB_DESCRIPTOR, 0, 0, 2, buf);
    if (retval)
        return retval;

    if (device_desc->bDescriptorType != USB_DESCRIPTOR_HUB)
        return -EINVAL;

    if (device_desc->bLength > ARRAY_SIZE(buf))
        return -ENOMEM;

    retval = hcd->driver->hub_control(hcd, USB_GET_HUB_DESCRIPTOR, 0, 0, device_desc->bLength, buf);
    if (retval)
        return retval;

    kprintf("%s: found new hub with %u ports.\n", hcd->device.name,
                                                  hub_desc->bNbrPorts);

    for (int i = 1; i <= hub_desc->bNbrPorts; i++) {
        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, (char*) &status);
        if (retval)
            continue;

        if (!(status & (1 << PORT_CONNECTION)))
            continue;

        kprintf("Port status: %X\n", status);

        retval = hcd->driver->hub_control(hcd, USB_SET_PORT_FEATURE, PORT_ENABLE, i, 0, NULL);

        mdelay(hub_desc->bPwrOn2PwrGood * 2);

        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, (char*) &status);
        kprintf("Port status: %X\n", status);

        retval = hcd->driver->hub_control(hcd, USB_SET_PORT_FEATURE, PORT_POWER, i, 0, NULL);

        mdelay(hub_desc->bPwrOn2PwrGood * 2);

        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, (char*) &status);
        kprintf("Port status: %X\n", status);

        retval = hcd->driver->hub_control(hcd, USB_SET_PORT_FEATURE, PORT_RESET, i, 0, NULL);

        retval = hcd->driver->hub_control(hcd, USB_GET_PORT_STATUS, 0, i, 4, (char*) &status);
        kprintf("Port status: %X\n", status);

        usb_enumerate_device(hcd);
    }

    return 0;
}

void enumerate_everything(void *data)
{
    struct usb_hcd *hcd = data;

    int retval = usb_enumerate_bus(hcd);
    atomic_init(&dev_id, 1);

    if (retval)
        kprintf("usb: enumeration error: %d %s\n", retval, strerror(retval));

    while (1);
}

int usb_hcd_register(struct usb_hcd *hcd)
{
    task_run(enumerate_everything, hcd, 0);
    return 0;
}


