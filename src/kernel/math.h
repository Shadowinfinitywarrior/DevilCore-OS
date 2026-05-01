#ifndef DEVILCORE_MATH_H
#define DEVILCORE_MATH_H

#include <stdint.h>

// Simple math functions for kernel use
// These are simplified implementations

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Square root using Newton's method
static inline float sqrtf(float x) {
    if (x <= 0.0f) return 0.0f;
    
    float guess = x;
    float epsilon = 0.00001f;
    
    for (int i = 0; i < 20; i++) {
        float next = 0.5f * (guess + x / guess);
        if (next - guess < epsilon && guess - next < epsilon) {
            break;
        }
        guess = next;
    }
    
    return guess;
}

// Power function (simplified for positive bases)
static inline float powf(float base, float exp) {
    if (exp == 0.0f) return 1.0f;
    if (base == 0.0f) return 0.0f;
    if (base == 1.0f) return 1.0f;
    
    // Simple implementation for integer exponents
    if (exp == 2.0f) return base * base;
    if (exp == 3.0f) return base * base * base;
    if (exp == 0.5f) return sqrtf(base);
    
    // For other cases, use repeated multiplication for small exponents
    float result = 1.0f;
    float e = exp;
    float b = base;
    
    if (e < 0) {
        e = -e;
        b = 1.0f / b;
    }
    
    while (e >= 1.0f) {
        result *= b;
        e -= 1.0f;
    }
    
    return result;
}

// Sine approximation using Taylor series
static inline float sinf(float x) {
    // Normalize to [-PI, PI]
    while (x > M_PI) x -= 2.0f * M_PI;
    while (x < -M_PI) x += 2.0f * M_PI;
    
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    
    // Taylor series: sin(x) = x - x^3/3! + x^5/5! - x^7/7!
    return x - x3 / 6.0f + x5 / 120.0f - x7 / 5040.0f;
}

// Absolute value
static inline float fabsf(float x) {
    return (x < 0.0f) ? -x : x;
}

// Minimum and maximum
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#endif // DEVILCORE_MATH_H
