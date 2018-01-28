#include <stdlib.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/desig.h>

#define UID_LEN  (12 * 2 + 1) /* 12-byte, each byte turnned into 2-byte hex, then '\0'. */

#define DEV_VID    0x0483 /* ST Microelectronics */
#define DEV_PID    0x5711 /* STM32F103 */
#define DEV_VER    0x0009 /* 0.9 */

#define EP_INT     0x83
#define EP_OUT     0x82
#define EP_IN      0x01

#define STR_MAN    0x01
#define STR_PROD   0x02
#define STR_SER    0x03

#include "usbcdc.h"

static const struct usb_device_descriptor dev = {
  .bLength            = USB_DT_DEVICE_SIZE,
  .bDescriptorType    = USB_DT_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = USB_CLASS_CDC,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = USBCDC_PKT_SIZE_DAT,
  .idVendor           = DEV_VID,
  .idProduct          = DEV_PID,
  .bcdDevice          = DEV_VER,
  .iManufacturer      = STR_MAN,
  .iProduct           = STR_PROD,
  .iSerialNumber      = STR_SER,
  .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
  .bLength            = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType    = USB_DT_ENDPOINT,
  .bEndpointAddress   = EP_INT,
  .bmAttributes       = USB_ENDPOINT_ATTR_INTERRUPT,
  .wMaxPacketSize     = USBCDC_PKT_SIZE_INT,
  .bInterval          = 255, //interrupt every 255 msec
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
  .bLength            = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType    = USB_DT_ENDPOINT,
  .bEndpointAddress   = EP_IN,
  .bmAttributes       = USB_ENDPOINT_ATTR_BULK,
  .wMaxPacketSize     = USBCDC_PKT_SIZE_DAT,
  .bInterval          = 1,
}, {
  .bLength            = USB_DT_ENDPOINT_SIZE,
  .bDescriptorType    = USB_DT_ENDPOINT,
  .bEndpointAddress   = EP_OUT,
  .bmAttributes       = USB_ENDPOINT_ATTR_BULK,
  .wMaxPacketSize     = USBCDC_PKT_SIZE_DAT,
  .bInterval          = 1,
}};

static const struct {
  struct usb_cdc_header_descriptor header;
  struct usb_cdc_call_management_descriptor call_mgmt;
  struct usb_cdc_acm_descriptor acm;
  struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
  .header = {
    .bFunctionLength    = sizeof(struct usb_cdc_header_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
    .bcdCDC = 0x0110,
  },
  .call_mgmt = {
    .bFunctionLength    = sizeof(struct usb_cdc_call_management_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
    .bmCapabilities     = 0,
    .bDataInterface     = 1,
  },
  .acm = {
    .bFunctionLength    = sizeof(struct usb_cdc_acm_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_ACM,
    .bmCapabilities     = 0,
  },
  .cdc_union = {
    .bFunctionLength    = sizeof(struct usb_cdc_union_descriptor),
    .bDescriptorType    = CS_INTERFACE,
    .bDescriptorSubtype = USB_CDC_TYPE_UNION,
    .bControlInterface  = 0,
    .bSubordinateInterface0 = 1,
   },
};

static const struct usb_interface_descriptor comm_iface[] = {{
  .bLength              = USB_DT_INTERFACE_SIZE,
  .bDescriptorType      = USB_DT_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 1,
  .bInterfaceClass      = USB_CLASS_CDC,
  .bInterfaceSubClass   = USB_CDC_SUBCLASS_ACM,
  .bInterfaceProtocol   = USB_CDC_PROTOCOL_AT,
  .iInterface           = 0,

  .endpoint             = comm_endp,

  .extra                = &cdcacm_functional_descriptors,
  .extralen             = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
  .bLength              = USB_DT_INTERFACE_SIZE,
  .bDescriptorType      = USB_DT_INTERFACE,
  .bInterfaceNumber     = 1,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 2,
  .bInterfaceClass      = USB_CLASS_DATA,
  .bInterfaceSubClass   = 0,
  .bInterfaceProtocol   = 0,
  .iInterface           = 0,

  .endpoint             = data_endp,
}};

static const struct usb_interface ifaces[] = {{
  .num_altsetting       = 1,
  .altsetting           = comm_iface,
}, {
  .num_altsetting       = 1,
  .altsetting           = data_iface,
}};

static const struct usb_config_descriptor config = {
  .bLength              = USB_DT_CONFIGURATION_SIZE,
  .bDescriptorType      = USB_DT_CONFIGURATION,
  .wTotalLength         = 0,
  .bNumInterfaces       = 2,
  .bConfigurationValue  = 1,
  .iConfiguration       = 0,
  .bmAttributes         = 0x80,
  .bMaxPower            = 0x32,

  .interface            = ifaces,
};

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128];

static int cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
    uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
  uint32_t bitrate;

  switch (req->bRequest) {
  case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
    /*
     * This Linux cdc_acm driver requires this to be implemented
     * even though it's optional in the CDC spec, and we don't
     * advertise it in the ACM functional descriptor.
     */
    struct usb_cdc_notification *notif = (void *)*buf;

    /* We echo signals back to host as notification. */
    notif->bmRequestType = req->bmRequestType | 0x80;
    notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
    notif->wValue        = 0;
    notif->wIndex        = 0;
    notif->wLength       = 2;
    (*buf)[8] 			 = req->wValue & 3; // State of Carriers Tx Rx
    (*buf)[9] 			 = 0;
    usbd_ep_write_packet(usbd_dev, EP_INT, buf, 10);
    return USBD_REQ_HANDLED;
  }
  case USB_CDC_REQ_SET_LINE_CODING:
    if (*len > sizeof(struct usb_cdc_line_coding))
      return USBD_REQ_NOTSUPP;
//    bitrate = ((*buf)[3]<<24) | ((*buf)[2]<<16) | ((*buf)[1]<<8) | (*buf)[0];
//    printf("Bitrate: %d bps\\r", bitrate);
//   	printf("Stop bits val: %d\\r", (*buf)[4] );
//   	printf("Parity val: %d\\r", (*buf)[5] );
//   	printf("DataBits: %d\\r", (*buf)[6] );
    return USBD_REQ_HANDLED;
  }
  return USBD_REQ_NEXT_CALLBACK;
}

volatile bool usb_ready = false;

static void cdcacm_reset(void) {
  usb_ready = false;
}

static void cdcacm_interrupt_cb(usbd_device *usbd_dev, uint8_t ep) {
	(void) usbd_dev;
}

volatile char rbuf[USBCDC_PKT_SIZE_DAT * 2];
volatile uint32_t rbuf_len = 0;

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
  (void)ep;
  uint32_t len;

  len = usbd_ep_read_packet(usbd_dev, EP_IN, (char*)&rbuf[rbuf_len], sizeof(rbuf) - rbuf_len);

  rbuf_len += len;
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue) {
  usbd_ep_setup(usbd_dev, EP_IN , USB_ENDPOINT_ATTR_BULK, USBCDC_PKT_SIZE_DAT, cdcacm_data_rx_cb);
  usbd_ep_setup(usbd_dev, EP_OUT, USB_ENDPOINT_ATTR_BULK, USBCDC_PKT_SIZE_DAT, NULL);
  usbd_ep_setup(usbd_dev, EP_INT, USB_ENDPOINT_ATTR_INTERRUPT, USBCDC_PKT_SIZE_INT, cdcacm_interrupt_cb);

  usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        cdcacm_control_request);

  if (wValue > 0) {
    usb_ready = true;
  }
}

static usbd_device *usbd_dev; /* Just a pointer, need not to be volatile. */

static char serial[UID_LEN];

/* Vendor, device, version. */
static const char *usb_strings[] = {
  "flashrom",
  "STM32 virtual serprog for flashrom",
  serial,
};

void usbcdc_init(void) {
  desig_get_unique_id_as_string(serial, UID_LEN);
  usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
  usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
  usbd_register_reset_callback(usbd_dev, cdcacm_reset);

  /* NOTE: Must be called after USB setup since this enables calling usbd_poll(). */
  /* NVIC_USB_HP_CAN_TX_IRQ */
  nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
  nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);
}

/* Application-level functions */
uint16_t usbcdc_write(void *buf, size_t len) {
  uint16_t ret;

  /* Blocking write */
  while (0 == (ret = usbd_ep_write_packet(usbd_dev, EP_OUT, buf, len)));
  return ret;
}


/* Interrupts */

void usb_int_relay(void) {
  /* Need to pass a parameter... otherwise just alias it directly. */
  usbd_poll(usbd_dev);
}

void usb_wakeup_isr(void)
__attribute__ ((alias ("usb_int_relay")));

void usb_hp_can_tx_isr(void)
__attribute__ ((alias ("usb_int_relay")));

void usb_lp_can_rx0_isr(void)
__attribute__ ((alias ("usb_int_relay")));
