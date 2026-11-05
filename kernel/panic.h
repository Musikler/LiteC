#ifndef PANIC_H
#define PANIC_H

/* Register frame passed by exception handlers (for future IDT work). */
typedef struct {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags;
} __attribute__((packed)) regs_t;

__attribute__((noreturn)) void panic(const char *reason);
__attribute__((noreturn)) void panic_regs(const char *reason, regs_t *r);
__attribute__((noreturn)) void panic_ex(const char *reason,
                                         const char *h1,
                                         const char *h2,
                                         const char *h3);

/* Simple assert: panic("assert failed: expr") */
#define assert(x) do { if (!(x)) panic("assert failed: " #x); } while (0)

#endif