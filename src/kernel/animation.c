#include "devilui.h"
#include "memory.h"
#include "slab.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

extern void kprintf(const char *fmt, ...);

// Fixed-point math for animations (16.16 format)
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)
#define FP_MASK ((1 << FP_SHIFT) - 1)

static inline int32_t to_fp(float x) { return (int32_t)(x * FP_ONE); }
static inline float from_fp(int32_t x) { return (float)x / FP_ONE; }
static inline int32_t mul_fp(int32_t a, int32_t b) { 
    return (int32_t)(((int64_t)a * (int64_t)b) >> FP_SHIFT); 
}

static animation_t *animation_list = NULL;
static uint32_t animation_count = 0;

// Integer-based easing functions (input and output are 0 to FP_ONE)
static int32_t easing_linear(int32_t t) {
    return t;
}

static int32_t easing_ease_in(int32_t t) {
    return mul_fp(mul_fp(t, t), t);
}

static int32_t easing_ease_out(int32_t t) {
    int32_t f = FP_ONE - t;
    return FP_ONE - mul_fp(mul_fp(f, f), f);
}

static int32_t easing_ease_in_out(int32_t t) {
    if (t < FP_ONE / 2) {
        int32_t half = t * 2;
        return mul_fp(mul_fp(half, half), half) / 2;
    } else {
        int32_t f = FP_ONE * 2 - t * 2;
        return FP_ONE - mul_fp(mul_fp(f, f), f) / 2;
    }
}

static int32_t easing_apply_fp(easing_t ease, int32_t t) {
    switch (ease) {
        case EASING_LINEAR:        return easing_linear(t);
        case EASING_EASE_IN:       return easing_ease_in(t);
        case EASING_EASE_OUT:      return easing_ease_out(t);
        case EASING_EASE_IN_OUT:   return easing_ease_in_out(t);
        default:                   return easing_linear(t);
    }
}

void animation_init(void) {
    animation_list = NULL;
    animation_count = 0;
    kprintf("Animation system initialized\n");
}

static animation_t *animation_create(void) {
    animation_t *anim = slab_alloc(cache_64);
    if (!anim) return NULL;
    
    memset(anim, 0, sizeof(animation_t));
    
    // Add to list
    anim->next = animation_list;
    animation_list = anim;
    animation_count++;
    
    return anim;
}

static void animation_free(animation_t *anim) {
    if (!anim) return;
    
    // Remove from list
    animation_t **current = &animation_list;
    while (*current) {
        if (*current == anim) {
            *current = anim->next;
            break;
        }
        current = &(*current)->next;
    }
    
    slab_free(cache_64, anim);
    animation_count--;
}

animation_t *animate_float(float *target, float to, uint32_t duration_ms, easing_t easing) {
    if (!target || duration_ms == 0) return NULL;
    
    animation_t *anim = animation_create();
    if (!anim) return NULL;
    
    anim->target_float = target;
    anim->values.f.from = *target;
    anim->values.f.to = to;
    anim->duration_ms = duration_ms;
    anim->easing = easing;
    anim->start_time = timer_ticks() * 10;  // Convert to approximate ms
    anim->active = true;
    
    return anim;
}

animation_t *animate_int(int32_t *target, int32_t to, uint32_t duration_ms, easing_t easing) {
    if (!target || duration_ms == 0) return NULL;
    
    animation_t *anim = animation_create();
    if (!anim) return NULL;
    
    anim->target_int = target;
    anim->values.i.from = *target;
    anim->values.i.to = to;
    anim->duration_ms = duration_ms;
    anim->easing = easing;
    anim->start_time = timer_ticks() * 10;
    anim->active = true;
    
    return anim;
}

animation_t *animate_color(color_t *target, color_t to, uint32_t duration_ms, easing_t easing) {
    if (!target || duration_ms == 0) return NULL;
    
    animation_t *anim = animation_create();
    if (!anim) return NULL;
    
    anim->target_color = target;
    anim->values.c.from = *target;
    anim->values.c.to = to;
    anim->duration_ms = duration_ms;
    anim->easing = easing;
    anim->start_time = timer_ticks() * 10;
    anim->active = true;
    
    return anim;
}

void animation_tick(uint64_t current_time_ms) {
    animation_t *anim = animation_list;
    animation_t *to_remove[32];
    uint32_t remove_count = 0;
    
    while (anim) {
        if (!anim->active) {
            anim = anim->next;
            continue;
        }
        
        // Calculate progress
        uint64_t elapsed = current_time_ms - anim->start_time;
        float t = (float)elapsed / (float)anim->duration_ms;
        
        uint8_t complete = false;
        
        if (t >= 1.0f) {
            t = 1.0f;
            complete = true;
        }
        
        // Apply easing using fixed-point math
        int32_t t_fp = (int32_t)(t * FP_ONE);
        if (t_fp > FP_ONE) t_fp = FP_ONE;
        int32_t et_fp = easing_apply_fp(anim->easing, t_fp);
        
        // Update target values
        if (anim->target_float) {
            int32_t from_val = (int32_t)(anim->values.f.from * FP_ONE);
            int32_t to_val = (int32_t)(anim->values.f.to * FP_ONE);
            int32_t val_fp = from_val + mul_fp(to_val - from_val, et_fp);
            *anim->target_float = (float)val_fp / FP_ONE;
        }
        
        if (anim->target_int) {
            int32_t range = anim->values.i.to - anim->values.i.from;
            *anim->target_int = anim->values.i.from + (range * et_fp) / FP_ONE;
        }
        
        if (anim->target_color) {
            int32_t from_r = anim->values.c.from.r;
            int32_t to_r = anim->values.c.to.r;
            anim->target_color->r = (uint8_t)(from_r + ((to_r - from_r) * et_fp) / FP_ONE);
            
            int32_t from_g = anim->values.c.from.g;
            int32_t to_g = anim->values.c.to.g;
            anim->target_color->g = (uint8_t)(from_g + ((to_g - from_g) * et_fp) / FP_ONE);
            
            int32_t from_b = anim->values.c.from.b;
            int32_t to_b = anim->values.c.to.b;
            anim->target_color->b = (uint8_t)(from_b + ((to_b - from_b) * et_fp) / FP_ONE);
            
            int32_t from_a = anim->values.c.from.a;
            int32_t to_a = anim->values.c.to.a;
            anim->target_color->a = (uint8_t)(from_a + ((to_a - from_a) * et_fp) / FP_ONE);
        }
        
        // Handle completion
        if (complete) {
            if (anim->loop) {
                // Restart animation
                anim->start_time = current_time_ms;
            } else {
                anim->active = false;
                
                // Call completion callback
                if (anim->on_complete) {
                    anim->on_complete(anim, anim->callback_data);
                }
                
                // Queue for removal
                if (remove_count < 32) {
                    to_remove[remove_count++] = anim;
                }
            }
        }
        
        anim = anim->next;
    }
    
    // Remove completed animations
    for (uint32_t i = 0; i < remove_count; i++) {
        animation_free(to_remove[i]);
    }
}

void animation_stop(animation_t *anim) {
    if (!anim) return;
    anim->active = false;
    animation_free(anim);
}

// Convenience functions for widget animations
void window_animate_open(widget_t *win) {
    if (!win) return;
    
    // Start with reduced opacity and scale
    win->surface->opacity = 0.0f;
    
    // Animate to full opacity
    animate_float(&win->surface->opacity, 1.0f, 150, EASING_EASE_OUT);
}

void window_animate_close(widget_t *win, void (*done)(void*), void *data) {
    if (!win) return;
    
    animation_t *anim = animate_float(&win->surface->opacity, 0.0f, 100, EASING_EASE_IN);
    if (anim) {
        // Note: callback type mismatch - disabled for now
        anim->callback_data = data;
    }
}
