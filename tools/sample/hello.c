#define SYSCALL(n) (*(void**)(0x4020 + (n)*4))

typedef void (*write_t)(const char*);
typedef void (*pix_t)(int,int,unsigned int);
typedef void (*fill_t)(int,int,int,int,unsigned int);
typedef void (*text_t)(int,int,const char*,unsigned int);
typedef void (*exit_t)(void);

__attribute__((section(".text.start")))
void _start(void) {
    ((write_t)SYSCALL(0))("Hello from .lcs!\n");
    ((fill_t)SYSCALL(2))(320, 240, 384, 288, 0x89B4FA);
    ((text_t)SYSCALL(3))(340, 260, "Hello, LiteC!", 0x1A1A2E);
    ((text_t)SYSCALL(3))(340, 290, "This is a .lcs program.", 0x1A1A2E);
    for (int i = 0; i < 20000000; i++) ;   /* visible pause */
    ((exit_t)SYSCALL(4))();
    for (;;) ;
}
