#ifndef BOOTINFO_H
#define BOOTINFO_H

typedef struct {
    unsigned int magic;      /* 0x424F4F54 = "BOOT" */
    unsigned int fb_addr;
    unsigned int fb_w, fb_h;
    unsigned int fb_pitch;
    unsigned int fb_bpp;
} bootinfo_t;

/* Volatile: silences -Warray-bounds on the hard-coded physical address.
   Callers read individual fields; they never dereference the struct as an array. */
static inline volatile bootinfo_t *bootinfo(void) {
    return (volatile bootinfo_t*)(unsigned long)0x0600;
}

#endif