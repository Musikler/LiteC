/* console.h */
#ifndef CONSOLE_H
#define CONSOLE_H
void console_init(void);
void console_clear(void);
void console_putchar(char c);
void console_write(const char *s);
void console_set_color(unsigned char vga_attr);
#endif