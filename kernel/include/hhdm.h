#pragma once

#include <stdbool.h>
#include <stdint.h>

bool hhdm_present(void);
uintptr_t hhdm_offset(void);

void* phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(const void* virt);
