#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <limine.h>

extern uintptr_t hhdm_offset;
extern struct limine_memmap_response* memmap;

void mem_init();

void* phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(const void* virt);
