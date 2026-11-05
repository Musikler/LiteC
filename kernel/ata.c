#include "ata.h"

static inline void outb(unsigned short p, unsigned char v)
{ __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }
static inline unsigned char inb(unsigned short p)
{ unsigned char r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(p)); return r; }
static inline void outw(unsigned short p, unsigned short v)
{ __asm__ volatile("outw %0,%1" :: "a"(v), "Nd"(p)); }
static inline unsigned short inw(unsigned short p)
{ unsigned short r; __asm__ volatile("inw %1,%0" : "=a"(r) : "Nd"(p)); return r; }
static inline void insw(unsigned short p, void *a, unsigned int n)
{ __asm__ volatile("cld; rep insw" : "+D"(a), "+c"(n) : "d"(p) : "memory"); }
static inline void outsw(unsigned short p, const void *a, unsigned int n)
{ __asm__ volatile("cld; rep outsw" : "+S"(a), "+c"(n) : "d"(p) : "memory"); }

#define IO   0x1F0
#define CTRL 0x3F6
#define SR_BSY  0x80
#define SR_DRDY 0x40
#define SR_DRQ  0x08
#define SR_ERR  0x01
#define CMD_READ  0x20
#define CMD_WRITE 0x30
#define CMD_FLUSH 0xE7
#define CMD_ID    0xEC

static int have_disk = 0;
static void delay400(void) { for (int i = 0; i < 4; i++) inb(CTRL); }

static int wait_ready(void) {
    for (int i = 0; i < 1000000; i++) {
        unsigned char s = inb(IO + 7);
        if (s & SR_ERR) return -1;
        if (!(s & SR_BSY) && (s & SR_DRDY)) return 0;
    }
    return -1;
}

static int wait_drq(void) {
    for (int i = 0; i < 1000000; i++) {
        unsigned char s = inb(IO + 7);
        if (s & SR_ERR) return -1;
        if (!(s & SR_BSY) && (s & SR_DRQ)) return 0;
    }
    return -1;
}

int ata_init(void) {
    have_disk = 0;
    outb(CTRL, 0x04); delay400();
    outb(CTRL, 0x00); delay400();
    outb(IO + 6, 0xA0); delay400();
    outb(IO + 2, 0);
    outb(IO + 3, 0); outb(IO + 4, 0); outb(IO + 5, 0);
    outb(IO + 7, CMD_ID); delay400();
    unsigned char s = inb(IO + 7);
    if (s == 0) return -1;
    while (s & SR_BSY) s = inb(IO + 7);
    if (inb(IO + 4) || inb(IO + 5)) return -1;
    if (wait_drq() != 0) return -1;
    for (int i = 0; i < 256; i++) inw(IO);
    have_disk = 1;
    return 0;
}

int ata_present(void) { return have_disk; }

int ata_read_lba(unsigned int lba, unsigned char count, void *buf) {
    if (!have_disk) return -1;
    if (wait_ready() != 0) return -1;
    outb(IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IO + 1, 0);
    outb(IO + 2, count);
    outb(IO + 3, (unsigned char)(lba));
    outb(IO + 4, (unsigned char)(lba >> 8));
    outb(IO + 5, (unsigned char)(lba >> 16));
    outb(IO + 7, CMD_READ);
    for (int i = 0; i < count; i++) {
        if (wait_drq() != 0) return -1;
        insw(IO, (unsigned char*)buf + i * 512, 256);
    }
    return 0;
}

int ata_write_lba(unsigned int lba, unsigned char count, const void *buf) {
    if (!have_disk) return -1;
    if (wait_ready() != 0) return -1;           /* DRDY before issuing */
    outb(IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IO + 1, 0);
    outb(IO + 2, count);
    outb(IO + 3, (unsigned char)(lba));
    outb(IO + 4, (unsigned char)(lba >> 8));
    outb(IO + 5, (unsigned char)(lba >> 16));
    outb(IO + 7, CMD_WRITE);
    for (int i = 0; i < count; i++) {
        if (wait_drq() != 0) return -1;
        outsw(IO, (const unsigned char*)buf + i * 512, 256);
    }
    if (wait_ready() != 0) return -1;           /* drain BUSY */
    outb(IO + 7, CMD_FLUSH);
    for (int i = 0; i < 1000000; i++) {
        unsigned char s = inb(IO + 7);
        if (s & SR_ERR) return -1;
        if (!(s & SR_BSY) && (s & SR_DRDY)) return 0;
    }
    return -1;
}