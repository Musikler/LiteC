#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "commands.h"
#include "fs.h"
#include "string.h"
#define LINE_MAX 256
static char line[LINE_MAX];
static int len;
static void read_line(void){
    len=0;
    for(;;){
        char c=keyboard_getchar();
        if(c=='\n'){vga_putchar('\n');line[len]=0;return;}
        if(c=='\b'){if(len>0){len--;vga_putchar('\b');}}
        else if(c>=32&&c<127&&len<LINE_MAX-1){line[len++]=c;vga_putchar(c);}
    }
}
static void execute(char*s){
    char*argv[16];int argc=0;
    while(*s&&argc<16){
        while(*s==' ')s++;
        if(!*s)break;
        argv[argc++]=s;
        while(*s&&*s!=' ')s++;
        if(*s)*s++=0;
    }
    if(argc==0)return;
    for(const command_t*c=__start_commands;c<__stop_commands;c++)
        if(strcmp(c->name,argv[0])==0){c->func(argc,argv);return;}
    vga_write("Unknown command: ");vga_write(argv[0]);vga_putchar('\n');
}
void shell_run(void){
    for(;;){
        vga_set_color(0x0B);vga_write("litec> ");vga_set_color(0x07);
        read_line();execute(line);
    }
}
