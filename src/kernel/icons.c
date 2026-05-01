#include "icons.h"
#include "framebuffer.h"
#include <string.h>

#include "add_icon.h"
#include "airplane_mode_icon.h"
#include "appearance_icon.h"
#include "back_icon.h"
#include "backup_icon.h"
#include "battery_charging_icon.h"
#include "battery_critical_icon.h"
#include "battery_full_icon.h"
#include "battery_low_icon.h"
#include "battery_med_icon.h"
#include "bell_icon.h"
#include "bios_icon.h"
#include "bluetooth_icon.h"
#include "bootloader_icon.h"
#include "browser_icon.h"
#include "calculator_icon.h"
#include "calendar_icon.h"
#include "camera_icon.h"
#include "close_icon.h"
#include "copy_icon.h"
#include "cpu_icon.h"
#include "crash_icon.h"
#include "cut_icon.h"
#include "debug_icon.h"
#include "delete_icon.h"
#include "dependency_icon.h"
#include "desktop_icon.h"
#include "disk_icon.h"
#include "dock_icon.h"
#include "document_icon.h"
#include "download_icon.h"
#include "downloads_icon.h"
#include "edit_icon.h"
#include "email_icon.h"
#include "encryption_icon.h"
#include "error_icon.h"
#include "ethernet_off_icon.h"
#include "ethernet_on_icon.h"
#include "ext_drive_icon.h"
#include "filemanager_icon.h"
#include "firewall_icon.h"
#include "folder_icon.h"
#include "folder_hidden_icon.h"
#include "folder_network_icon.h"
#include "folder_open_icon.h"
#include "folder_shared_icon.h"
#include "fonts_icon.h"
#include "forward_icon.h"
#include "fullscreen_icon.h"
#include "gpu_icon.h"
#include "home_icon.h"
#include "image_viewer_icon.h"
#include "info_icon.h"
#include "kernel_icon.h"
#include "key_icon.h"
#include "lock_icon.h"
#include "logs_icon.h"
#include "maximize_icon.h"
#include "media_player_icon.h"
#include "menu_icon.h"
#include "message_icon.h"
#include "mic_icon.h"
#include "minimize_icon.h"
#include "more_icon.h"
#include "music_icon.h"
#include "network_icon.h"
#include "notes_icon.h"
#include "package_manager_icon.h"
#include "paste_icon.h"
#include "pictures_icon.h"
#include "pkg_install_icon.h"
#include "pkg_remove_icon.h"
#include "pkg_update_icon.h"
#include "power_icon.h"
#include "print_icon.h"
#include "printer_icon.h"
#include "recovery_icon.h"
#include "redo_icon.h"
#include "refresh_icon.h"
#include "resize_icon.h"
#include "restart_icon.h"
#include "restore_icon.h"
#include "root_fs_icon.h"
#include "safemode_icon.h"
#include "save_icon.h"
#include "save_as_icon.h"
#include "search_icon.h"
#include "service_running_icon.h"
#include "service_stopped_icon.h"
#include "settings_icon.h"
#include "share_icon.h"
#include "shield_icon.h"
#include "sleep_icon.h"
#include "sync_icon.h"
#include "system_icon.h"
#include "system_monitor_icon.h"
#include "terminal_icon.h"
#include "texteditor_icon.h"
#include "themes_icon.h"
#include "trash_empty_icon.h"
#include "trash_full_icon.h"
#include "undo_icon.h"
#include "unlock_icon.h"
#include "up_icon.h"
#include "update_icon.h"
#include "upload_icon.h"
#include "usb_icon.h"
#include "user_icon.h"
#include "videos_icon.h"
#include "vol_high_icon.h"
#include "vol_low_icon.h"
#include "vol_med_icon.h"
#include "vol_muted_icon.h"
#include "wallpaper_icon.h"
#include "warning_icon.h"
#include "wifi_high_icon.h"
#include "wifi_low_icon.h"
#include "wifi_med_icon.h"
#include "wifi_none_icon.h"

static const struct icon_data icons_list[] = {
    {"add", 32, 32, add_pixels},
    {"airplane_mode", 32, 32, airplane_mode_pixels},
    {"appearance", 32, 32, appearance_pixels},
    {"back", 32, 32, back_pixels},
    {"backup", 32, 32, backup_pixels},
    {"battery_charging", 32, 32, battery_charging_pixels},
    {"battery_critical", 32, 32, battery_critical_pixels},
    {"battery_full", 32, 32, battery_full_pixels},
    {"battery_low", 32, 32, battery_low_pixels},
    {"battery_med", 32, 32, battery_med_pixels},
    {"bell", 32, 32, bell_pixels},
    {"bios", 32, 32, bios_pixels},
    {"bluetooth", 32, 32, bluetooth_pixels},
    {"bootloader", 32, 32, bootloader_pixels},
    {"browser", 32, 32, browser_pixels},
    {"calculator", 32, 32, calculator_pixels},
    {"calendar", 32, 32, calendar_pixels},
    {"camera", 32, 32, camera_pixels},
    {"close", 32, 32, close_pixels},
    {"copy", 32, 32, copy_pixels},
    {"cpu", 32, 32, cpu_pixels},
    {"crash", 32, 32, crash_pixels},
    {"cut", 32, 32, cut_pixels},
    {"debug", 32, 32, debug_pixels},
    {"delete", 32, 32, delete_pixels},
    {"dependency", 32, 32, dependency_pixels},
    {"desktop", 32, 32, desktop_pixels},
    {"disk", 32, 32, disk_pixels},
    {"dock", 32, 32, dock_pixels},
    {"document", 32, 32, document_pixels},
    {"download", 32, 32, download_pixels},
    {"downloads", 32, 32, downloads_pixels},
    {"edit", 32, 32, edit_pixels},
    {"email", 32, 32, email_pixels},
    {"encryption", 32, 32, encryption_pixels},
    {"error", 32, 32, error_pixels},
    {"ethernet_off", 32, 32, ethernet_off_pixels},
    {"ethernet_on", 32, 32, ethernet_on_pixels},
    {"ext_drive", 32, 32, ext_drive_pixels},
    {"filemanager", 32, 32, filemanager_pixels},
    {"firewall", 32, 32, firewall_pixels},
    {"folder", 32, 32, folder_pixels},
    {"folder_hidden", 32, 32, folder_hidden_pixels},
    {"folder_network", 32, 32, folder_network_pixels},
    {"folder_open", 32, 32, folder_open_pixels},
    {"folder_shared", 32, 32, folder_shared_pixels},
    {"fonts", 32, 32, fonts_pixels},
    {"forward", 32, 32, forward_pixels},
    {"fullscreen", 32, 32, fullscreen_pixels},
    {"gpu", 32, 32, gpu_pixels},
    {"home", 32, 32, home_pixels},
    {"image_viewer", 32, 32, image_viewer_pixels},
    {"info", 32, 32, info_pixels},
    {"kernel", 32, 32, kernel_pixels},
    {"key", 32, 32, key_pixels},
    {"lock", 32, 32, lock_pixels},
    {"logs", 32, 32, logs_pixels},
    {"maximize", 32, 32, maximize_pixels},
    {"media_player", 32, 32, media_player_pixels},
    {"menu", 32, 32, menu_pixels},
    {"message", 32, 32, message_pixels},
    {"mic", 32, 32, mic_pixels},
    {"minimize", 32, 32, minimize_pixels},
    {"more", 32, 32, more_pixels},
    {"music", 32, 32, music_pixels},
    {"network", 32, 32, network_pixels},
    {"notes", 32, 32, notes_pixels},
    {"package_manager", 32, 32, package_manager_pixels},
    {"paste", 32, 32, paste_pixels},
    {"pictures", 32, 32, pictures_pixels},
    {"pkg_install", 32, 32, pkg_install_pixels},
    {"pkg_remove", 32, 32, pkg_remove_pixels},
    {"pkg_update", 32, 32, pkg_update_pixels},
    {"power", 32, 32, power_pixels},
    {"print", 32, 32, print_pixels},
    {"printer", 32, 32, printer_pixels},
    {"recovery", 32, 32, recovery_pixels},
    {"redo", 32, 32, redo_pixels},
    {"refresh", 32, 32, refresh_pixels},
    {"resize", 32, 32, resize_pixels},
    {"restart", 32, 32, restart_pixels},
    {"restore", 32, 32, restore_pixels},
    {"root_fs", 32, 32, root_fs_pixels},
    {"safemode", 32, 32, safemode_pixels},
    {"save", 32, 32, save_pixels},
    {"save_as", 32, 32, save_as_pixels},
    {"search", 32, 32, search_pixels},
    {"service_running", 32, 32, service_running_pixels},
    {"service_stopped", 32, 32, service_stopped_pixels},
    {"settings", 32, 32, settings_pixels},
    {"share", 32, 32, share_pixels},
    {"shield", 32, 32, shield_pixels},
    {"sleep", 32, 32, sleep_pixels},
    {"sync", 32, 32, sync_pixels},
    {"system", 32, 32, system_pixels},
    {"system_monitor", 32, 32, system_monitor_pixels},
    {"terminal", 32, 32, terminal_pixels},
    {"texteditor", 32, 32, texteditor_pixels},
    {"themes", 32, 32, themes_pixels},
    {"trash_empty", 32, 32, trash_empty_pixels},
    {"trash_full", 32, 32, trash_full_pixels},
    {"undo", 32, 32, undo_pixels},
    {"unlock", 32, 32, unlock_pixels},
    {"up", 32, 32, up_pixels},
    {"update", 32, 32, update_pixels},
    {"upload", 32, 32, upload_pixels},
    {"usb", 32, 32, usb_pixels},
    {"user", 32, 32, user_pixels},
    {"videos", 32, 32, videos_pixels},
    {"vol_high", 32, 32, vol_high_pixels},
    {"vol_low", 32, 32, vol_low_pixels},
    {"vol_med", 32, 32, vol_med_pixels},
    {"vol_muted", 32, 32, vol_muted_pixels},
    {"wallpaper", 32, 32, wallpaper_pixels},
    {"warning", 32, 32, warning_pixels},
    {"wifi_high", 32, 32, wifi_high_pixels},
    {"wifi_low", 32, 32, wifi_low_pixels},
    {"wifi_med", 32, 32, wifi_med_pixels},
    {"wifi_none", 32, 32, wifi_none_pixels},
};

const struct icon_data *icon_get(const char *name) {
    for (uint32_t i = 0; i < sizeof(icons_list) / sizeof(icons_list[0]); ++i) {
        if (strcmp(name, icons_list[i].name) == 0) {
            return &icons_list[i];
        }
    }
    return NULL;
}

void draw_icon(uint32_t x, uint32_t y, const struct icon_data *icon) {
    if (icon == NULL || framebuffer == NULL) return;
    fb_draw_bitmap_alpha(x, y, icon->width, icon->height, icon->pixels);
}
