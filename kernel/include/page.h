#pragma once

#include <stdint.h>

struct page {
    uint64_t magic; // just a test
};

uintptr_t pfn_to_phys(uintptr_t pfn);
uintptr_t phys_to_pfn(uintptr_t phys);
struct page* pfn_to_page(uintptr_t pfn);