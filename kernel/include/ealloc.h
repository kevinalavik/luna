#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct ealloc_range {
    uintptr_t base; // physical, page aligned
    size_t length;  // bytes, page multiple
} ealloc_range_t;

void ealloc_init(void);
void* early_alloc(size_t bytes);

// for the pfndb
size_t ealloc_used_count(void);
bool ealloc_get_used(size_t index, ealloc_range_t* out);
void ealloc_disable(void);