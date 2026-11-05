#ifndef FS_H
#define FS_H
#define FS_MAX_FILES 16
#define FS_MAX_NAME  32
#define FS_MAX_SIZE  4096
int fs_init(void);
int fs_create(const char *name);
int fs_write(const char *name, const char *data, int len);
int fs_read(const char *name, char *buf, int maxlen);
int fs_delete(const char *name);
int fs_list(void (*cb)(const char *name, int size));
#endif
