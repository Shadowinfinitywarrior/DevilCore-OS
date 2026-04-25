/*
 * DevilCore OS — USB Human Interface Device (HID)
 */
#ifndef DC_USB_HID_H
#define DC_USB_HID_H

#include "../../include/types.h"
#include "usb.h"

/* Initialize the generic HID abstraction layer */
void usb_hid_init(void);

/* Register a USB Keyboard or Mouse */
void usb_hid_register_device(usb_device_t *dev);

#endif /* DC_USB_HID_H */
