#ifndef STRING_H
#define STRING_H

typedef unsigned int size_t;

void  *memset(void *s, int c, size_t n);
void  *memcpy(void *d, const void *s, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, unsigned int n);
size_t strlen(const char *s);
char  *strcpy(char *d, const char *s);

#endif