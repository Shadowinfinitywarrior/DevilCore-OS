#include "devilui.h"
#include "memory.h"
#include "compositor.h"
#include "timer.h"
#include "scheduler.h"
#include <string.h>

extern void kprintf(const char *fmt, ...);
extern widget_t *focused_widget;

// Event queue
#define EVENT_QUEUE_SIZE 256

typedef struct event_queue {
    event_t events[EVENT_QUEUE_SIZE];
    uint32_t head, tail, count;
    uint8_t lock;
} event_queue_t;

static event_queue_t event_queue;
static uint8_t event_loop_running = 0;
static uint32_t next_timer_id = 1;

// Timer structure
struct timer_entry {
    uint32_t id;
    uint64_t fire_time;
    uint32_t interval_ms;
    bool repeat;
    void (*callback)(uint32_t timer_id, void *data);
    void *data;
    struct timer_entry *next;
};

static struct timer_entry *timer_list = NULL;

void event_loop_init(void) {
    memset(&event_queue, 0, sizeof(event_queue_t));
    event_queue.head = 0;
    event_queue.tail = 0;
    event_queue.count = 0;
    event_queue.lock = 0;
    event_loop_running = 0;
}

static void spinlock_acquire(uint8_t *lock) {
    while (__sync_lock_test_and_set(lock, 1)) {
        // Spin
    }
}

static void spinlock_release(uint8_t *lock) {
    __sync_lock_release(lock);
}

void event_push(event_t *event) {
    if (!event) return;
    
    spinlock_acquire(&event_queue.lock);
    
    if (event_queue.count < EVENT_QUEUE_SIZE) {
        event->timestamp = timer_ticks() * 10;  // Approximate ms
        event_queue.events[event_queue.tail] = *event;
        event_queue.tail = (event_queue.tail + 1) % EVENT_QUEUE_SIZE;
        event_queue.count++;
    }
    
    spinlock_release(&event_queue.lock);
}

bool event_poll(event_t *event) {
    spinlock_acquire(&event_queue.lock);
    
    if (event_queue.count == 0) {
        spinlock_release(&event_queue.lock);
        return false;
    }
    
    *event = event_queue.events[event_queue.head];
    event_queue.head = (event_queue.head + 1) % EVENT_QUEUE_SIZE;
    event_queue.count--;
    
    spinlock_release(&event_queue.lock);
    return true;
}

static widget_t *root_widget = NULL;

void event_loop_set_root(widget_t *root) {
    root_widget = root;
}

void event_dispatch(event_t *event) {
    if (!event) return;
    
    // Find target widget
    widget_t *target = NULL;
    
    switch (event->type) {
        case EVENT_MOUSE_PRESS:
        case EVENT_MOUSE_RELEASE:
        case EVENT_MOUSE_MOVE:
        case EVENT_MOUSE_SCROLL:
            if (root_widget) {
                target = widget_hit_test(root_widget, event->mouse.x, event->mouse.y);
            }
            
            // Handle hover state
            if (event->type == EVENT_MOUSE_MOVE) {
                if (target != hovered_widget) {
                    if (hovered_widget && hovered_widget->on_mouse_leave) {
                        hovered_widget->hovered = false;
                        hovered_widget->on_mouse_leave(hovered_widget);
                    }
                    if (target && target->on_mouse_enter) {
                        target->hovered = true;
                        target->on_mouse_enter(target);
                    }
                    hovered_widget = target;
                }
            }
            
            // Handle focus on press
            if (event->type == EVENT_MOUSE_PRESS && target) {
                widget_set_focus(target);
            }
            break;
            
        case EVENT_KEY_PRESS:
        case EVENT_KEY_RELEASE:
        case EVENT_TEXT_INPUT:
            // Send to focused widget
            target = focused_widget;
            break;
            
        default:
            break;
    }
    
    if (!target) return;
    
    // Dispatch to widget via vtable if present
    if (target->vtable && target->vtable->on_event) {
        target->vtable->on_event(target, event);
    }
    
    // Call specific event handlers if not handled by vtable (simplified)
    switch (event->type) {
        case EVENT_MOUSE_PRESS:
            if (target->on_mouse_press) {
                target->on_mouse_press(target, event->mouse.x, event->mouse.y, 
                                       event->mouse.button);
            }
            break;
        case EVENT_MOUSE_RELEASE:
            if (target->on_mouse_release) {
                target->on_mouse_release(target, event->mouse.x, event->mouse.y,
                                         event->mouse.button);
            }
            break;
        case EVENT_MOUSE_MOVE:
            if (target->on_mouse_move) {
                target->on_mouse_move(target, event->mouse.x, event->mouse.y);
            }
            break;
        case EVENT_KEY_PRESS:
            if (target->on_key_press) {
                target->on_key_press(target, event->key.keycode, event->key.modifiers);
            }
            break;
        default:
            break;
    }
}

// Timer management
static void process_timers(uint64_t current_time) {
    struct timer_entry **current = &timer_list;
    
    while (*current) {
        struct timer_entry *timer = *current;
        
        if (timer->fire_time <= current_time) {
            // Fire timer
            if (timer->callback) {
                timer->callback(timer->id, timer->data);
            }
            
            // Create timer event
            event_t event = {
                .type = EVENT_TIMER,
                .timestamp = current_time,
                .timer = {.timer_id = timer->id}
            };
            event_push(&event);
            
            if (timer->repeat) {
                // Reschedule
                timer->fire_time = current_time + timer->interval_ms;
                current = &(*current)->next;
            } else {
                // Remove timer
                *current = timer->next;
                kfree(timer);
            }
        } else {
            current = &(*current)->next;
        }
    }
}

uint32_t event_loop_add_timer(uint32_t interval_ms, bool repeat, 
                               void (*callback)(uint32_t, void*), void *data) {
    struct timer_entry *timer = kmalloc(sizeof(struct timer_entry));
    if (!timer) return 0;
    
    timer->id = next_timer_id++;
    timer->fire_time = timer_ticks() * 10 + interval_ms;
    timer->interval_ms = interval_ms;
    timer->repeat = repeat;
    timer->callback = callback;
    timer->data = data;
    timer->next = timer_list;
    timer_list = timer;
    
    return timer->id;
}

void event_loop_remove_timer(uint32_t timer_id) {
    struct timer_entry **current = &timer_list;
    
    while (*current) {
        if ((*current)->id == timer_id) {
            struct timer_entry *to_remove = *current;
            *current = (*current)->next;
            kfree(to_remove);
            return;
        }
        current = &(*current)->next;
    }
}

void event_loop_run(void) {
    event_loop_running = 1;
    
    uint64_t last_frame = timer_ticks() * 10;
    const uint64_t frame_time = 1000 / 60;   // 60 FPS
    
    while (event_loop_running) {
        // Process all pending events
        event_t event;
        while (event_poll(&event)) {
            event_dispatch(&event);
        }
        
        // Process timers
        uint64_t now = timer_ticks() * 10;
        process_timers(now);
        
        // Update animations
        animation_tick(now);
        
        // Render if needed
        if (g_compositor && compositor_needs_redraw(g_compositor)) {
            compositor_render(g_compositor);
            compositor_present(g_compositor);
        }
        
        // Calculate sleep time
        uint64_t next_frame = last_frame + frame_time;
        if (now >= next_frame) {
            last_frame = now;
        }
        
        // Yield to scheduler
        scheduler_yield();
    }
}

void event_loop_quit(void) {
    event_loop_running = 0;
}
