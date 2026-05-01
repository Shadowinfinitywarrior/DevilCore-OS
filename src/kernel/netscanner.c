#include "netscanner.h"
#include "wm.h"
#include "framebuffer.h"
#include "timer.h"
#include "memory.h"
#include "net.h"
#include <string.h>
#include <stdio.h>

#define MAX_PACKETS 100

struct net_packet_log {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t protocol;
    uint16_t length;
    uint8_t severity; // 0=safe, 1=warning, 2=danger
    uint64_t timestamp;
};

struct netscanner_data {
    struct net_packet_log packets[MAX_PACKETS];
    uint32_t packet_idx;
    uint32_t packet_count;
    uint8_t is_scanning;
};

// Global pointer for the callback to find the window
static struct netscanner_data *global_ns_data = NULL;

static struct wm_window *ns_window = NULL;

struct ipv4_header {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

void netscanner_log_packet(struct net_packet *packet) {
    if (!global_ns_data || !global_ns_data->is_scanning || !packet || packet->length < 14) return;
    
    uint8_t *pdata = (uint8_t *)packet->data;
    uint16_t eth_type = (pdata[12] << 8) | pdata[13];
    
    struct net_packet_log *pkt = &global_ns_data->packets[global_ns_data->packet_idx];
    pkt->timestamp = timer_ticks();
    pkt->length = packet->length;
    
    if (eth_type == 0x0806) { // ARP
        pkt->protocol = 0; // Special protocol for ARP
        pkt->src_ip = 0;
        pkt->dst_ip = 0;
        pkt->severity = 0;
    } else if (eth_type == 0x0800 && packet->length >= 34) { // IPv4
        struct ipv4_header *ipv4 = (struct ipv4_header *)(pdata + 14);
        pkt->src_ip = ipv4->src_ip;
        pkt->dst_ip = ipv4->dest_ip;
        pkt->protocol = ipv4->protocol;
        
        if (pkt->protocol == 1) pkt->severity = 1; // ICMP
        else pkt->severity = 0;
    } else {
        return; // Ignore other ethernet types
    }
    
    global_ns_data->packet_idx = (global_ns_data->packet_idx + 1) % MAX_PACKETS;
    if (global_ns_data->packet_count < MAX_PACKETS) global_ns_data->packet_count++;
    
    if (ns_window) {
        wm_widget_mark_dirty(&ns_window->widget);
    }
}

static void ns_draw_content(struct wm_window *win) {
    struct netscanner_data *data = (struct netscanner_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Toolbar
    fb_fill_rect(x, y, w, 40, 0x00111111);
    fb_draw_rect_outline(x, y, w, 40, 0x00333333);
    
    if (data->is_scanning) {
        fb_fill_rect_rounded(x + 10, y + 8, 100, 24, 4, 0x00aa0000);
        fb_draw_string(x + 25, y + 12, "Stop Scan", 0x00ffffff, 0);
    } else {
        fb_fill_rect_rounded(x + 10, y + 8, 100, 24, 4, 0x0000aa00);
        fb_draw_string(x + 20, y + 12, "Start Scan", 0x00ffffff, 0);
    }
    
    // Header
    uint32_t list_y = y + 40;
    fb_fill_rect(x, list_y, w, 20, 0x00222222);
    fb_draw_string(x + 10, list_y + 4, "TIME", 0x00aaaaaa, 0);
    fb_draw_string(x + 100, list_y + 4, "SRC IP", 0x00aaaaaa, 0);
    fb_draw_string(x + 240, list_y + 4, "DST IP", 0x00aaaaaa, 0);
    fb_draw_string(x + 380, list_y + 4, "PROTO", 0x00aaaaaa, 0);
    fb_draw_string(x + 460, list_y + 4, "LEN", 0x00aaaaaa, 0);
    
    list_y += 20;
    
    // List packets (from newest to oldest)
    uint32_t visible_lines = (h - 60) / 20;
    uint32_t draw_count = data->packet_count > visible_lines ? visible_lines : data->packet_count;
    
    for (uint32_t i = 0; i < draw_count; i++) {
        // Calculate index going backwards from packet_idx
        uint32_t idx = (data->packet_idx + MAX_PACKETS - 1 - i) % MAX_PACKETS;
        struct net_packet_log *pkt = &data->packets[idx];
        
        uint32_t bg_color = (i % 2 == 0) ? 0x00151515 : 0x00111111;
        fb_fill_rect(x, list_y, w, 20, bg_color);
        
        uint32_t text_color = 0x00aaaaaa;
        if (pkt->severity == 1) text_color = 0x00ffaa00; // Warning
        else if (pkt->severity == 2) text_color = 0x00ff4444; // Danger
        
        char buf[64];
        sprintf(buf, "%05d", (uint32_t)(pkt->timestamp % 100000));
        fb_draw_string(x + 10, list_y + 4, buf, text_color, 0);
        
        sprintf(buf, "%d.%d.%d.%d", (pkt->src_ip >> 24) & 0xFF, (pkt->src_ip >> 16) & 0xFF, (pkt->src_ip >> 8) & 0xFF, pkt->src_ip & 0xFF);
        fb_draw_string(x + 100, list_y + 4, buf, text_color, 0);
        
        sprintf(buf, "%d.%d.%d.%d", (pkt->dst_ip >> 24) & 0xFF, (pkt->dst_ip >> 16) & 0xFF, (pkt->dst_ip >> 8) & 0xFF, pkt->dst_ip & 0xFF);
        fb_draw_string(x + 240, list_y + 4, buf, text_color, 0);
        
        const char *proto = "UNK";
        if (pkt->protocol == 17) proto = "UDP";
        else if (pkt->protocol == 1) proto = "ICMP";
        else if (pkt->protocol == 6) proto = "TCP";
        else if (pkt->protocol == 0) proto = "ARP";
        
        fb_draw_string(x + 380, list_y + 4, proto, text_color, 0);
        
        sprintf(buf, "%d", pkt->length);
        fb_draw_string(x + 460, list_y + 4, buf, text_color, 0);
        
        list_y += 20;
    }
    
    // No mock data generated here anymore!
}

static void ns_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct netscanner_data *data = (struct netscanner_data *)win->widget.data;
    if (!data) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_KEY) {
        if (event->key == 'q' || event->key == 27) {
            wm_destroy_window(win);
            ns_window = NULL;
            global_ns_data = NULL;
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS && (event->buttons & 1)) {
        if (event->y >= 8 && event->y <= 32 && event->x >= 10 && event->x <= 110) {
            data->is_scanning = !data->is_scanning;
            wm_widget_mark_dirty(widget);
        }
    }
}

void netscanner_open(void) {
    if (ns_window != NULL) {
        wm_focus_window(ns_window);
        return;
    }
    
    struct netscanner_data *data = kmalloc(sizeof(struct netscanner_data));
    memset(data, 0, sizeof(struct netscanner_data));
    data->is_scanning = 0;
    
    global_ns_data = data;
    
    ns_window = wm_create_window("Network Scanner (Sniffer)", 250, 180, 550, 400);
    if (ns_window != NULL) {
        ns_window->widget.data = data;
        ns_window->draw_content = ns_draw_content;
        ns_window->widget.on_event = ns_event;
        ns_window->bg_color = 0x00151515;
    } else {
        kfree(data);
    }
}
