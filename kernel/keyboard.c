#include "keyboard.h"
#include "serial.h"

static inline void outb(unsigned short p, unsigned char v)
{ __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }
static inline unsigned char inb(unsigned short p)
{ unsigned char r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(p)); return r; }

static const char kmap[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};
static const char kmap_s[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};
static int shift = 0;
static int skip_next = 0;

void keyboard_init(void) {
    /* 1. drain whatever the BIOS left in the output buffer */
    for (int i = 0; i < 100 && (inb(0x64) & 1); i++) inb(0x60);

    /* 2. enable first PS/2 port (keyboard) */
    outb(0x64, 0xAE);

    /* 3. read controller config byte */
    outb(0x64, 0x20);
    for (int i = 0; i < 100 && !(inb(0x64) & 1); i++) ;
    unsigned char cfg = inb(0x60);

    /* 4. set bit0 = keyboard IRQ (harmless with IF=0),
          set bit6 = translate set-2 → set-1 (this is the VMware fix),
          clear bit4 = keyboard enabled                              */
    cfg |= 0x01;
    cfg |= 0x40;
    cfg &= ~0x10;

    /* 5. write config byte back */
    outb(0x64, 0x60);
    for (int i = 0; i < 100 && (inb(0x64) & 2); i++) ;
    outb(0x60, cfg);

    /* 6. re-enable keyboard and drain the ACK */
    outb(0x64, 0xAE);
    for (int i = 0; i < 100 && (inb(0x64) & 1); i++) inb(0x60);

    shift = 0;
    skip_next = 0;
}

char keyboard_getchar(void) {
    for (;;) {
        char s = serial_getchar_nonblocking();
        if (s) { if (s == '\r') return '\n'; if (s == 0x7F) return '\b'; return s; }
        if (inb(0x64) & 1) {
            unsigned char st = inb(0x64);
            unsigned char sc = inb(0x60);
            if (st & 0x20) continue;            /* ← mouse byte, skip */
            if (skip_next) { skip_next = 0; continue; }
            if (sc == 0xE0 || sc == 0xE1) { skip_next = 1; continue; }
            if (sc == 0x2A || sc == 0x36) { shift = 1; continue; }
            if (sc == 0xAA || sc == 0xB6) { shift = 0; continue; }
            if (sc & 0x80) continue;
            char c = shift ? kmap_s[sc] : kmap[sc];
            if (c) return c;
        }
    }
}