#include "mouse.h"
#include "serial.h"
static inline void outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p));
}
static inline unsigned char inb(unsigned short p) {
    unsigned char r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(p)); return r;
}

static void mouse_wait(int type) {
    for (int i = 0; i < 100000; i++) {
        unsigned char st = inb(0x64);
        if (type == 0) { if (st & 1) return; }      /* wait for output */
        else           { if (!(st & 2)) return; }   /* wait for input  */
    }
}

static void mouse_write(unsigned char val) {
    mouse_wait(1); outb(0x64, 0xD4);   /* tell controller: next byte -> mouse */
    mouse_wait(1); outb(0x60, val);
    mouse_wait(0); inb(0x60);          /* drain ACK */
}

static int mx = 512, my = 384, mb = 0, new_click = 0;
static int cycle = 0;
static unsigned char pkt[3];

void mouse_init(void) {
    /* Enable the auxiliary PS/2 port (mouse) */
    mouse_wait(1); outb(0x64, 0xA8);

    /* Enable IRQ12 in the controller config byte */
    mouse_wait(1); outb(0x64, 0x20);
    mouse_wait(0); unsigned char cfg = inb(0x60);
    cfg |= 0x02;    /* IRQ12 on */
    cfg &= ~0x20;   /* clock on  */
    mouse_wait(1); outb(0x64, 0x60);
    mouse_wait(1); outb(0x60, cfg);

    mouse_write(0xF6);        /* set defaults */
    mouse_write(0xF3);        /* set sample rate */
    mouse_write(0x64);        /* 100 samples/sec */
    mouse_write(0xF4);        /* enable data reporting */

    while (inb(0x64) & 1) inb(0x60);   /* flush stale bytes */
    cycle = 0;
}

void mouse_poll(void) {
    new_click = 0;
    while (1) {
        unsigned char st = inb(0x64);
        if (!(st & 1))   break;        /* nothing pending */
        if (!(st & 0x20)) break;       /* keyboard byte — leave for keyboard driver */

        unsigned char d = inb(0x60);
        if (d == 0xFA || d == 0xFE) continue;   /* ACK / resend */

        /* Resync: if we are expecting a header, bit 3 MUST be set */
        if (cycle == 0 && !(d & 0x08)) {
            continue; 
        }

        pkt[cycle++] = d;
        if (cycle == 3) {
            cycle = 0;
            
            /* Ignore packets with overflow */
            if (pkt[0] & 0xC0) continue; 

            int dx = (int)pkt[1];
            int dy = (int)pkt[2];

            /* 
             * Handle BOTH Two's Complement (if byte > 127) 
             * AND Sign-Magnitude (if header bit is set).
             */
            if (dx > 127) dx -= 256;
            else if (pkt[0] & 0x10) dx = -dx;

            if (dy > 127) dy -= 256;
            else if (pkt[0] & 0x20) dy = -dy;

            mx += dx;  
            my -= dy; /* Standard PS/2 sends positive Y for UP */

            if (mx < 0)   mx = 0;
            if (my < 0)   my = 0;
            if (mx > 1023) mx = 1023;
            if (my > 767)  my = 767;

            int nb = pkt[0] & 0x07;
            if ((nb & 1) && !(mb & 1)) new_click = 1;
            mb = nb;
        }
    }
}

int mouse_x(void) { return mx; }
int mouse_y(void) { return my; }
int mouse_btn(void) { return mb; }
int mouse_clicked(void) { return new_click; }