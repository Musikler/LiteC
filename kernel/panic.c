#include "panic.h"
#include "serial.h"

#define VGA ((unsigned short*)0xB8000)
#define W 80
#define H 25

/* color attributes: high nibble = bg, low nibble = fg (VGA text) */
#define A_BG      0x40   /* black on red   */
#define A_BORDER  0x4F   /* white on red   */
#define A_TITLE   0x4F
#define A_BODY    0x07   /* light gray on black */
#define A_LABEL   0x0E   /* yellow on black */
#define A_HINT    0x0B   /* cyan on black   */
#define A_INFO    0x08   /* dark gray       */

static inline void port_outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p));
}

static unsigned int rd_cr0(void){ unsigned int v; __asm__ volatile("mov %%cr0,%0":"=r"(v)); return v; }
static unsigned int rd_cr2(void){ unsigned int v; __asm__ volatile("mov %%cr2,%0":"=r"(v)); return v; }
static unsigned int rd_cr3(void){ unsigned int v; __asm__ volatile("mov %%cr3,%0":"=r"(v)); return v; }

static unsigned int get_eip(void) {
    unsigned int eip;
    __asm__ __volatile__("call 1f\n1:\tpopl %0" : "=r"(eip));
    return eip;
}

static void disable_cursor(void) {
    port_outb(0x3D4, 0x0A);
    port_outb(0x3D5, 0x20);
}

static void fill_attr(unsigned char attr) {
    unsigned short v = ((unsigned short)attr << 8) | ' ';
    for (int i = 0; i < W * H; i++) VGA[i] = v;
}

static void write_char(int row, int col, unsigned char c, unsigned char attr) {
    if (row < 0 || row >= H || col < 0 || col >= W) return;
    VGA[row * W + col] = ((unsigned short)attr << 8) | c;
}

static void write_at(int row, int col, const char *s, unsigned char attr) {
    while (*s && col < W - 1) {
        write_char(row, col, (unsigned char)*s, attr);
        col++; s++;
    }
}

static void hex8_at(int row, int col, unsigned int v, unsigned char attr) {
    static const char hx[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        write_char(row, col + i, (unsigned char)hx[v & 0xF], attr);
        v >>= 4;
    }
}

static void kv(int row, int lcol, const char *label, int hcol, unsigned int v) {
    write_at(row, lcol, label, A_LABEL);
    hex8_at(row, hcol, v, A_BODY);
}

static void border_top(void) {
    write_char(0, 0, 0xC9, A_BORDER);
    for (int c = 1; c < W - 1; c++) write_char(0, c, 0xCD, A_BORDER);
    write_char(0, W - 1, 0xBB, A_BORDER);
}
static void border_mid(void) {
    write_char(2, 0, 0xCC, A_BORDER);
    for (int c = 1; c < W - 1; c++) write_char(2, c, 0xCD, A_BORDER);
    write_char(2, W - 1, 0xB9, A_BORDER);
}
static void border_bottom(int row) {
    write_char(row, 0, 0xC8, A_BORDER);
    for (int c = 1; c < W - 1; c++) write_char(row, c, 0xCD, A_BORDER);
    write_char(row, W - 1, 0xBC, A_BORDER);
}
static void side(int row) {
    write_char(row, 0, 0xBA, A_BORDER);
    write_char(row, W - 1, 0xBA, A_BORDER);
}
static void body_row(int row) {
    for (int c = 1; c < W - 1; c++) write_char(row, c, ' ', A_BODY);
    side(row);
}

static int contains(const char *hay, const char *needle) {
    if (!hay || !needle || !*needle) return 0;
    for (; *hay; hay++) {
        const char *a = hay, *b = needle;
        while (*a && *b && *a == *b) { a++; b++; }
        if (!*b) return 1;
    }
    return 0;
}

static void serial_str(const char *s) { while (s && *s) serial_putchar(*s++); }

__attribute__((noreturn))
static void panic_impl(const char *reason, regs_t *r,
                       const char *h1, const char *h2, const char *h3) {
    __asm__ volatile("cli");
    disable_cursor();
    fill_attr(A_BG);

    /* Frame */
    border_top();
    body_row(1);
    {
        const char *title = "LITEC  (BUMBLEBEE)  KERNEL  PANIC";
        int len = 0; while (title[len]) len++;
        int start = 1 + (78 - len) / 2;
        write_at(1, start, title, A_TITLE);
    }
    border_mid();
    for (int row = 3; row <= 19; row++) body_row(row);

    /* REASON */
    write_at(3, 3, "REASON:", A_LABEL);
    write_at(4, 5, reason ? reason : "(no reason given)",
             reason ? A_BODY : A_INFO);

    /* REGISTERS */
    write_at(6, 3, "REGISTERS:", A_LABEL);
    unsigned int eax, ebx, ecx, edx, esi, edi, ebp, esp, eip, cs, eflags, intno;
    if (r) {
        eax = r->eax; ebx = r->ebx; ecx = r->ecx; edx = r->edx;
        esi = r->esi; edi = r->edi; ebp = r->ebp; esp = r->esp;
        eip = r->eip; cs = r->cs;   eflags = r->eflags; intno = r->int_no;
    } else {
        __asm__ volatile("mov %%eax,%0":"=r"(eax));
        __asm__ volatile("mov %%ebx,%0":"=r"(ebx));
        __asm__ volatile("mov %%ecx,%0":"=r"(ecx));
        __asm__ volatile("mov %%edx,%0":"=r"(edx));
        __asm__ volatile("mov %%esi,%0":"=r"(esi));
        __asm__ volatile("mov %%edi,%0":"=r"(edi));
        __asm__ volatile("mov %%ebp,%0":"=r"(ebp));
        __asm__ volatile("mov %%esp,%0":"=r"(esp));
        eflags = 0; __asm__ volatile("pushf; pop %0" : "=r"(eflags));
        cs = 0; __asm__ volatile("mov %%cs,%0":"=r"(cs));
        eip = get_eip() - 5;         /* return address minus call instruction */
        intno = 0;
    }

    kv(7,  5, "EAX=",  9, eax);
    kv(7, 19, "EBX=", 23, ebx);
    kv(7, 33, "ECX=", 37, ecx);
    kv(7, 47, "EDX=", 51, edx);

    kv(8,  5, "ESI=",  9, esi);
    kv(8, 19, "EDI=", 23, edi);
    kv(8, 33, "EBP=", 37, ebp);
    kv(8, 47, "ESP=", 51, esp);

    kv(9,  5, "EIP=",    9, eip);
    kv(9, 19, "CS=",    23, cs);
    kv(9, 33, "EFLAGS=", 41, eflags);
    kv(9, 51, "INT=",   55, intno);

    kv(10,  5, "CR0=",  9, rd_cr0());
    kv(10, 19, "CR2=", 23, rd_cr2());
    kv(10, 33, "CR3=", 37, rd_cr3());

    /* WHAT TO DO */
    write_at(12, 3, "WHAT TO DO:", A_LABEL);

    if (!h1) {
        if (contains(reason, "ATA")) {
            h1 = "Ensure a data disk is attached (QEMU: -drive file=data.img,if=ide).";
            h2 = "VMware: add ide0:0.fileName=\"data.img\" to the .vmx (VM off).";
            h3 = "Delete data.img to force LiteFS to reformat on next boot.";
        } else if (contains(reason, "LiteFS")) {
            h1 = "The on-disk filesystem is corrupt or was not recognised.";
            h2 = "From the shell: 'format'  (erases all files on the data disk!).";
            h3 = "Or delete data.img and let LiteFS reformat on next boot.";
        } else if (contains(reason, "keyboard") || contains(reason, "PS/2")) {
            h1 = "The PS/2 keyboard controller did not respond.";
            h2 = "Check the VM's USB controller setting; then reboot the VM.";
            h3 = "Serial console on COM1 works as a keyboard fallback.";
        } else if (contains(reason, "assert")) {
            h1 = "An internal invariant was violated.";
            h2 = "Note the reason above; that is the culprit expression.";
            h3 = "Rebuild and retest; if it repeats, bisect the last change.";
        } else {
            h1 = "Reboot the VM. If the panic repeats, note the reason above.";
            h2 = "Run 'make clean && make' to rule out a stale build.";
            h3 = "Bisect the last change; the register dump above is the clue.";
        }
    }
    write_at(13, 5, "1.", A_LABEL); write_at(13, 8, h1 ? h1 : "", A_HINT);
    write_at(14, 5, "2.", A_LABEL); write_at(14, 8, h2 ? h2 : "", A_HINT);
    write_at(15, 5, "3.", A_LABEL); write_at(15, 8, h3 ? h3 : "", A_HINT);

    /* Build info */
    write_at(17, 3, "LiteC (Bumblebee) v0.3    built "
             __DATE__ " " __TIME__, A_INFO);

    /* Halt */
    write_at(19, 3,
             "System halted. Power off the VM (QEMU: Ctrl+A then X; VMware: Power Off).",
             A_LABEL);

    border_bottom(20);

    /* Mirror to serial for logging */
    serial_putchar('\n');
    serial_str("*** LiteC PANIC ***\n");
    serial_str(reason ? reason : "(no reason)");
    serial_putchar('\n');

    for (;;) __asm__ volatile("hlt");
}

void panic(const char *reason) {
    panic_impl(reason, 0, 0, 0, 0);
}
void panic_regs(const char *reason, regs_t *r) {
    panic_impl(reason, r, 0, 0, 0);
}
void panic_ex(const char *reason, const char *h1, const char *h2, const char *h3) {
    panic_impl(reason, 0, h1, h2, h3);
}