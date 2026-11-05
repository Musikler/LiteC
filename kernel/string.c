#include "string.h"

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *d, const void *s, size_t n) {
    unsigned char *dp = d;
    const unsigned char *sp = s;
    while (n--) *dp++ = *sp++;
    return d;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    while (n--) { if (*x != *y) return *x - *y; x++; y++; }
    return 0;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, unsigned int n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    if ((int)n < 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

char *strcpy(char *d, const char *s) {
    char *r = d;
    while ((*d++ = *s++)) ;
    return r;
}