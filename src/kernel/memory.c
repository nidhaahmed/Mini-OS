#include "kernel/memory.h"
#include "kernel/console.h"
#include "lib/string.h"

extern uint8_t kernel_start;
extern uint8_t kernel_end;

#define BLOCK_MAGIC 0xDEADBEEF
#define HEAP_INITIAL_SIZE (4 * 1024 * 1024) // 4 MB kernel heap

static uint8_t* pmm_bitmap = NULL;
static uint64_t total_memory_bytes = 0;
static uint64_t total_frames = 0;
static uint64_t used_frames = 0;

static inline void bitmap_set(uint64_t frame) {
    pmm_bitmap[frame / 8] |= (1 << (frame % 8));
}

static inline void bitmap_clear(uint64_t frame) {
    pmm_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static inline int bitmap_test(uint64_t frame) {
    return (pmm_bitmap[frame / 8] & (1 << (frame % 8))) != 0;
}

void pmm_init(multiboot_info_t* mboot) {
    if (!mboot || !(mboot->flags & MULTIBOOT_INFO_MEM_MAP)) {
        kprintf("[!] PMM Warning  : Multiboot memory map not provided, using fallback 128MB\n");
        total_memory_bytes = 128 * 1024 * 1024;
    } else {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)((uint64_t)mboot->mmap_addr);
        uint64_t mmap_end = mboot->mmap_addr + mboot->mmap_length;

        while ((uint64_t)mmap < mmap_end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                uint64_t top = mmap->addr + mmap->len;
                if (top > total_memory_bytes) {
                    total_memory_bytes = top;
                }
            }
            mmap = (multiboot_memory_map_t*)((uint64_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    total_frames = total_memory_bytes / PAGE_SIZE;
    uint64_t bitmap_size = (total_frames + 7) / 8;

    // Place bitmap directly after kernel_end, aligned to 4KB
    uint64_t kend = (uint64_t)&kernel_end;
    pmm_bitmap = (uint8_t*)PAGE_ALIGN_UP(kend);
    uint64_t pmm_reserved_end = (uint64_t)pmm_bitmap + bitmap_size;

    // Initially mark all frames as used (1)
    memset(pmm_bitmap, 0xFF, bitmap_size);
    used_frames = total_frames;

    // Free available RAM regions according to multiboot mmap
    if (mboot && (mboot->flags & MULTIBOOT_INFO_MEM_MAP)) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)((uint64_t)mboot->mmap_addr);
        uint64_t mmap_end = mboot->mmap_addr + mboot->mmap_length;

        while ((uint64_t)mmap < mmap_end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                uint64_t start_frame = PAGE_ALIGN_UP(mmap->addr) / PAGE_SIZE;
                uint64_t end_frame = PAGE_ALIGN_DOWN(mmap->addr + mmap->len) / PAGE_SIZE;

                for (uint64_t f = start_frame; f < end_frame && f < total_frames; f++) {
                    bitmap_clear(f);
                    used_frames--;
                }
            }
            mmap = (multiboot_memory_map_t*)((uint64_t)mmap + mmap->size + sizeof(mmap->size));
        }
    } else {
        // Fallback: free 1MB to 128MB
        for (uint64_t f = (1024 * 1024) / PAGE_SIZE; f < total_frames; f++) {
            bitmap_clear(f);
            used_frames--;
        }
    }

    // Reserve lower 1MB and kernel + bitmap area
    uint64_t reserved_end_frame = PAGE_ALIGN_UP(pmm_reserved_end) / PAGE_SIZE;
    for (uint64_t f = 0; f < reserved_end_frame && f < total_frames; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
        }
    }

    kprintf("[+] PMM Bitmap   : Manages %u MB RAM (%u frames, %u free, %u used)\n",
            (uint32_t)(total_memory_bytes / (1024 * 1024)),
            (uint32_t)total_frames,
            (uint32_t)(total_frames - used_frames),
            (uint32_t)used_frames);
}

uint64_t pmm_alloc_frame(void) {
    for (uint64_t i = 0; i < total_frames; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_frames++;
            return i * PAGE_SIZE;
        }
    }
    kprintf("[!] PMM ERROR: Out of physical memory frames!\n");
    return 0;
}

void pmm_free_frame(uint64_t phys_addr) {
    uint64_t frame = phys_addr / PAGE_SIZE;
    if (frame < total_frames && bitmap_test(frame)) {
        bitmap_clear(frame);
        used_frames--;
    }
}

uint64_t pmm_get_total_frames(void) { return total_frames; }
uint64_t pmm_get_free_frames(void)  { return total_frames - used_frames; }
uint64_t pmm_get_used_frames(void)  { return used_frames; }

/* --------------------------------------------------------------------------
   Kernel Heap Allocator: Boundary-Tag Coalescing Allocator
   -------------------------------------------------------------------------- */

typedef struct heap_block {
    uint32_t magic;
    uint32_t is_free;
    size_t   size;
    struct heap_block* next;
    struct heap_block* prev;
} __attribute__((aligned(16))) heap_block_t;

#define HEAP_HEADER_SIZE sizeof(heap_block_t)

static uint8_t kernel_heap_area[HEAP_INITIAL_SIZE] __attribute__((aligned(4096)));
static heap_block_t* heap_head = NULL;

void kmalloc_init(void) {
    heap_head = (heap_block_t*)kernel_heap_area;
    heap_head->magic   = BLOCK_MAGIC;
    heap_head->is_free = 1;
    heap_head->size    = HEAP_INITIAL_SIZE - HEAP_HEADER_SIZE;
    heap_head->next    = NULL;
    heap_head->prev    = NULL;

    kprintf("[+] Heap (kmalloc): Initialized %u KB free kernel heap at %p\n",
            HEAP_INITIAL_SIZE / 1024, (void*)kernel_heap_area);
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Align size to 16 bytes
    size = (size + 15) & ~15ULL;

    heap_block_t* current = heap_head;
    while (current) {
        if (current->magic != BLOCK_MAGIC) {
            kprintf("[!] Heap Corruption Detected at 0x%p!\n", (void*)current);
            return NULL;
        }

        if (current->is_free && current->size >= size) {
            // Check if block can be split
            if (current->size >= size + HEAP_HEADER_SIZE + 32) {
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)current + HEAP_HEADER_SIZE + size);
                new_block->magic   = BLOCK_MAGIC;
                new_block->is_free = 1;
                new_block->size    = current->size - size - HEAP_HEADER_SIZE;
                new_block->next    = current->next;
                new_block->prev    = current;

                if (current->next) {
                    current->next->prev = new_block;
                }
                current->next = new_block;
                current->size = size;
            }

            current->is_free = 0;
            return (void*)((uint8_t*)current + HEAP_HEADER_SIZE);
        }
        current = current->next;
    }

    kprintf("[!] kmalloc failed: Out of heap memory for request %u bytes\n", (uint32_t)size);
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - HEAP_HEADER_SIZE);
    if (block->magic != BLOCK_MAGIC) {
        kprintf("[!] kfree ERROR: Invalid pointer or buffer overflow at 0x%p\n", ptr);
        return;
    }

    block->is_free = 1;

    // Coalesce with next block if free
    if (block->next && block->next->is_free) {
        block->size += HEAP_HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Coalesce with prev block if free
    if (block->prev && block->prev->is_free) {
        block->prev->size += HEAP_HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void* kcalloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - HEAP_HEADER_SIZE);
    if (block->size >= new_size) {
        return ptr;
    }

    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}

void heap_dump_stats(void) {
    size_t free_bytes = 0;
    size_t used_bytes = 0;
    size_t free_blocks = 0;
    size_t used_blocks = 0;

    heap_block_t* cur = heap_head;
    while (cur) {
        if (cur->is_free) {
            free_bytes += cur->size;
            free_blocks++;
        } else {
            used_bytes += cur->size;
            used_blocks++;
        }
        cur = cur->next;
    }

    kprintf("[Heap Stats] Used: %u bytes (%u blocks) | Free: %u bytes (%u blocks)\n",
            (uint32_t)used_bytes, (uint32_t)used_blocks,
            (uint32_t)free_bytes, (uint32_t)free_blocks);
}
