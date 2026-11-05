#include "serial.h"
static inline void outb(unsigned short p,unsigned char v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline unsigned char inb(unsigned short p){unsigned char r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}
#define COM1 0x3F8
void serial_init(void){
    outb(COM1+1,0);outb(COM1+3,0x80);outb(COM1+0,3);
    outb(COM1+1,0);outb(COM1+3,3);outb(COM1+2,0xC7);outb(COM1+4,0x0B);
}
void serial_putchar(char c){
    while(!(inb(COM1+5)&0x20));
    if(c=='\n')outb(COM1,'\r');
    outb(COM1,c);
}
char serial_getchar_nonblocking(void){return (inb(COM1+5)&1)?inb(COM1):0;}
