#include "fs.h"
#include "string.h"
typedef struct{char name[FS_MAX_NAME];unsigned char data[FS_MAX_SIZE];int size;int used;}file_t;
static file_t files[FS_MAX_FILES];
int fs_init(void){memset(files,0,sizeof(files));return 0;}
static file_t *find(const char*name){
    for(int i=0;i<FS_MAX_FILES;i++)if(files[i].used&&strcmp(files[i].name,name)==0)return &files[i];
    return 0;
}
int fs_create(const char*name){
    if(strlen(name)>=FS_MAX_NAME)return -1;
    if(find(name))return -2;
    for(int i=0;i<FS_MAX_FILES;i++)if(!files[i].used){
        files[i].used=1;files[i].size=0;strcpy(files[i].name,name);return 0;
    }
    return -3;
}
int fs_write(const char*name,const char*data,int len){
    file_t*f=find(name);if(!f)return -1;
    if(len>FS_MAX_SIZE)len=FS_MAX_SIZE;
    memcpy(f->data,data,len);f->size=len;return len;
}
int fs_read(const char*name,char*buf,int maxlen){
    file_t*f=find(name);if(!f)return -1;
    int n=f->size<maxlen?f->size:maxlen;memcpy(buf,f->data,n);return n;
}
int fs_delete(const char*name){file_t*f=find(name);if(!f)return -1;f->used=0;return 0;}
int fs_list(void(*cb)(const char*,int)){
    int c=0;
    for(int i=0;i<FS_MAX_FILES;i++)if(files[i].used){cb(files[i].name,files[i].size);c++;}
    return c;
}
