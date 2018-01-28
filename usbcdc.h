#ifndef __STM32_VSERPOG_USB_CDC_H__
#define __STM32_VSERPOG_USB_CDC_H__

#include <stdint.h>
#include <stddef.h>

#define USBCDC_PKT_SIZE_DAT 64
#define USBCDC_PKT_SIZE_INT 16

extern volatile bool usb_ready;

extern volatile char rbuf[USBCDC_PKT_SIZE_DAT * 2];
extern volatile uint32_t rbuf_len;

void     usbcdc_init(void);
uint16_t usbcdc_write(void *buf, size_t len);
void usb_int_relay(void);

#endif /* __STM32_VSERPOG_USB_CDC_H__ */
