#include <vmemmap.h>
#include <paging.h>

uintptr_t vmemmap_base = 0;

void vmemmap_init() {
    paging_init();
}