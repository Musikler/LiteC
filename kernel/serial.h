#ifndef SERIAL_H
#define SERIAL_H
void serial_init(void);
void serial_putchar(char c);
char serial_getchar_nonblocking(void);
#endif
