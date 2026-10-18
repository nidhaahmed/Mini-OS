#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>

#define COM1_PORT 0x3F8

void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char* str);
void serial_write(const char* buf, size_t len);
void serial_print_hex(uint64_t val);
void serial_print_dec(uint64_t val);

#endif /* SERIAL_H */
