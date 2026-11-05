#ifndef GFX_H
#define GFX_H
unsigned int gfx_blend(unsigned int dst, unsigned int src, int alpha);
void gfx_fill_rect_alpha(int x, int y, int w, int h, unsigned int rgb, int a);
void gfx_gradient_v(int x, int y, int w, int h,
                    unsigned int top, unsigned int bot);
void gfx_rounded_rect(int x, int y, int w, int h, int radius,
                      unsigned int rgb, int alpha);
void gfx_text(int x, int y, const char *s, unsigned int rgb);
void gfx_text_bg(int x, int y, const char *s, unsigned int fg, unsigned int bg);
int  gfx_text_w(const char *s);
#endif