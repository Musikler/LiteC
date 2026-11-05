#ifndef FB_H
#define FB_H
#include "bootinfo.h"
int  fb_init(void);
int  fb_available(void);
void fb_put_pixel(int x, int y, unsigned int rgb);
void fb_fill_rect(int x, int y, int w, int h, unsigned int rgb);
void fb_clear(unsigned int rgb);
int  fb_width(void);
int  fb_height(void);
#endif