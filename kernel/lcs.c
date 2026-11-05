#include "lcs.h"
#include "fs.h"
#include "string.h"
#include "serial.h"
#include "fb.h"
#include "gfx.h"

#define LCS_BASE    0x300000
#define SYSCALL_AT  0x4020

typedef struct {
    char magic[4];
    unsigned int version;
    unsigned int flags;
    unsigned int name_len;
    unsigned int entry_off;
    unsigned int payload_size;
    char name[64];
} __attribute__((packed)) lcs_hdr_t;

static int              lcs_active;
static volatile int     lcs_exited;

/* syscall implementations exposed to .lcs programs */
static void s_write(const char *s)      { while (*s) serial_putchar(*s++); }
static void s_putpixel(int x, int y, unsigned int c) { fb_put_pixel(x, y, c); }
static void s_fill(int x, int y, int w, int h, unsigned int c) { fb_fill_rect(x, y, w, h, c); }
static void s_text(int x, int y, const char *s, unsigned int c) { gfx_text(x, y, s, c); }
static void s_exit(void) { lcs_exited = 1; }

int lcs_run(const char *name) {
    if (!fb_available()) return -1;
    if (lcs_active)      return -2;

    char diskname[64];
    int i = 0;
    const char *pfx = "app_";
    while (pfx[i]) { diskname[i] = pfx[i]; i++; }
    int j = 0;
    while (name[j] && i < 60) diskname[i++] = name[j++];
    const char *ext = ".lcs";
    int e = 0;
    while (ext[e]) diskname[i++] = ext[e++];
    diskname[i] = 0;

    static unsigned char filebuf[FS_MAX_SIZE];
    int n = fs_read(diskname, (char*)filebuf, sizeof(filebuf));
    if (n < (int)sizeof(lcs_hdr_t)) return -3;

    lcs_hdr_t *h = (lcs_hdr_t*)filebuf;
    if (memcmp(h->magic, "LCS1", 4) != 0) return -4;
    if (h->version != 1) return -4;
    if (h->payload_size > 32768) return -5;
    if (n < (int)(sizeof(lcs_hdr_t) + h->payload_size)) return -6;

    /* copy payload into load base */
    unsigned char *dst = (unsigned char*)LCS_BASE;
    unsigned char *src = filebuf + sizeof(lcs_hdr_t);
    for (unsigned int k = 0; k < h->payload_size; k++) dst[k] = src[k];

    /* install syscall table */
    void **sys = (void**)SYSCALL_AT;
    sys[0] = (void*)s_write;
    sys[1] = (void*)s_putpixel;
    sys[2] = (void*)s_fill;
    sys[3] = (void*)s_text;
    sys[4] = (void*)s_exit;

    /* call entry */
    lcs_active = 1;
    lcs_exited = 0;
    void (*entry)(void) = (void(*)(void))(LCS_BASE + h->entry_off);
    entry();
    lcs_active = 0;

    return lcs_exited ? 0 : -7;
}