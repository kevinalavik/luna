#include <page.h>
#include <vmemmap.h>
#include <paging.h>

uintptr_t pfn_to_phys(uintptr_t pfn) {
    return pfn * PAGE_SIZE;
}

uintptr_t phys_to_pfn(uintptr_t phys) {
    return phys / PAGE_SIZE;
}

struct page* pfn_to_page(uintptr_t pfn) {
    return (struct page*)(VMEMMAP_BASE + pfn * sizeof(struct page));
}