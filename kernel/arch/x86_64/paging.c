#include <paging.h>
#include <stddef.h>
#include <mem.h>
#include <align.h>
#include <printf.h>
#include <asm.h>
#include <string.h>
#include <ealloc.h>
#include <limine.h>

ptable_t* kernel_ptable = NULL;
extern char __limine_requests_start[];
extern char __limine_requests_end[];
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_executable_address_request
    addr_request = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID, .revision = 0};

uint64_t kernel_phys = 0;
uint64_t kernel_virt = 0;

#define PT_MASK 0x1FFULL
#define PML4_INDEX(va) (((va) >> 39) & PT_MASK)
#define PDPT_INDEX(va) (((va) >> 30) & PT_MASK)
#define PD_INDEX(va) (((va) >> 21) & PT_MASK)
#define PT_INDEX(va) (((va) >> 12) & PT_MASK)
#define PAGE_OFFSET(va) ((va) & 0xFFFULL)

void* page_alloc() {
    void* ptr = (void*)(virt_to_phys(
        early_alloc(PAGE_SIZE))); // idk why i was stupid and made early_alloc return HHDM values...
    if (ptr)
        return ptr;

    return NULL; // no other allocator we can try right now. TODO: add like `page_alloc(1)` once its
                 // ready
}

uint64_t ptable_virt_to_phys(ptable_t* pml4, uint64_t vaddr) {
    ptable_t* pdpt;
    ptable_t* pd;
    ptable_t* pt;
    uint64_t entry;

    /* page map level 4 entry */
    entry = pml4->entries[PML4_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT))
        return 0;

    pdpt = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);

    /* page directory pointer table entry (long ahh intel name) */
    entry = pdpt->entries[PDPT_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT))
        return 0;

    pd = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);

    /* page directory entry */
    entry = pd->entries[PD_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT))
        return 0;

    pt = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);

    /* page table entry */
    entry = pt->entries[PT_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT))
        return 0;

    uint64_t paddr = entry & PTE_ADDR_MASK;
    return paddr + PAGE_OFFSET(vaddr);
}

int map_page(ptable_t* pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    ptable_t* pdpt;
    ptable_t* pd;
    ptable_t* pt;
    uint64_t entry;
    uint64_t phys;

    pml4 = phys_to_virt((uint64_t)pml4);

    entry = pml4->entries[PML4_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT)) {
        phys = (uint64_t)page_alloc();
        if (!phys) {
            printf("arch: failed to allocate pdpt\n");
            return -1;
        }

        pdpt = (ptable_t*)((uint64_t)phys + hhdm_offset);
        memset(pdpt, 0, PAGE_SIZE);
        pml4->entries[PML4_INDEX(vaddr)] = phys | PTE_PRESENT | PTE_RW;
        entry = pml4->entries[PML4_INDEX(vaddr)];
    }

    pdpt = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);
    entry = pdpt->entries[PDPT_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT)) {
        phys = (uint64_t)page_alloc();
        if (!phys) {
            printf("arch: failed to allocate pd\n");
            return -1;
        }

        pd = (ptable_t*)((uint64_t)phys + hhdm_offset);
        memset(pd, 0, PAGE_SIZE);
        pdpt->entries[PDPT_INDEX(vaddr)] = phys | PTE_PRESENT | PTE_RW;
        entry = pdpt->entries[PDPT_INDEX(vaddr)];
    }

    pd = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);
    entry = pd->entries[PD_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT)) {
        phys = (uint64_t)page_alloc();
        if (!phys) {
            printf("arch: failed to alloc pt\n");
            return -1;
        }
        pt = (ptable_t*)((uint64_t)phys + hhdm_offset);
        memset(pt, 0, PAGE_SIZE);
        pd->entries[PD_INDEX(vaddr)] = phys | PTE_PRESENT | PTE_RW;
        entry = pd->entries[PD_INDEX(vaddr)];
    }

    pt = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);
    entry = pt->entries[PT_INDEX(vaddr)];
    pt->entries[PT_INDEX(vaddr)] = (paddr & PTE_ADDR_MASK) | flags | PTE_PRESENT;
    return 0;
}

int unmap_page(ptable_t* pml4, uint64_t vaddr) {
    ptable_t* pdpt;
    ptable_t* pd;
    ptable_t* pt;
    uint64_t entry;

    pml4 = phys_to_virt((uint64_t)pml4);

    entry = pml4->entries[PML4_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT)) {
        printf("arch: paging: unmap: missing pml4 entry VA=%p", vaddr);
        return -1;
    }

    pdpt = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);
    entry = pdpt->entries[PDPT_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT)) {
        printf("arch: paging: unmap: missing pdpt entry VA=%p", vaddr);
        return -1;
    }

    pd = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);
    entry = pd->entries[PD_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT)) {
        printf("arch: paging: unmap: missing pd entry VA=%p", vaddr);
        return -1;
    }

    pt = (ptable_t*)((uint64_t)(entry & PTE_ADDR_MASK) + hhdm_offset);
    entry = pt->entries[PT_INDEX(vaddr)];
    if (!(entry & PTE_PRESENT)) {
        printf("arch: paging: unmap: page not mapped VA=%p", vaddr);
        return -1;
    }

    pt->entries[PT_INDEX(vaddr)] = 0;
    invlpg(vaddr);
    return 0;
}

ptable_t* ptable_create() {
    void* page = page_alloc();
    if (!page) {
        printf("arch: failed to create new ptable\n");
        return NULL;
    }

    ptable_t* pml4 = (ptable_t*)((uint64_t)page + hhdm_offset);
    memset(pml4, 0, PAGE_SIZE);
    if (kernel_ptable) {
        ptable_t* kpml4 = (ptable_t*)((uint64_t)kernel_ptable + hhdm_offset);
        for (int i = 256; i < 512; i++)
            pml4->entries[i] = kpml4->entries[i];
    }
    return page;
}

void ptable_load(ptable_t* pt) {
    __asm__ volatile("mov %0, %%cr3" ::"r"((uint64_t)pt) : "memory");
}

void paging_init() {
    kernel_ptable = ptable_create();

    if (!addr_request.response) {
        printf("arch: no information about where the kernel is located (paging is cooked)!\n");
        halt();
    }

    kernel_virt = addr_request.response->virtual_base;
    kernel_phys = addr_request.response->physical_base;

#define MAP_SECTION(vstart, vend, flags)                                                           \
    do {                                                                                           \
        uint64_t _v = ALIGN_DOWN((uint64_t)(vstart), PAGE_SIZE);                                   \
        uint64_t _e = ALIGN_UP((uint64_t)(vend), PAGE_SIZE);                                       \
        for (; _v < _e; _v += PAGE_SIZE) {                                                         \
            uint64_t _p = _v - kernel_virt + kernel_phys;                                          \
            map_page(kernel_ptable, _v, _p, (flags));                                              \
        }                                                                                          \
    } while (0)

    MAP_SECTION(__limine_requests_start, __limine_requests_end, PTE_NX);
    MAP_SECTION(__text_start, __text_end, 0);
    MAP_SECTION(__rodata_start, __rodata_end, PTE_NX);
    MAP_SECTION(__data_start, __data_end, PTE_RW | PTE_NX);

    ptable_t* new_pml4 = (ptable_t*)((uint64_t)kernel_ptable + hhdm_offset);
    ptable_t* boot_pml4 = (ptable_t*)(read_cr3() + hhdm_offset);

    for (int i = 256; i < 512; i++)
        new_pml4->entries[i] = boot_pml4->entries[i];

    printf("arch: mappings:\n");
    printf("arch:     limine_requests: 0x%llx -> 0x%llx [RO NX]\n",
           (uint64_t)__limine_requests_start,
           (uint64_t)__limine_requests_end);
    printf("arch:     text:            0x%llx -> 0x%llx [RX]\n",
           (uint64_t)__text_start,
           (uint64_t)__text_end);
    printf("arch:     rodata:          0x%llx -> 0x%llx [RO NX]\n",
           (uint64_t)__rodata_start,
           (uint64_t)__rodata_end);
    printf("arch:     data/bss:        0x%llx -> 0x%llx [RW NX]\n",
           (uint64_t)__data_start,
           (uint64_t)__data_end);
    ptable_load(kernel_ptable);
    printf("arch: loaded kernel CR3: %p\n", (void*)read_cr3());
}