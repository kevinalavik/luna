#pragma once

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %b0, %w1" : : "a"(data), "Nd"(port) : "memory");
}