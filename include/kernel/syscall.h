#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/isr.h"

#define SYS_YIELD     0
#define SYS_GETPID    1
#define SYS_SLEEP     2
#define SYS_PRINT     3
#define SYS_MALLOC    4
#define SYS_FREE      5
#define SYS_FS_CREATE 6
#define SYS_FS_WRITE  7
#define SYS_FS_READ   8
#define SYS_FS_LIST   9
#define SYS_EXIT      10

void syscall_init(void);
void syscall_dispatcher(interrupt_frame_t* frame);

/* C System Call wrappers executing software trap int 0x80 */
int64_t sys_call(int64_t num, int64_t a1, int64_t a2, int64_t a3);
int32_t sys_getpid(void);
void    sys_yield(void);
void    sys_sleep(uint64_t ticks);
void    sys_print(const char* msg);
void*   sys_malloc(size_t size);
void    sys_free(void* ptr);

#endif /* SYSCALL_H */
