#include "keyboard.h"
#include "serial.h"

static inline void outb(unsigned short p, unsigned char v) { __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }
static inline unsigned char inb(unsigned short p) { unsigned char r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(p)); return r; }

static const char kmap_en[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};
static const char kmap_en_s[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};

static const char kmap_de[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0',0xDF,'\'','\b',
    '\t','q','w','e','r','t','z','u','i','o','p',0xFC,'+','\n',
    0,'a','s','d','f','g','h','j','k','l',0xF6,0xE4,'#',
    0,'\\','y','x','c','v','b','n','m',',','.','-',0,'*',0,' '
};
static const char kmap_de_s[128] = {
    0,27,'!','"',0xA7,'$','%','&','/','(',')','=','?','`','\b',
    '\t','Q','W','E','R','T','Z','U','I','O','P',0xDC,'*','\n',
    0,'A','S','D','F','G','H','J','K','L',0xD6,0xC4,'\'',
    0,'|','Y','X','C','V','B','N','M',';',':','_',0,'*',0,' '
};

static int is_german = 0;
static int shift = 0;
static int skip_next = 0;
static int extended = 0;

void keyboard_set_layout(int german) { is_german = german; }

void keyboard_init(void) {
    for (int i = 0; i < 100 && (inb(0x64) & 1); i++) inb(0x60);
    outb(0x64, 0xAE);
    outb(0x64, 0x20);
    for (int i = 0; i < 100 && !(inb(0x64) & 1); i++) ;
    unsigned char cfg = inb(0x60);
    cfg |= 0x41; cfg &= ~0x10;
    outb(0x64, 0x60);
    for (int i = 0; i < 100 && (inb(0x64) & 2); i++) ;
    outb(0x60, cfg);
    outb(0x64, 0xAE);
    for (int i = 0; i < 100 && (inb(0x64) & 1); i++) inb(0x60);
    shift = 0; skip_next = 0; extended = 0;
}

int keyboard_getkey(void) {
    for (;;) {
        char s = serial_getchar_nonblocking();
        if (s) { if (s == '\r') return '\n'; if (s == 0x7F) return '\b'; return s; }
        
        if (inb(0x64) & 1) {
            unsigned char st = inb(0x64);
            if (st & 0x20) continue; /* mouse byte */
            unsigned char sc = inb(0x60);

            if (sc == 0xE0) { extended = 1; continue; }
            if (extended) {
                extended = 0;
                if (sc & 0x80) continue;
                if (sc == 0x48) return KEY_UP;
                if (sc == 0x50) return KEY_DOWN;
                if (sc == 0x4B) return KEY_LEFT;
                if (sc == 0x4D) return KEY_RIGHT;
                continue;
            }

            if (sc == 0x2A || sc == 0x36) { shift = 1; continue; }
            if (sc == 0xAA || sc == 0xB6) { shift = 0; continue; }
            if (sc & 0x80) continue;

            const char *map = shift ? (is_german ? kmap_de_s : kmap_en_s) : (is_german ? kmap_de : kmap_en);
            char c = map[sc];
            if (c) return c;
        }
    }
}

char keyboard_getchar(void) { return (char)keyboard_getkey(); }