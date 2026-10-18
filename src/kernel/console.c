#include "kernel/console.h"
#include "kernel/serial.h"
#include "kernel/io.h"
#include "lib/string.h"

static volatile uint16_t* const VGA_BUFFER = (volatile uint16_t*)0xB8000;
static size_t console_row = 0;
static size_t console_col = 0;
static uint8_t console_color = 0x0F; // White on black

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | ((uint16_t)color << 8);
}

static void update_cursor(size_t x, size_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void console_init(void) {
    console_row = 0;
    console_col = 0;
    console_color = (VGA_COLOR_BLACK << 4) | VGA_COLOR_WHITE;
    console_clear();
}

void console_set_color(uint8_t fg, uint8_t bg) {
    console_color = (bg << 4) | (fg & 0x0F);
}

void console_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = vga_entry(' ', console_color);
        }
    }
    console_row = 0;
    console_col = 0;
    update_cursor(0, 0);
}

static void console_scroll(void) {
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = VGA_BUFFER[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', console_color);
    }
    console_row = VGA_HEIGHT - 1;
}

void console_putchar(char c) {
    // Send to serial port simultaneously
    if (c == '\n') {
        serial_putchar('\r');
    }
    serial_putchar(c);

    // Send to VGA text buffer
    if (c == '\n') {
        console_col = 0;
        if (++console_row >= VGA_HEIGHT) {
            console_scroll();
        }
    } else if (c == '\r') {
        console_col = 0;
    } else if (c == '\t') {
        console_col = (console_col + 4) & ~3;
        if (console_col >= VGA_WIDTH) {
            console_col = 0;
            if (++console_row >= VGA_HEIGHT) {
                console_scroll();
            }
        }
    } else if (c == '\b') {
        if (console_col > 0) {
            console_col--;
            VGA_BUFFER[console_row * VGA_WIDTH + console_col] = vga_entry(' ', console_color);
        }
    } else {
        const size_t index = console_row * VGA_WIDTH + console_col;
        VGA_BUFFER[index] = vga_entry(c, console_color);
        if (++console_col >= VGA_WIDTH) {
            console_col = 0;
            if (++console_row >= VGA_HEIGHT) {
                console_scroll();
            }
        }
    }
    update_cursor(console_col, console_row);
}

void console_puts(const char* str) {
    if (!str) return;
    while (*str) {
        console_putchar(*str++);
    }
}

void kvprintf(const char* format, va_list args) {
    char buf[65];
    for (size_t i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            console_putchar(format[i]);
            continue;
        }

        i++; // Move past '%'
        if (format[i] == '\0') break;

        switch (format[i]) {
            case 'c': {
                char c = (char)va_arg(args, int);
                console_putchar(c);
                break;
            }
            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                console_puts(s);
                break;
            }
            case 'd':
            case 'i': {
                int64_t val = va_arg(args, int64_t);
                itoa(val, buf, 10);
                console_puts(buf);
                break;
            }
            case 'u': {
                uint64_t val = va_arg(args, uint64_t);
                uitoa(val, buf, 10);
                console_puts(buf);
                break;
            }
            case 'x': {
                uint64_t val = va_arg(args, uint64_t);
                uitoa(val, buf, 16);
                console_puts(buf);
                break;
            }
            case 'p': {
                uint64_t val = (uint64_t)va_arg(args, void*);
                console_puts("0x");
                uitoa(val, buf, 16);
                console_puts(buf);
                break;
            }
            case 'b': {
                uint64_t val = va_arg(args, uint64_t);
                uitoa(val, buf, 2);
                console_puts(buf);
                break;
            }
            case '%': {
                console_putchar('%');
                break;
            }
            default: {
                console_putchar('%');
                console_putchar(format[i]);
                break;
            }
        }
    }
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    kvprintf(format, args);
    va_end(args);
}
