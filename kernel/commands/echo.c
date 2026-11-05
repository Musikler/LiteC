#include "../commands.h"
#include "../vga.h"
static void echo_fn(int argc,char**argv){
    for(int i=1;i<argc;i++){if(i>1)vga_putchar(' ');vga_write(argv[i]);}
    vga_putchar('\n');
}
REGISTER_COMMAND("echo","Print text",echo_fn)
