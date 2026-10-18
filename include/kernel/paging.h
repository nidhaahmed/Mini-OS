#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

#define PTE_PRESENT   (1ULL << 0)
#define PTE_WRITABLE  (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_PWT       (1ULL << 3)
#define PTE_PCD       (1ULL << 4)
#define PTE_ACCESSED  (1ULL << 5)
#define PTE_DIRTY     (1ULL << 6)
#define PTE_HUGE      (1ULL << 7)
#define PTE_GLOBAL    (1ULL << 8)
#define PTE_NX        (1ULL << 63)

#ifndef PAGE_ALIGN_DOWN
#define PAGE_ALIGN_DOWN(addr) ((uint64_t)(addr) & ~0xFFFULL)
#endif

#ifndef PAGE_ALIGN_UP
#define PAGE_ALIGN_UP(addr)   (((uint64_t)(addr) + 0xFFFULL) & ~0xFFFULL)
#endif

void paging_init(void);
int  paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void paging_unmap_page(uint64_t virt);
uint64_t paging_get_phys(uint64_t virt);
void paging_invlpg(uint64_t virt);

#endif /* PAGING_H */
