/* vga.c */
#include "vga.h"
#include "console.h"
void vga_init(void)   { console_init(); }
void vga_clear(void)  { console_clear(); }
void vga_putchar(char c) { console_putchar(c); }
void vga_write(const char *s) { console_write(s); }
void vga_set_color(unsigned char c) { console_set_color(c); }