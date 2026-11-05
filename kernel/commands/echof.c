#include "../commands.h"
#include "../vga.h"
#include "../fs.h"
static void cat_fn(int argc,char**argv){
    if(argc<2){vga_write("usage: echof <file>\n");return;}
    char buf[FS_MAX_SIZE];
    int n=fs_read(argv[1],buf,sizeof(buf));
    if(n<0){vga_write("no such file\n");return;}
    for(int i=0;i<n;i++)vga_putchar(buf[i]);
    if(n&&buf[n-1]!='\n')vga_putchar('\n');
}
REGISTER_COMMAND("echof","Print file contents",cat_fn)
