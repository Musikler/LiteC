#include "sound.h"

static inline void outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p));
}
static inline unsigned char inb(unsigned short p) {
    unsigned char r; __asm__ volatile("inb %1,%0" : "=a"(r) : "Nd"(p)); return r;
}

void speaker_play(unsigned int frequency) {
    if (frequency == 0) { speaker_stop(); return; }
    unsigned int div = 1193180 / frequency;
    outb(0x43, 0xB6);                     /* PIT channel 2, mode 3 (square wave) */
    outb(0x42, (unsigned char)(div & 0xFF));
    outb(0x42, (unsigned char)((div >> 8) & 0xFF));
    outb(0x61, inb(0x61) | 3);            /* enable speaker */
}

void speaker_stop(void) {
    outb(0x61, inb(0x61) & 0xFC);
}

/* Busy-wait delay calibrated roughly for QEMU/VMware */
static void delay_ms(unsigned int ms) {
    for (unsigned int i = 0; i < ms; i++) {
        for (volatile unsigned int j = 0; j < 20000; j++)
            __asm__ volatile("pause");
    }
}

void speaker_beep(unsigned int frequency, unsigned int duration_ms) {
    speaker_play(frequency);
    delay_ms(duration_ms);
    speaker_stop();
}