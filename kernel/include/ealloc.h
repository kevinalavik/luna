#pragma once

#include <stdint.h>
#include <stddef.h>

// early bump allocator
void ealloc_init();
void* early_alloc(size_t bytes); // rounded to PAGE_SIZE