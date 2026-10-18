#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include "boot/multiboot.h"

#define PAGE_SIZE 4096
#define PAGE_ALIGN_DOWN(addr) ((uint64_t)(addr) & ~0xFFFULL)
#define PAGE_ALIGN_UP(addr)   (((uint64_t)(addr) + 0xFFFULL) & ~0xFFFULL)

/* Physical Memory Manager (PMM) */
void pmm_init(multiboot_info_t* mboot);
uint64_t pmm_alloc_frame(void);
void pmm_free_frame(uint64_t phys_addr);
uint64_t pmm_get_total_frames(void);
uint64_t pmm_get_free_frames(void);
uint64_t pmm_get_used_frames(void);

/* Kernel Heap Allocator */
void kmalloc_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t new_size);
void heap_dump_stats(void);

#endif /* MEMORY_H */
