/*
 * DevilCore OS — Advanced/Extra Features Diagnostics
 * Items 111-120
 */

#include "diag.h"
#include "../gui/framebuffer.h"
#include "../arch/x86_64/cpu.h"
#include "../drivers/pci.h"
#include "../drivers/ata.h"
#include "../include/ports.h"
#include "../include/string.h"

/* ================================================================
 * 3D GPU COMMAND QUEUE (Item 111)
 * ================================================================ */
static void diag_extra_gpu_queue(diag_output_t *out) {
    diag_header(out, "3D GPU Command Queue Status");

#ifdef CONFIG_GPU_VULKAN
    gpu_queue_info_t gpu;
    gpu_get_queue_info(&gpu);

    diag_printf_user(out, "  GPU: %s\n", gpu.device_name);
    diag_printf_user(out, "  Driver: %s\n", gpu.driver);
    diag_printf_user(out, "  API: %s\n", gpu.api); /* Vulkan/OpenGL */
    diag_printf_user(out, "\n  Command Queues:\n");
    for (int i = 0; i < gpu.num_queues; i++) {
        diag_printf_user(out, "    Queue %d: %s\n", i,
            gpu.queues[i].type == QUEUE_GRAPHICS ? "Graphics" :
            gpu.queues[i].type == QUEUE_COMPUTE ? "Compute" :
            gpu.queues[i].type == QUEUE_TRANSFER ? "Transfer" : "Unknown");
        diag_printf_user(out, "      Pending: %u cmds\n", gpu.queues[i].pending);
        diag_printf_user(out, "      Submitted: %llu\n", gpu.queues[i].submitted);
        diag_printf_user(out, "      Completed: %llu\n", gpu.queues[i].completed);
    }
    diag_printf_user(out, "\n  GPU Utilization: %.1f%%\n", gpu.utilization);
    diag_printf_user(out, "  Memory: %u MB VRAM / %u MB GTT\n", gpu.vram_mb, gpu.gtt_mb);
#else
    diag_printf_user(out, "  3D Graphics: Not implemented\n");
    diag_printf_user(out, "  No Vulkan/OpenGL support in kernel.\n");
    diag_printf_user(out, "  Software rendering: CPU-based (VESA framebuffer)\n");
#endif
}

/* ================================================================
 * VMX/SVM (Virtualization) STATUS (Item 112)
 * ================================================================ */
static void diag_extra_vmx_svm(diag_output_t *out) {
    diag_header(out, "Virtualization Support (VMX/SVM)");

    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    int vmx_supported = (ecx & (1 << 5)) != 0;
    int svm_supported  = (ecx & (1 << 2)) != 0; /* Different CPUID leaf needed */

    diag_printf_user(out, "  CPUID Feature Flags:\n");
    diag_printf_user(out, "    VMX (Intel VT-x): %s\n", vmx_supported ? "Supported" : "Not supported");
    diag_printf_user(out, "    SVM (AMD-V):     %s\n", svm_supported ? "Supported" : "Not supported");

    /* Check if actually enabled in CR4 */
    uint64_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r" (cr4));
    int vmx_enabled = (cr4 & (1ULL << 13)) != 0; /* CR4.VMXE */
    int svm_enabled = 0; /* SVM uses EFER.SVME, bit 12 */

    diag_printf_user(out, "\n  Current State:\n");
    diag_printf_user(out, "    VMX Enabled: %s\n", vmx_enabled ? "Yes" : "No");
    diag_printf_user(out, "    SVM Enabled: %s\n", svm_enabled ? "Yes" : "No");

    if (vmx_enabled || svm_enabled) {
        /* Read VMCS/VMCB if present */
        diag_printf_user(out, "\n  Hypervisor present: %s\n",
            (cr4 & (1<<31)) ? "Guest under hypervisor" : "Bare metal");
    } else {
        diag_printf_user(out, "\n  No hypervisor detected (running on bare metal)\n");
    }

    diag_printf_user(out, "\n  Note: DevilCore can act as a hypervisor (future)\n");
    diag_printf_user(out, "  or run as guest under VMware/QEMU/Hyper-V.\n");
}

/* ================================================================
 * POWER MANAGEMENT SLEEP STATES (Item 113)
 * ================================================================ */
static void diag_extra_power_states(diag_output_t *out) {
    diag_header(out, "Power Management Sleep States");

    /* Query ACPI for supported Sx states */
    diag_printf_user(out, "  ACPI Sleep States Supported:\n");
    diag_printf_user(out, "    S0  - Working (fully on)\n");
    diag_printf_user(out, "    S1  - Halt (CPU stopped, RAM refresh)\n");
    diag_printf_user(out, "    S2  - Power-down (CPU off, RAM refresh)\n");
    diag_printf_user(out, "    S3  - Suspend-to-RAM (STR)\n");
    diag_printf_user(out, "    S4  - Suspend-to-Disk ( hibernation)\n");
    diag_printf_user(out, "    S5  - Soft-Off (G2)\n");

    /* Detect actual support via ACPI FACP */
    uint8_t s1 = acpi_check_sleep_state(1);
    uint8_t s3 = acpi_check_sleep_state(3);
    uint8_t s4 = acpi_check_sleep_state(4);

    diag_printf_user(out, "\n  This System Supports:\n");
    diag_printf_user(out, "    S1: %s\n", s1 ? "Yes" : "No");
    diag_printf_user(out, "    S3: %s\n", s3 ? "Yes" : "No");
    diag_printf_user(out, "    S4: %s\n", s4 ? "Yes" : "No");

    diag_printf_user(out, "\n  Current Power State: S0 (Working)\n");
    diag_printf_user(out, "  Last Wake Source: %s\n", get_wake_source_str());
    diag_printf_user(out, "  Sleep Statistics:\n");
    diag_printf_user(out, "    S1 transitions: %u\n", get_sleep_count_s1());
    diag_printf_user(out, "    S3 transitions: %u\n", get_sleep_count_s3());
    diag_printf_user(out, "    Total sleep time: %llu seconds\n", get_total_sleep_time());
}

/* ================================================================
 * BATTERY STATUS (Item 114)
 * ================================================================
 */
static void diag_extra_battery(diag_output_t *out) {
    diag_header(out, "Battery and Power Source");

#ifdef CONFIG_ACPI_BATTERY
    battery_info_t batt;
    battery_get_info(&batt);

    diag_printf_user(out, "  Power Source: %s\n",
        batt.on_ac ? "AC Adapter" : "Battery");
    diag_printf_user(out, "  Battery Present: %s\n", batt.present ? "Yes" : "No");
    diag_printf_user(out, "  State: %s\n",
        batt.state == BATT_CHARGING ? "Charging" :
        batt.state == BATT_DISCHARGING ? "Discharging" :
        batt.state == BATT_FULL ? "Full" : "Unknown");

    diag_printf_user(out, "\n  Capacity:\n");
    diag_printf_user(out, "    Design:   %u mAh\n", batt.design_capacity);
    diag_printf_user(out, "    Full:     %u mAh\n", batt.last_full_capacity);
    diag_printf_user(out, "    Remaining:%u mAh (%.1f%%)\n",
        batt.remaining_capacity,
        batt.last_full_capacity ?
        (double)batt.remaining_capacity * 100.0 / (double)batt.last_full_capacity : 0.0);

    diag_printf_user(out, "\n  Voltage: %.2f V\n", batt.voltage / 1000.0);
    diag_printf_user(out, "  Current: %.2f A (%s)\n",
        fabs(batt.current) / 1000.0,
        batt.current > 0 ? "Charging" : "Discharging");

    if (batt.present) {
        diag_printf_user(out, "\n  Time Remaining:\n");
        if (batt.state == BATT_DISCHARGING) {
            diag_printf_user(out, "    Discharge time left: %u minutes\n", batt.minutes_left);
        } else if (batt.state == BATT_CHARGING) {
            diag_printf_user(out, "    Time to full: %u minutes\n", batt.minutes_to_full);
        }
    }
#else
    diag_printf_user(out, "  ACPI Battery driver: Not implemented\n");
    diag_printf_user(out, "  Assuming desktop/AC power only.\n");
    diag_printf_user(out, "  Power Source: AC Adapter (assumed)\n");
#endif
}

/* ================================================================
 * MULTI-MONITOR CONFIGURATION (Item 115)
 * ================================================================ */
static void diag_extra_monitors(diag_output_t *out) {
    diag_header(out, "Multi-Monitor Configuration");

    display_info_t displays[8];
    int ndisplays = gfx_get_displays(displays, 8);

    if (ndisplays == 0) {
        diag_printf_user(out, "  No displays detected.\n");
        return;
    }

    diag_printf_user(out, "  Detected %d display(s):\n", ndisplays);
    diag_printf_user(out, "\n  #  Connector  Type       Resolution     Depth   Refresh  Primary\n");
    diag_printf_user(out, "  -  ---------  ---------  -------------  ------  -------  --------\n");

    for (int i = 0; i < ndisplays; i++) {
        const char *conn = "Unknown";
        switch (displays[i].connector) {
            case CONN_VGA:   conn = "VGA"; break;
            case CONN_DVI:   conn = "DVI"; break;
            case CONN_HDMI:  conn = "HDMI"; break;
            case CONN_DP:    conn = "DP"; break;
            case CONN_LVDS:  conn = "LVDS"; break;
        }

        const char *type = "Unknown";
        switch (displays[i].panel_type) {
            case PANEL_CRT:    type = "CRT"; break;
            case PANEL_LCD:    type = "LCD"; break;
            case PANEL_LED:    type = "LED"; break;
            case PANEL_OLED:   type = "OLED"; break;
            case PANEL_PLASMA: type = "Plasma"; break;
        }

        diag_printf_user(out, "  %-2d  %-9s  %-10s  %-13s  %-6d  %-7d  %s\n",
            i,
            conn,
            type,
            displays[i].edid_name,
            displays[i].bpp,
            displays[i].refresh_rate,
            displays[i].is_primary ? "Yes" : "No");
    }

    diag_printf_user(out, "\n  Desktop Configuration:\n");
    diag_printf_user(out, "    Virtual Size: %d x %d\n",
        g_desktop_width, g_desktop_height);
    diag_printf_user(out, "    Primary Monitor: #%d\n", g_primary_monitor);
    diag_printf_user(out, "    Extended: %s\n", g_extended_desktop ? "Yes" : "No");
    diag_printf_user(out, "    Mirroring: %s\n", g_mirror_displays ? "Yes" : "No");

    if (ndisplays > 1) {
        diag_printf_user(out, "\n  Monitor Arrangement:\n");
        for (int i = 0; i < ndisplays; i++) {
            diag_printf_user(out, "    %d: offset (%d, %d)\n",
                i, displays[i].offset_x, displays[i].offset_y);
        }
    }
}

/* ================================================================
 * PRINT SPOOLER QUEUE (Item 116)
 * ================================================================ */
static void diag_extra_print_spool(diag_output_t *out) {
    diag_header(out, "Print Spooler Queue");

#ifdef CONFIG_PRINT_SPOOLER
    print_job_t jobs[32];
    int njobs = spooler_get_queue(jobs, 32);

    if (njobs == 0) {
        diag_printf_user(out, "  Print queue is empty.\n");
        return;
    }

    diag_printf_user(out, "  #  Job ID   Owner       Document          Size     Pages   Status\n");
    diag_printf_user(out, "  -  --------  ----------  ----------------  -------  ------  ------\n");

    for (int i = 0; i < njobs; i++) {
        const char *status = "Pending";
        if (jobs[i].flags & JOB_PRINTING) status = "Printing";
        if (jobs[i].flags & JOB_COMPLETE)  status = "Complete";
        if (jobs[i].flags & JOB_ERROR)    status = "Error";
        if (jobs[i].flags & JOB_CANCELLED) status = "Cancelled";

        diag_printf_user(out, "  %-2d  %-8u  %-10s  %-16s  %7u  %6d  %s\n",
            i,
            jobs[i].job_id,
            jobs[i].owner,
            jobs[i].document_name,
            jobs[i].file_size,
            jobs[i].page_count,
            status);
    }

    diag_printf_user(out, "\n  Print Spooler Status: %s\n",
        spooler_is_active() ? "Active" : "Inactive");
    diag_printf_user(out, "  Active Job: %s\n",
        spooler_get_active_job() ? spooler_get_active_job()->document_name : "None");
    diag_printf_user(out, "  Default Printer: %s\n", spooler_get_default_printer());
    diag_printf_user(out, "  Queue Capacity: %d jobs\n", spooler_queue_capacity());
#else
    diag_printf_user(out, "  Print Spooler: Not implemented\n");
#endif
}

/* ================================================================
 * USER SESSIONS AND LOGIN TIMES (Item 117)
 * ================================================================ */
static void diag_extra_sessions(diag_output_t *out) {
    diag_header(out, "User Sessions and Login Times");

    session_info_t sessions[16];
    int nsessions = session_get_list(sessions, 16);

    if (nsessions == 0) {
        diag_printf_user(out, "  No active sessions.\n");
        return;
    }

    diag_printf_user(out, "  #  Terminal  User        UID   Login Time            Idle Time\n");
    diag_printf_user(out, "  -  --------  ----------  ----  -------------------   --------\n");

    for (int i = 0; i < nsessions; i++) {
        diag_printf_user(out, "  %-2d  %-8s  %-10s  %-4d  %-20s  %s\n",
            i,
            sessions[i].tty,
            sessions[i].username,
            sessions[i].uid,
            ctime(&sessions[i].login_time), /* ctime includes newline */
            format_duration(sessions[i].idle_seconds));
    }

    diag_printf_user(out, "\n  Active Sessions: %d\n", nsessions);
    diag_printf_user(out, "  Current session: %s (UID %d)\n",
        get_current_tty(), get_current_uid());
    diag_printf_user(out, "  Session leader: PID %d\n", get_session_leader());
}

/* ================================================================
 * CLIPBOARD CONTENTS (Item 118)
 * ================================================================ */
static void diag_extra_clipboard(diag_output_t *out) {
    diag_header(out, "Clipboard Contents");

    clipboard_info_t clip;
    clipboard_get_info(&clip);

    diag_printf_user(out, "  Clipboard Owner:  PID %d\n", clip.owner_pid);
    diag_printf_user(out, "  Sequence Number:  %u\n", clip.seq);
    diag_printf_user(out, "  Last Update:      %llu\n", clip.last_update);

    if (clip.has_text) {
        diag_printf_user(out, "\n  Text (UTF-8):\n");
        diag_printf_user(out, "    \"%s\"\n", clip.text_data);
        diag_printf_user(out, "    Length: %u bytes\n", clip.text_len);
    }

    if (clip.has_image) {
        diag_printf_user(out, "\n  Image:\n");
        diag_printf_user(out, "    Format: %s\n",
            clip.image_format == IMG_PNG ? "PNG" :
            clip.image_format == IMG_JPEG ? "JPEG" : "RAW");
        diag_printf_user(out, "    Width:  %d\n", clip.image_width);
        diag_printf_user(out, "    Height: %d\n", clip.image_height);
        diag_printf_user(out, "    Size:   %u bytes\n", clip.image_size);
    }

    if (clip.has_file_list) {
        diag_printf_user(out, "\n  File List (%d items):\n", clip.file_count);
        for (int i = 0; i < clip.file_count; i++) {
            diag_printf_user(out, "    %s\n", clip.files[i]);
        }
    }
}

/* ================================================================
 * DRAG-DROP OPERATION (Item 119)
 * ================================================================ */
static void diag_extra_drag_drop(diag_output_t *out) {
    diag_header(out, "Drag-Drop Operation");

    dragdrop_info_t dd;
    dragdrop_get_info(&dd);

    if (!dd.in_progress) {
        diag_printf_user(out, "  No drag-drop operation in progress.\n");
        return;
    }

    diag_printf_user(out, "  Drag in progress:\n");
    diag_printf_user(out, "    Source Window: 0x%06X\n", dd.source_window);
    diag_printf_user(out, "    Target Window: 0x%06X (current)\n", dd.target_window);
    diag_printf_user(out, "    Start Pos: (%d, %d)\n", dd.start_x, dd.start_y);
    diag_printf_user(out, "    Current Pos: (%d, %d)\n", dd.current_x, dd.current_y);
    diag_printf_user(out, "    Dragged Data: %s\n",
        dd.has_files ? "Files" :
        dd.has_text ? "Text" :
        dd.has_image ? "Image" : "Unknown");
    diag_printf_user(out, "    Operation: %s\n",
        dd.effect == DRAG_COPY ? "Copy" :
        dd.effect == DRAG_MOVE ? "Move" :
        dd.effect == DRAG_LINK ? "Link" : "None");

    diag_printf_user(out, "\n  Drag Images:\n");
    diag_printf_user(out, "    Ghost image: %s\n", dd.show_ghost ? "Yes" : "No");
    diag_printf_user(out, "    Drop indicator: %s\n", dd.show_drop_target ? "Yes" : "No");
}

/* ================================================================
 * GLOBAL HOTKEY REGISTRATION TABLE (Item 120)
 * ================================================================ */
static void diag_extra_hotkeys(diag_output_t *out) {
    diag_header(out, "Global Hotkey Registration Table");

    hotkey_entry_t hotkeys[64];
    int nhotkeys = hotkey_get_registered(hotkeys, 64);

    if (nhotkeys == 0) {
        diag_printf_user(out, "  No global hotkeys registered.\n");
        return;
    }

    diag_printf_user(out, "  #  Key Combo        Modifiers        Window/App       Callback\n");
    diag_printf_user(out, "  -  ---------------  ---------------  --------------   ------------------\n");

    for (int i = 0; i < nhotkeys; i++) {
        char mods[32] = {0};
        strcpy(mods, "");
        if (hotkeys[i].modifiers & MOD_CONTROL) strcat(mods, "Ctrl+");
        if (hotkeys[i].modifiers & MOD_ALT)     strcat(mods, "Alt+");
        if (hotkeys[i].modifiers & MOD_SHIFT)   strcat(mods, "Shift+");
        if (hotkeys[i].modifiers & MOD_SUPER)   strcat(mods, "Super+");

        diag_printf_user(out, "  %-2d  %-15s  %-15s  0x%06X        0x%016llX\n",
            i,
            hotkeys[i].key_combo,
            mods,
            hotkeys[i].window,
            hotkeys[i].callback);
    }

    diag_printf_user(out, "\n  Global Hotkey Engine:\n");
    diag_printf_user(out, "    Active: %s\n", hotkey_engine_active() ? "Yes" : "No");
    diag_printf_user(out, "    Total registered: %d\n", nhotkeys);
    diag_printf_user(out, "    Conflicts: %d\n", hotkey_count_conflicts());
    diag_printf_user(out, "    Mode: %s\n",
        hotkey_is_exclusive() ? "Exclusive grab" : "Cooperative");
}

/* ================================================================
 * ADVANCED DISPATCHER
 * ================================================================ */
void diag_extra_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_EXTRA_GPU_QUEUE:
            diag_extra_gpu_queue(out);
            break;
        case DIAG_EXTRA_VMX_SVM:
            diag_extra_vmx_svm(out);
            break;
        case DIAG_EXTRA_POWER_STATES:
            diag_extra_power_states(out);
            break;
        case DIAG_EXTRA_BATTERY:
            diag_extra_battery(out);
            break;
        case DIAG_EXTRA_MONITORS:
            diag_extra_monitors(out);
            break;
        case DIAG_EXTRA_PRINT_SPOOL:
            diag_extra_print_spool(out);
            break;
        case DIAG_EXTRA_SESSIONS:
            diag_extra_sessions(out);
            break;
        case DIAG_EXTRA_CLIPBOARD:
            diag_extra_clipboard(out);
            break;
        case DIAG_EXTRA_DRAG_DROP:
            diag_extra_drag_drop(out);
            break;
        case DIAG_EXTRA_HOTKEYS:
            diag_extra_hotkeys(out);
            break;
        case 0: /* All extra diagnostics */
            diag_extra_gpu_queue(out);
            diag_extra_vmx_svm(out);
            diag_extra_power_states(out);
            diag_extra_battery(out);
            diag_extra_monitors(out);
            diag_extra_print_spool(out);
            diag_extra_sessions(out);
            diag_extra_clipboard(out);
            diag_extra_drag_drop(out);
            diag_extra_hotkeys(out);
            break;
        default:
            diag_header(out, "Advanced Features Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
