/*
 * DevilCore OS — Device Drivers & I/O Diagnostics
 * Items 51-60
 */

#include "diag.h"
#include "../drivers/pci.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"
#include "../drivers/timer.h"
#include "../drivers/pic.h"
#include "../drivers/ata.h"
#include "../drivers/usb/usb.h"
#include "../drivers/usb/usb_hid.h"
#include "../include/ports.h"

/* ================================================================
 * REGISTERED DEVICE DRIVERS (Item 51)
 * ================================================================ */
static void diag_io_drivers(diag_output_t *out) {
    diag_header(out, "Registered Device Drivers");

    driver_info_t drivers[32];
    int ndrivers = driver_get_registered(drivers, 32);

    if (ndrivers == 0) {
        diag_printf_user(out, "  No drivers registered.\n");
        return;
    }

    diag_printf_user(out, "  #  Name              Type     IRQ  State   Addr\n");
    diag_printf_user(out, "  -  ----------------  -------- ---  ------  ------------------\n");

    for (int i = 0; i < ndrivers; i++) {
        const char *state = "OK";
        if (drivers[i].flags & DRIVER_FLAG_ERROR) state = "ERROR";
        if (drivers[i].flags & DRIVER_FLAG_DISABLED) state = "DISABLED";
        if (drivers[i].flags & DRIVER_FLAG_SUSPENDED) state = "SUSPEND";

        diag_printf_user(out, "  %-2d  %-16s  %-8s  %3d  %-6s  0x%016llX\n",
            i,
            drivers[i].name,
            drivers[i].type,
            drivers[i].irq,
            state,
            drivers[i].address);
    }

    diag_printf_user(out, "\n  Total drivers: %d\n", ndrivers);
    diag_printf_user(out, "  Active: %d\n", driver_count_active());
}

/* ================================================================
 * KEYBOARD SCAN CODE BUFFER (Item 52)
 * ================================================================ */
static void diag_io_keyboard(diag_output_t *out) {
    diag_header(out, "Keyboard Scan Code Buffer");

    keyboard_state_t kbd;
    keyboard_get_state(&kbd);

    diag_printf_user(out, "  Keyboard State: %s\n", kbd.present ? "Present" : "Not detected");
    diag_printf_user(out, "  LEDs: %s%s%s\n",
        kbd.leds & KBD_LED_NUM ? "NUM " : "",
        kbd.leds & KBD_LED_CAPS ? "CAPS " : "",
        kbd.leds & KBD_LED_SCROLL ? "SCROLL" : "");
    diag_printf_user(out, "  Scan Code Set: %u\n", kbd.scan_code_set);
    diag_printf_user(out, "\n  Input Buffer:\n");
    diag_printf_user(out, "    Capacity: %u bytes\n", KBD_BUFFER_SIZE);
    diag_printf_user(out, "    Used:     %u bytes (%u keys)\n",
        kbd.buffer_used, kbd.buffer_used / 2);
    diag_printf_user(out, "    Pending Events: %u\n", kbd.pending_events);
    diag_printf_user(out, "\n  Last Scancodes (hex):\n");
    for (int i = 0; i < MIN(16, kbd.buffer_used/2); i++) {
        diag_printf_user(out, "    0x%02X ", kbd.scancode_buffer[i]);
        if ((i + 1) % 8 == 0) diag_printf_user(out, "\n");
    }
    diag_printf_user(out, "\n\n  Modifiers: ");
    if (kbd.modifiers & MOD_LSHIFT) diag_printf_user(out, "LSHIFT ");
    if (kbd.modifiers & MOD_RSHIFT) diag_printf_user(out, "RSHIFT ");
    if (kbd.modifiers & MOD_LCTRL)  diag_printf_user(out, "LCTRL ");
    if (kbd.modifiers & MOD_RCTRL)  diag_printf_user(out, "RCTRL ");
    if (kbd.modifiers & MOD_LALT)   diag_printf_user(out, "LALT ");
    if (kbd.modifiers & MOD_RALT)   diag_printf_user(out, "RALT ");
    diag_printf_user(out, "\n");
}

/* ================================================================
 * MOUSE MOVEMENT AND BUTTONS (Item 54)
 * ================================================================ */
static void diag_io_mouse(diag_output_t *out) {
    diag_header(out, "Mouse State");

    mouse_state_t mouse;
    mouse_get_state(&mouse);

    diag_printf_user(out, "  Position: X=%d, Y=%d\n", mouse.x, mouse.y);
    diag_printf_user(out, "  Movement Delta: X=%+d, Y=%+d\n", mouse.dx, mouse.dy);
    diag_printf_user(out, "  Scroll Delta: %d\n", mouse.scroll_delta);
    diag_printf_user(out, "\n  Buttons: ");
    if (mouse.buttons & MOUSE_BUTTON_LEFT)   diag_printf_user(out, "LEFT ");
    if (mouse.buttons & MOUSE_BUTTON_RIGHT)  diag_printf_user(out, "RIGHT ");
    if (mouse.buttons & MOUSE_BUTTON_MIDDLE) diag_printf_user(out, "MIDDLE ");
    if (mouse.buttons & MOUSE_BUTTON_SIDE1)  diag_printf_user(out, "SIDE1 ");
    if (mouse.buttons & MOUSE_BUTTON_SIDE2)  diag_printf_user(out, "SIDE2 ");
    diag_printf_user(out, "\n");

    diag_printf_user(out, "  Mouse Speed: %.2f pixels/dot\n", mouse.speed);
    diag_printf_user(out, "  Acceleration: %.2f\n", mouse.acceleration);
    diag_printf_user(out, "  Sample Rate: %u Hz\n", mouse.sample_rate);
    diag_printf_user(out, "  Device Type: %s\n",
        mouse.type == MOUSE_PS2 ? "PS/2" :
        mouse.type == MOUSE_USB ? "USB" : "Unknown");
    diag_printf_user(out, "  Driver: %s\n", mouse.driver_name);
}

/* ================================================================
 * PCI DEVICES WITH IRQ ASSIGNMENTS (Item 54)
 * ================================================================ */
static void diag_io_pci_irq(diag_output_t *out) {
    diag_header(out, "PCI Devices with IRQ Assignments");

    pci_device_t devs[64];
    int ndevs = pci_get_devices(devs, 64);

    if (ndevs == 0) {
        diag_printf_user(out, "  No PCI devices found.\n");
        return;
    }

    diag_printf_user(out, "  Bus Dev Fun  Vendor  Device   Class      Subclass    IRQ   MSI\n");
    diag_printf_user(out, "  --- ---- ----  ------  -------  ---------  ----------  ----  ---\n");

    for (int i = 0; i < ndevs; i++) {
        /* Decode class code */
        uint8_t class = devs[i].class_code >> 24;
        uint8_t subclass = (devs[i].class_code >> 16) & 0xFF;

        const char *class_name = "Unknown";
        switch (class) {
            case 0x00: class_name = "Unclassified"; break;
            case 0x01: class_name = "Mass Storage"; break;
            case 0x02: class_name = "Network"; break;
            case 0x03: class_name = "Display"; break;
            case 0x04: class_name = "Multimedia"; break;
            case 0x05: class_name = "Memory"; break;
            case 0x06: class_name = "Bridge"; break;
            case 0x07: class_name = "Communications"; break;
            case 0x08: class_name = "System Periph"; break;
            case 0x09: class_name = "Input"; break;
            case 0x0A: class_name = "Docking"; break;
            case 0x0B: class_name = "Processor"; break;
            case 0x0C: class_name = "Serial Bus"; break;
            case 0x0D: class_name = "Wireless"; break;
            case 0x0E: class_name = "Intelligent I/O"; break;
            case 0x0F: class_name = "Satellite"; break;
            case 0x10: class_name = "Encryption"; break;
            case 0x11: class_name = "Signal Processing"; break;
        }

        diag_printf_user(out, "  %02X  %03X  %03X  0x%04X  0x%04X  %-11s  %-10s  %4d  %s\n",
            devs[i].bus,
            devs[i].device,
            devs[i].function,
            devs[i].vendor_id,
            devs[i].device_id,
            class_name,
            "N/A", /* subclass name - could be more detailed */
            devs[i].irq,
            devs[i].msi_enabled ? "Yes" : "No");
    }

    diag_printf_user(out, "\n  Total PCI devices: %d\n", ndevs);
    diag_printf_user(out, "  MSI-capable devices: %d\n", pci_count_msi_devices());
}

/* ================================================================
 * USB DEVICE TREE (Item 55)
 * ================================================================ */
static void diag_io_usb_tree(diag_output_t *out) {
    diag_header(out, "USB Device Tree");

#ifdef CONFIG_USB
    usb_device_t root_hub;
    int ret = usb_get_root_hub(&root_hub);

    if (ret < 0) {
        diag_printf_user(out, "  USB Controller: Not initialized\n");
        return;
    }

    diag_printf_user(out, "  USB Host Controller: %s\n",
        root_hub.controller_name ? root_hub.controller_name : "UHCI/OHCI/EHCI");
    diag_printf_user(out, "  Root Hub Ports: %d\n", root_hub.num_ports);
    diag_printf_user(out, "  USB Version: %u.%u\n",
        root_hub.major_version, root_hub.minor_version);
    diag_printf_user(out, "\n  Device Tree:\n");
    usb_print_tree(out, &root_hub, 0); /* Recursive print helper */
#else
    diag_printf_user(out, "  USB Subsystem: Not implemented\n");
#endif
}

/* ================================================================
 * AUDIO BUFFER STATUS (Item 56)
 * ================================================================ */
static void diag_io_audio(diag_output_t *out) {
    diag_header(out, "Audio Buffer Status");

#ifdef CONFIG_AUDIO
    audio_stats_t audio;
    audio_get_stats(&audio);

    diag_printf_user(out, "  Audio Device: %s\n", audio.device_name);
    diag_printf_user(out, "  Sample Rate: %u Hz\n", audio.sample_rate);
    diag_printf_user(out, "  Channels: %u\n", audio.channels);
    diag_printf_user(out, "  Bits per Sample: %u\n", audio.bits_per_sample);

    diag_printf_user(out, "\n  Playback:\n");
    diag_printf_user(out, "    Buffer Size:   %u bytes\n", audio.playback_buffer_size);
    diag_printf_user(out, "    Write Pointer: %u bytes\n", audio.playback_write_ptr);
    diag_printf_user(out, "    Free Space:    %u bytes (%.1f%%)\n",
        audio.playback_free,
        (double)audio.playback_free * 100.0 / audio.playback_buffer_size);
    diag_printf_user(out, "    Underruns:     %u\n", audio.playback_underruns);

    diag_printf_user(out, "\n  Capture (Recording):\n");
    diag_printf_user(out, "    Buffer Size:   %u bytes\n", audio.capture_buffer_size);
    diag_printf_user(out, "    Read Pointer:  %u bytes\n", audio.capture_read_ptr);
    diag_printf_user(out, "    Available:     %u bytes (%.1f%%)\n",
        audio.capture_available,
        (double)audio.capture_available * 100.0 / audio.capture_buffer_size);
    diag_printf_user(out, "    Overruns:      %u\n", audio.capture_overruns);
#else
    diag_printf_user(out, "  Audio Subsystem: Not implemented\n");
#endif
}

/* ================================================================
 * DMA CHANNELS (Item 57)
 * ================================================================ */
static void diag_io_dma(diag_output_t *out) {
    diag_header(out, "DMA Channels and Transfers");

#ifdef CONFIG_DMA
    dma_channel_t chans[8];
    int nchans = dma_get_channels(chans, 8);

    diag_printf_user(out, "  DMA Channels (8237/PC/ISA style):\n");
    diag_printf_user(out, "  Ch  Mode     Addr     Count    AutoInc   Masked   Active\n");
    diag_printf_user(out, "  --  -------  -------  -------  --------  -------  ------\n");

    for (int i = 0; i < nchans; i++) {
        diag_printf_user(out, "  %-2d  %-7s  0x%06X  %-7u  %-8s  %-7s  %s\n",
            chans[i].channel,
            chans[i].mode == DMA_MODE_READ ? "Read" :
            chans[i].mode == DMA_MODE_WRITE ? "Write" : "Invalid",
            chans[i].address,
            chans[i].count,
            chans[i].auto_increment ? "Yes" : "No",
            chans[i].masked ? "Yes" : "No",
            chans[i].active ? "Yes" : "No");
    }
    diag_printf_user(out, "\n  Total Channels: %d\n", nchans);
    diag_printf_user(out, "  Active Transfers: %d\n", dma_count_active());
#else
    diag_printf_user(out, "  DMA Controller: Not implemented\n");
    diag_printf_user(out, "  (Legacy ISA DMA not in use on modern systems)\n");
#endif
}

/* ================================================================
 * IRQ TO HANDLER MAPPING (Item 58)
 * ================================================================ */
static void diag_io_irq_map(diag_output_t *out) {
    diag_header(out, "IRQ to Handler Mapping");

    irq_handler_t handlers[32];
    int nhandlers = pic_get_handlers(handlers, 32);

    diag_printf_user(out, "  IRQ  Vector  Handler Addr          Handler Name     Count\n");
    diag_printf_user(out, "  ---  ------  -------------------   --------------   -----\n");

    for (int i = 0; i < nhandlers; i++) {
        diag_printf_user(out, "  %3d  0x%03X   0x%016llX   %-15s  %llu\n",
            handlers[i].irq,
            handlers[i].vector,
            handlers[i].handler_addr,
            handlers[i].handler_name,
            handlers[i].irq_count);
    }

    diag_printf_user(out, "\n  Total IRQs: %d\n", nhandlers);
    diag_printf_user(out, "  PIC (8259) mode: %s\n", pic_is_mode() ? "Cascade" : "Single");
    diag_printf_user(out, "  APIC mode: %s\n", apic_is_enabled() ? "Enabled" : "Disabled");
}

/* ================================================================
 * STORAGE QUEUE (Item 59)
 * ================================================================ */
static void diag_io_storage_queue(diag_output_t *out) {
    diag_header(out, "Storage Queue");

    ata_device_t ata_devs[4];
    int ndevs = ata_get_devices(ata_devs, 4);

    if (ndevs == 0) {
        diag_printf_user(out, "  No ATA devices detected.\n");
        return;
    }

    for (int i = 0; i < ndevs; i++) {
        diag_printf_user(out, "  ATA Device %d (%s):\n", i,
            ata_devs[i].is_cdrom ? "CD-ROM" : "Disk");
        diag_printf_user(out, "    Model: %s\n", ata_devs[i].model);
        diag_printf_user(out, "    Serial: %s\n", ata_devs[i].serial);
        diag_printf_user(out, "    Size: %llu MB (%llu sectors * 512B)\n",
            ata_devs[i].sectors / (1024 * 1024),
            ata_devs[i].sectors);
        diag_printf_user(out, "    PIO Mode: %u\n", ata_devs[i].pio_mode);
        diag_printf_user(out, "    DMA Mode: %u\n", ata_devs[i].dma_mode);
        diag_printf_user(out, "    Pending I/O: %u\n", ata_devs[i].pending_io);
        diag_printf_user(out, "    Queue Depth:  %u / %u\n",
            ata_devs[i].queue_depth,
            ata_devs[i].max_queue_depth);
        diag_printf_user(out, "    Errors:      %u\n", ata_devs[i].error_count);
        diag_printf_user(out, "    Last Error:  0x%02X\n", ata_devs[i].last_error);
    }
}

/* ================================================================
 * AVERAGE INTERRUPT LATENCY (Item 60)
 * ================================================================ */
static void diag_io_irq_latency(diag_output_t *out) {
    diag_header(out, "Interrupt Latency");

    irq_latency_stats_t ilat;
    pic_get_latency_stats(&ilat);

    diag_printf_user(out, "  Interrupt Latency Statistics:\n");
    diag_printf_user(out, "    Average Latency: %u cycles (%.2f us)\n",
        ilat.avg_cycles,
        ilat.avg_cycles / 2500.0);
    diag_printf_user(out, "    Min Latency: %u cycles\n", ilat.min_cycles);
    diag_printf_user(out, "    Max Latency: %u cycles (%.2f ms)\n",
        ilat.max_cycles,
        ilat.max_cycles / 2500000.0);

    diag_printf_user(out, "\n  Per-IRQ Latency (top 10 by count):\n");
    diag_printf_user(out, "    IRQ  Avg(us)  Max(ms)  Count\n");
    diag_printf_user(out, "    ---  --------  -------  -----\n");
    for (int i = 0; i < ilat.num_irqs; i++) {
        diag_printf_user(out, "    %3d  %8.2f  %7.2f  %6llu\n",
            ilat.irq_stats[i].irq,
            (double)ilat.irq_stats[i].avg_cycles / 2500.0,
            (double)ilat.irq_stats[i].max_cycles / 2500000.0,
            ilat.irq_stats[i].count);
    }

    diag_printf_user(out, "\n  Note: Latency measured from IRQ assertion to handler entry.\n");
}

/* ================================================================
 * I/O DISPATCHER
 * ================================================================ */
void diag_io_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_IO_DRIVERS:
            diag_io_drivers(out);
            break;
        case DIAG_IO_KEYBOARD:
            diag_io_keyboard(out);
            break;
        case DIAG_IO_MOUSE:
            diag_io_mouse(out);
            break;
        case DIAG_IO_PCI_IRQ:
            diag_io_pci_irq(out);
            break;
        case DIAG_IO_USB_TREE:
            diag_io_usb_tree(out);
            break;
        case DIAG_IO_AUDIO:
            diag_io_audio(out);
            break;
        case DIAG_IO_DMA:
            diag_io_dma(out);
            break;
        case DIAG_IO_IRQ_MAP:
            diag_io_irq_map(out);
            break;
        case DIAG_IO_STORAGE_QUEUE:
            diag_io_storage_queue(out);
            break;
        case DIAG_IO_IRQ_LATENCY:
            diag_io_irq_latency(out);
            break;
        case 0: /* All I/O diagnostics */
            diag_io_drivers(out);
            diag_io_keyboard(out);
            diag_io_mouse(out);
            diag_io_pci_irq(out);
            diag_io_usb_tree(out);
            diag_io_audio(out);
            diag_io_dma(out);
            diag_io_irq_map(out);
            diag_io_storage_queue(out);
            diag_io_irq_latency(out);
            break;
        default:
            diag_header(out, "I/O Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
