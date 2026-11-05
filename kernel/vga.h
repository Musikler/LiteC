#ifndef VGA_H
#define VGA_H
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_write(const char *s);
void vga_set_color(unsigned char c);
#endif
