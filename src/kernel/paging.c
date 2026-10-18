#include "kernel/paging.h"
#include "kernel/memory.h"
#include "kernel/console.h"
#include "lib/string.h"

extern uint64_t initial_pml4;
extern uint64_t initial_pdpt;
extern uint64_t initial_pd;

static uint64_t* kernel_pml4 = NULL;

static inline uint64_t pml4_index(uint64_t virt) { return (virt >> 39) & 0x1FF; }
static inline uint64_t pdpt_index(uint64_t virt) { return (virt >> 30) & 0x1FF; }
static inline uint64_t pd_index(uint64_t virt)   { return (virt >> 21) & 0x1FF; }
static inline uint64_t pt_index(uint64_t virt)   { return (virt >> 12) & 0x1FF; }

void paging_invlpg(uint64_t virt) {
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

void paging_init(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    kernel_pml4 = (uint64_t*)(cr3 & ~0xFFFULL);

    kprintf("[+] Paging (x86-64): 4-level paging active (PML4 CR3: %p)\n", (void*)kernel_pml4);
}

int paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    if (!kernel_pml4) return -1;

    uint64_t pml4_idx = pml4_index(virt);
    uint64_t pdpt_idx = pdpt_index(virt);
    uint64_t pd_idx   = pd_index(virt);
    uint64_t pt_idx   = pt_index(virt);

    // 1. PML4 -> PDPT
    if (!(kernel_pml4[pml4_idx] & PTE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        if (!frame) return -1;
        memset((void*)frame, 0, PAGE_SIZE);
        kernel_pml4[pml4_idx] = frame | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);
    }
    uint64_t* pdpt = (uint64_t*)(kernel_pml4[pml4_idx] & ~0xFFFULL);

    // 2. PDPT -> PD
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        if (!frame) return -1;
        memset((void*)frame, 0, PAGE_SIZE);
        pdpt[pdpt_idx] = frame | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);
    }
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);

    // 3. PD -> PT
    // If PD entry is currently a 2MB huge page, split it into 512 x 4KB pages
    if ((pd[pd_idx] & PTE_PRESENT) && (pd[pd_idx] & PTE_HUGE)) {
        uint64_t base_2mb = pd[pd_idx] & ~0x1FFFFFULL;
        uint64_t frame = pmm_alloc_frame();
        if (!frame) return -1;
        uint64_t* new_pt = (uint64_t*)frame;
        for (int i = 0; i < 512; i++) {
            new_pt[i] = (base_2mb + (i * PAGE_SIZE)) | PTE_PRESENT | PTE_WRITABLE;
        }
        pd[pd_idx] = frame | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);
    } else if (!(pd[pd_idx] & PTE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        if (!frame) return -1;
        memset((void*)frame, 0, PAGE_SIZE);
        pd[pd_idx] = frame | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);
    }
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);

    // 4. Install PTE
    pt[pt_idx] = (phys & ~0xFFFULL) | flags | PTE_PRESENT;
    paging_invlpg(virt);

    return 0;
}

void paging_unmap_page(uint64_t virt) {
    if (!kernel_pml4) return;

    uint64_t pml4_idx = pml4_index(virt);
    uint64_t pdpt_idx = pdpt_index(virt);
    uint64_t pd_idx   = pd_index(virt);
    uint64_t pt_idx   = pt_index(virt);

    if (!(kernel_pml4[pml4_idx] & PTE_PRESENT)) return;
    uint64_t* pdpt = (uint64_t*)(kernel_pml4[pml4_idx] & ~0xFFFULL);

    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return;
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);

    if (!(pd[pd_idx] & PTE_PRESENT) || (pd[pd_idx] & PTE_HUGE)) return;
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);

    pt[pt_idx] = 0;
    paging_invlpg(virt);
}

uint64_t paging_get_phys(uint64_t virt) {
    if (!kernel_pml4) return 0;

    uint64_t pml4_idx = pml4_index(virt);
    if (!(kernel_pml4[pml4_idx] & PTE_PRESENT)) return 0;
    uint64_t* pdpt = (uint64_t*)(kernel_pml4[pml4_idx] & ~0xFFFULL);

    uint64_t pdpt_idx = pdpt_index(virt);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return 0;
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);

    uint64_t pd_idx = pd_index(virt);
    if (!(pd[pd_idx] & PTE_PRESENT)) return 0;
    if (pd[pd_idx] & PTE_HUGE) {
        return (pd[pd_idx] & ~0x1FFFFFULL) + (virt & 0x1FFFFFULL);
    }
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);

    uint64_t pt_idx = pt_index(virt);
    if (!(pt[pt_idx] & PTE_PRESENT)) return 0;

    return (pt[pt_idx] & ~0xFFFULL) + (virt & 0xFFFULL);
}
