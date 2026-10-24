#include <vmemmap.h>
#include <paging.h>
#include <mem.h>
#include <printf.h>
#include <align.h>
#include <page.h>
#include <ealloc.h>
#include <string.h>

#define MAX_BLOCK_PAGES (1UL << 10)
#define PAGES_PER_ARRAY_PAGE (PAGE_SIZE / sizeof(struct page))

uintptr_t vmemmap_max_pfn;

static uintptr_t mapped_end;
static uintptr_t mapped_pages;

static bool has_struct_page(uint64_t type) {
    switch (type) {
    case LIMINE_MEMMAP_USABLE:
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
    case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return true;
    default:
        return false;
    }
}

static bool vmemmap_populate(uintptr_t start_pfn, uintptr_t end_pfn) {
    uintptr_t va = ALIGN_DOWN(VMEMMAP_BASE + start_pfn * sizeof(struct page), PAGE_SIZE);
    uintptr_t va_end = ALIGN_UP(VMEMMAP_BASE + end_pfn * sizeof(struct page), PAGE_SIZE);

    if (va < mapped_end)
        va = mapped_end;

    for (; va < va_end; va += PAGE_SIZE) {
        void* frame = early_alloc(PAGE_SIZE);
        if (!frame)
            return false;

        map_page(kernel_ptable, va, virt_to_phys(frame), PTE_RW);

        struct page* p = (struct page*)va;
        memset(p, 0, PAGE_SIZE);

        for (uintptr_t i = 0; i < PAGES_PER_ARRAY_PAGE; i++) {
            p[i].flags = PG_RESERVED;
            p[i].refcount = 1;
        }

        mapped_pages++;
    }

    if (va_end > mapped_end)
        mapped_end = va_end;

    return true;
}

void vmemmap_init(void) {
    paging_init();

    uintptr_t max_pfn = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* e = memmap->entries[i];

        if (!has_struct_page(e->type))
            continue;

        uintptr_t end = ALIGN_UP(e->base + e->length, PAGE_SIZE) / PAGE_SIZE;
        if (end > max_pfn)
            max_pfn = end;
    }

    max_pfn = ALIGN_UP(max_pfn, MAX_BLOCK_PAGES);

    if (max_pfn * sizeof(struct page) > VMEMMAP_SIZE) {
        printf("vmemmap: physical memory exceeds vmemmap range\n");
        return;
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* e = memmap->entries[i];

        if (!has_struct_page(e->type))
            continue;

        uintptr_t start = ALIGN_DOWN(e->base, PAGE_SIZE) / PAGE_SIZE;
        uintptr_t end = ALIGN_UP(e->base + e->length, PAGE_SIZE) / PAGE_SIZE;

        start = ALIGN_DOWN(start, MAX_BLOCK_PAGES);
        end = ALIGN_UP(end, MAX_BLOCK_PAGES);

        if (!vmemmap_populate(start, end)) {
            printf("vmemmap: allocation failed\n");
            return;
        }
    }

    vmemmap_max_pfn = max_pfn;

    printf("vmemmap: %lu struct pages initialised, %lu KiB of metadata, max pfn %lu\n",
           mapped_pages * PAGES_PER_ARRAY_PAGE,
           mapped_pages * PAGE_SIZE / 1024,
           max_pfn);
}