#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include_next <vmemmap.h>
#include <list.h>

#define PAGE_SHIFT 12

enum pageflags {
    PG_reserved,
    PG_buddy,
    NR_PAGEFLAGS,
};

#define PG_RESERVED (1UL << PG_reserved)
#define PG_BUDDY (1UL << PG_buddy)

struct page {
    _Atomic uint64_t flags;

    union {
        struct {
            struct list_node buddy;
            uint32_t order;
        };
    };

    _Atomic int32_t refcount;
};

static inline void page_set(struct page* p, uint64_t mask) {
    atomic_fetch_or(&p->flags, mask);
}

static inline void page_clear(struct page* p, uint64_t mask) {
    atomic_fetch_and(&p->flags, ~mask);
}

static inline bool page_test(struct page* p, uint64_t mask) {
    return atomic_load(&p->flags) & mask;
}

static inline bool page_buddy(struct page* p) {
    return page_test(p, PG_BUDDY);
}

static inline bool page_reserved(struct page* p) {
    return page_test(p, PG_RESERVED);
}

#define vmemmap ((struct page*)VMEMMAP_BASE)

static inline uintptr_t pfn_to_phys(uintptr_t pfn) {
    return pfn << PAGE_SHIFT;
}

static inline uintptr_t phys_to_pfn(uintptr_t phys) {
    return phys >> PAGE_SHIFT;
}

static inline struct page* pfn_to_page(uintptr_t pfn) {
    return vmemmap + pfn;
}

static inline uintptr_t page_to_pfn(struct page* p) {
    return (uintptr_t)(p - vmemmap);
}

static inline uintptr_t page_to_phys(struct page* p) {
    return pfn_to_phys(page_to_pfn(p));
}

static inline struct page* phys_to_page(uintptr_t phys) {
    return pfn_to_page(phys_to_pfn(phys));
}