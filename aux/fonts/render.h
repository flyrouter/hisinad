#ifndef _RENDER_H_
#define _RENDER_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include "font.h"
#include "hi_comm_video.h"

//=========================================================================

#define BYTES_PER_PIXEL 3
#define BITS_PER_PIXEL BYTES_PER_PIXEL * 8

#define BLACK_COLOR             0x0000
#define WHITE_COLOR             0xFFFF
#define RED_COLOR               0xF800
#define GREEN_COLOR             0x07E0
#define BLUE_COLOR              0x001F
#define CYAN_COLOR              0x07FF
#define YELLOW_COLOR            0xFFE0
#define PURPLE_COLOR            0xF81F
#define LIGHT_GREY_COLOR        0xAD55
#define DARK_GREY_COLOR         0x52AA

#define DEFALT_BG_COLOR BLACK_COLOR
#define DEFALT_BRUSH_COLOR WHITE_COLOR

#define WIDTH                   1024
#define HEIGHT                  48

#define RGB565_TO_R8(color)         (((color) & 0xF800) >> 8)
#define RGB565_TO_G8(color)         (((color) & 0x07E0) >> 3)
#define RGB565_TO_B8(color)         (((color) & 0x001F) << 3)

#define RGB888_TO_RGB565(r,g,b)     (((((color_t)r) & 0b11111000) << 8) | ((((color_t)g) & 0b11111100) << 3) | (((color_t)b) >> 3))

//=========================================================================

#ifndef MIN
    #define MIN(a,b)(a<b?a:b)
#endif

#ifndef MAX
    #define MAX(a,b)(a>b?a:b)
#endif

//=========================================================================

typedef uint16_t color_t;

typedef struct {
    uint32_t x;
    uint32_t y;
} point_t;

typedef struct {
    uint32_t x0;
    uint32_t y0;
    uint32_t x1;
    uint32_t y1;
} rect_t;

#pragma pack(push,1)
typedef struct{
    uint8_t signature[2];
    uint32_t filesize;
    uint32_t reserved;
    uint32_t fileoffset_to_pixelarray;
} fileheader;
typedef struct{
    uint32_t dibheadersize;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitsperpixel;
    uint32_t compression;
    uint32_t imagesize;
    uint32_t ypixelpermeter;
    uint32_t xpixelpermeter;
    uint32_t numcolorspallette;
    uint32_t mostimpcolor;
} bitmapinfoheader;
typedef struct {
    fileheader fileheader;
    bitmapinfoheader bitmapinfoheader;
} bitmap;
#pragma pack(pop)

#define _planes 1
#define _compression 0
#define _pixelbytesize HEIGHT * WIDTH * BYTES_PER_PIXEL
#define _filesize _pixelbytesize + sizeof(bitmap)
#define _xpixelpermeter 0x130B //2835 , 72 DPI
#define _ypixelpermeter 0x130B //2835 , 72 DPI
#define pixel_black 0x00
#define pixel_white 0xFF

//=========================================================================
void get_hi_bmp(BITMAP_S *bmp);
void render_init(void);
void render_deinit(void);
void render_save_bitmap(void);
void render_clear_screen(void);
void render_draw_pixel(uint16_t x, uint16_t y, color_t color);
void render_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, color_t color);
void render_set_bound(uint16_t startx, uint16_t starty, uint16_t endx, uint16_t endy);
void render_write_gram(color_t color);
void render_draw_char(uint16_t x, uint16_t y, const font_t *fnt, utf8_t c);
void render_draw_string(uint16_t x, uint16_t y, const font_t *fnt, const char *s);
void render_set_back_color(color_t color);
void render_set_brush_color(color_t color);

#endif
