#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void* memset(void* dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
void* memmove(void* dest, const void* src, size_t len);
int memcmp(const void* s1, const void* s2, size_t n);

size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strchr(const char* s, int c);

void itoa(int64_t val, char* buf, int base);
void uitoa(uint64_t val, char* buf, int base);

#endif /* STRING_H */
