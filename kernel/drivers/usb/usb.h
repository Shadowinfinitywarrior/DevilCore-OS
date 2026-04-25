/*
 * DevilCore OS — USB Core Subsystem
 */
#ifndef DC_USB_H
#define DC_USB_H

#include "../../include/types.h"

/* USB Standard Descriptor Types */
#define USB_DESC_DEVICE         1
#define USB_DESC_CONFIGURATION  2
#define USB_DESC_STRING         3
#define USB_DESC_INTERFACE      4
#define USB_DESC_ENDPOINT       5
#define USB_DESC_HID            0x21

/* Core USB Device Descriptor */
#pragma pack(push, 1)
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} usb_device_descriptor_t;
#pragma pack(pop)

/* Subsystem Generic Device Abstraction */
typedef struct usb_device {
    uint8_t address;
    uint8_t port;
    usb_device_descriptor_t desc;
    struct usb_device *next;
} usb_device_t;

/* Initialize USB Core Subsystem */
void usb_init(void);

#endif /* DC_USB_H */
