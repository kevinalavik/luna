#pragma once

#include <stdint.h>
#include <tss.h>

#define MAX_CPUS 256
#define MSR_GS_BASE 0xC0000101

typedef struct cpu {
    struct cpu* self;
    uint32_t id;
    uint32_t lapic_id;
    tss_t tss;
    uint64_t gdt[7];
} cpu_t;

static inline cpu_t* current_cpu(void) {
    cpu_t* c;
    __asm__ volatile("mov %%gs:0, %0" : "=r"(c));
    return c;
}
