#pragma once

#include <page.h>
#include <mem.h>
#include <string.h>
#if ARCH == x86_64
#include <paging.h>
#endif

#define MAX_ORDER 11

static inline unsigned order_for_pages(size_t pages) {
    unsigned o = 0;
    while ((1ul << o) < pages)
        o++;
    return o;
}

void pmm_init(void);
struct page* alloc_pages(unsigned order);
void free_pages(struct page* p, unsigned order);
uintptr_t pmm_free_count(void);