
#include <vmemmap.h>
#include <paging.h>
#include <mem.h>
#include <printf.h>
#include <align.h>
#include <page.h>
#include <ealloc.h>

void vmemmap_init(void) {
    paging_init();

    uintptr_t last_page = 0;
    struct page* pages = NULL;
    uintptr_t page_count = 0;
    uintptr_t mapped_pages = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* e = memmap->entries[i];

        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;

        uintptr_t start = ALIGN_UP(e->base, PAGE_SIZE) / PAGE_SIZE;
        uintptr_t end = (e->base + e->length) / PAGE_SIZE;

        for (uintptr_t pfn = start; pfn < end;) {
            uintptr_t addr = VMEMMAP_BASE + pfn * sizeof(struct page);
            uintptr_t page = ALIGN_DOWN(addr, PAGE_SIZE);

            if (page != last_page) {
                pages = early_alloc(PAGE_SIZE);
                if (!pages) {
                    printf("vmemmap: allocation failed\n");
                    return;
                }

                map_page(kernel_ptable, page, virt_to_phys(pages), PTE_RW);
                last_page = page;
                mapped_pages++;
            }

            uintptr_t offset = (addr - page) / sizeof(struct page);
            uintptr_t count = PAGE_SIZE / sizeof(struct page);

            while (offset < count && pfn < end) {
                pages[offset++].magic = 69 + pfn; // just a test
                pfn++;
                page_count++;
            }
        }
    }

    printf("vmemmap: covered %lu pages, allocated %lu KiB for page meta data\n",
           page_count,
           mapped_pages * PAGE_SIZE / 1024);
}
