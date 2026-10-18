.intel_syntax noprefix

/* Multiboot Header Constants */
.set MB_MAGIC,    0x1BADB002
.set MB_FLAGS,    0x00000003  /* ALIGN | MEMINFO */
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot, "a"
.align 4
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECKSUM

.section .bss
.align 4096
.global initial_pml4
.global initial_pdpt
.global initial_pd
initial_pml4:
    .skip 4096
initial_pdpt:
    .skip 4096
initial_pd:
    .skip 4096

.align 16
stack_bottom:
    .skip 32768 /* 32 KB initial kernel stack */
.global stack_top
stack_top:

mb_magic_saved:
    .long 0
mb_info_saved:
    .long 0

.section .rodata
.align 16
.global gdt64
.global gdt64_ptr
gdt64:
    .quad 0x0000000000000000  /* 0x00: Null descriptor */
    .quad 0x00AF9A000000FFFF  /* 0x08: 64-bit Kernel Code (DPL 0, RX) */
    .quad 0x00CF92000000FFFF  /* 0x10: 64-bit Kernel Data (DPL 0, RW) */
    .quad 0x00AFFA000000FFFF  /* 0x18: 64-bit User Code   (DPL 3, RX) */
    .quad 0x00CFF2000000FFFF  /* 0x20: 64-bit User Data   (DPL 3, RW) */
gdt64_end:

gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .quad gdt64

.section .text
.code32
.global _start
_start:
    cli

    /* Save Multiboot magic & info passed in EAX & EBX */
    mov mb_magic_saved, eax
    mov mb_info_saved, ebx

    /* Setup temporary stack */
    mov esp, offset stack_top

    /* Setup initial identity paging (first 1GB) using 2MB huge pages */
    /* PML4[0] -> initial_pdpt | 0x03 */
    mov eax, offset initial_pdpt
    or eax, 0x03
    mov [initial_pml4], eax
    mov dword ptr [initial_pml4 + 4], 0

    /* PDPT[0] -> initial_pd | 0x03 */
    mov eax, offset initial_pd
    or eax, 0x03
    mov [initial_pdpt], eax
    mov dword ptr [initial_pdpt + 4], 0

    /* Populate 512 entries of initial_pd (each 2MB) -> 1GB total */
    mov ecx, 0
map_pd_loop:
    mov eax, ecx
    shl eax, 21               /* eax = ecx * 2MB */
    or eax, 0x83              /* Present (1) | Writable (2) | Huge 2MB (0x80) */
    mov [initial_pd + ecx * 8], eax
    mov dword ptr [initial_pd + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 512
    jne map_pd_loop

    /* Load page table directory (PML4) into CR3 */
    mov eax, offset initial_pml4
    mov cr3, eax

    /* Enable PAE (Physical Address Extension) in CR4 bit 5 and OSFXSR/OSXMMEXCPT bits 9-10 */
    mov eax, cr4
    or eax, (1 << 5) | (3 << 9)
    mov cr4, eax

    /* Set Long Mode Enable (LME) in EFER MSR (0xC0000080, bit 8) */
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    /* Enable Paging (PG, bit 31), Protection (PE, bit 0), MP (bit 1) and clear EM (bit 2) in CR0 */
    mov eax, cr0
    and eax, ~(1 << 2)        /* Clear EM */
    or eax, 0x80000003        /* PG | PE | MP */
    mov cr0, eax

    /* Load 64-bit Global Descriptor Table */
    lgdt [gdt64_ptr]

    /* Far jump to 64-bit Long Mode code segment (0x08) */
    jmp 0x08:long_mode_entry

.code64
long_mode_entry:
    /* Reload 64-bit segment registers */
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    /* Setup 64-bit kernel stack */
    mov rsp, offset stack_top

    /* Pass Multiboot arguments to kmain according to System V AMD64 ABI:
       RDI: Multiboot info pointer (struct multiboot_info*)
       RSI: Multiboot magic (uint32_t) */
    mov edi, mb_info_saved
    mov esi, mb_magic_saved

    /* Call C kernel entry point */
    call kmain

    /* Fallback halt loop if kmain returns */
hang_loop:
    cli
    hlt
    jmp hang_loop
