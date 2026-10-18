#include "kernel/serial.h"
#include "kernel/io.h"
#include "lib/string.h"

#define COM1 0x3F8

static int serial_is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00);    // Disable all interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
    outb(COM1 + 1, 0x00);    //                  (hi byte)
    outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_putchar(char c) {
    while (!serial_is_transmit_empty())
        ;
    outb(COM1, (uint8_t)c);
}

void serial_puts(const char* str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(*str++);
    }
}

void serial_write(const char* buf, size_t len) {
    if (!buf) return;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(buf[i]);
    }
}

void serial_print_hex(uint64_t val) {
    char buf[20];
    buf[0] = '0';
    buf[1] = 'x';
    uitoa(val, buf + 2, 16);
    serial_puts(buf);
}

void serial_print_dec(uint64_t val) {
    char buf[25];
    uitoa(val, buf, 10);
    serial_puts(buf);
}
