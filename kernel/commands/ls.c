#include "../commands.h"
#include "../vga.h"
#include "../fs.h"
static void cb(const char*name,int size){
    vga_write("  ");vga_write(name);vga_write("  (");
    char b[12];int i=0,n=size;if(n==0)b[i++]='0';
    while(n){b[i++]='0'+n%10;n/=10;}
    while(i--)vga_putchar(b[i]);
    vga_write(" bytes)\n");
}
static void ls_fn(int argc,char**argv){(void)argc;(void)argv;if(!fs_list(cb))vga_write("(empty)\n");}
REGISTER_COMMAND("ls","List files",ls_fn)
