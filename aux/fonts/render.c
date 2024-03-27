#include <string.h>
#include "render.h"
#include "utf8.h"

#define DIV_UP(x, a)             ( ((x) + ((a) - 1) ) / a )

//=========================================================================
static uint8_t *pixelbuffer = NULL;

static rect_t screen_bond;
static point_t cur;

//=========================================================================

#define ROW_CLERANCE_SIZE   5

#if (CONFIG_BPP == 1u)
static color_t brush_color[2] = {BLACK_COLOR, WHITE_COLOR};
#elif (CONFIG_BPP == 2u)
static color_t brush_color[4] = {BLACK_COLOR, DARK_GREY_COLOR, LIGHT_GREY_COLOR, WHITE_COLOR};
#elif (CONFIG_BPP == 4u)
static color_t brush_color[16] = {
    BLACK_COLOR,
    BLACK_COLOR,
    BLACK_COLOR,
    BLACK_COLOR,
    BLACK_COLOR,
    DARK_GREY_COLOR,
    DARK_GREY_COLOR,
    DARK_GREY_COLOR,
    LIGHT_GREY_COLOR,
    LIGHT_GREY_COLOR,
    LIGHT_GREY_COLOR,
    LIGHT_GREY_COLOR,
    WHITE_COLOR,
    WHITE_COLOR,
    WHITE_COLOR,
    WHITE_COLOR
};
#endif

//=========================================================================

void save_bitmap(uint8_t *pbuf) {
    FILE *fp = fopen("test.bmp", "wb");
    bitmap *pbitmap  = (bitmap*)calloc(1, sizeof(bitmap));
    strcpy(pbitmap->fileheader.signature, "BM");
    pbitmap->fileheader.filesize = _filesize;
    pbitmap->fileheader.fileoffset_to_pixelarray = sizeof(bitmap);
    pbitmap->bitmapinfoheader.dibheadersize = sizeof(bitmapinfoheader);
    pbitmap->bitmapinfoheader.width = WIDTH;
    pbitmap->bitmapinfoheader.height = -HEIGHT;
    // pbitmap->bitmapinfoheader.height = -_height;
    pbitmap->bitmapinfoheader.planes = _planes;
    pbitmap->bitmapinfoheader.bitsperpixel = BITS_PER_PIXEL;
    pbitmap->bitmapinfoheader.compression = _compression;
    pbitmap->bitmapinfoheader.imagesize = _pixelbytesize;
    pbitmap->bitmapinfoheader.ypixelpermeter = _ypixelpermeter ;
    pbitmap->bitmapinfoheader.xpixelpermeter = _xpixelpermeter ;
    pbitmap->bitmapinfoheader.numcolorspallette = 0;
    fwrite (pbitmap, 1, sizeof(bitmap), fp);

    fwrite(pbuf, 1, _pixelbytesize, fp);
    free(pbitmap);
    fclose(fp);
}

void render_save_bitmap(void) {
    save_bitmap(pixelbuffer);
}

void get_hi_bmp(BITMAP_S *bmp) {
    HI_S32 s32BytesPerPix = 2;
    // bmp->enPixelFormat = PIXEL_FORMAT_RGB_555;
    // bmp->enPixelFormat = PIXEL_FORMAT_RGB_2BPP; // PIXEL_FORMAT_2BPP;
    bmp->enPixelFormat = PIXEL_FORMAT_RGB_1555;
    bmp->u32Width = WIDTH;
    bmp->u32Height = HEIGHT;

    // HI_U16* pu16Data = malloc((WIDTH * BYTES_PER_PIXEL) * (HEIGHT));
    // HI_U16* pu16Data = malloc(((WIDTH * BYTES_PER_PIXEL) * (HEIGHT)) * sizeof(HI_U16));
    HI_U16* pu16Data = malloc(((WIDTH) * (HEIGHT))*2);
    // HI_U16* pu16Data = malloc(((WIDTH * BYTES_PER_PIXEL) * (HEIGHT)));
    printf("(WIDTH * BYTES_PER_PIXEL) * (HEIGHT): %d\n", (WIDTH * BYTES_PER_PIXEL) * (HEIGHT));
    // printf("(WIDTH * BYTES_PER_PIXEL) * (HEIGHT): %d\n", (WIDTH * BYTES_PER_PIXEL) * (HEIGHT));

    // uint32_t location = x * (BITS_PER_PIXEL / 8) + y * WIDTH * BYTES_PER_PIXEL;
    int x = 0;
    for (int i = 0; i < ((WIDTH * BYTES_PER_PIXEL) * (HEIGHT)); i=i+3) {
#if 0
        *pu16Data = pixelbuffer[i];
        *pu16Data = *pu16Data << 8;
        *pu16Data |= pixelbuffer[i+1];
        *pu16Data++;
#else
        /*
        r          g          b
        1111 1111  1111 1111  1111 1111
        1  11111  11111 11111
        1111111111111111 1111111111111111 1111111111111111
        */

        // pu16Data[x] = (red << (5+5) | (green << 5) | blue);
        pu16Data[x] = ((pixelbuffer[i] & 0xF8) << 8) | ((pixelbuffer[i+1] & 0xfc) << 3) | (pixelbuffer[i+2] >> 3);
        // *pu16Data = ((pixelbuffer[i] & 0xF8) << 8) | ((pixelbuffer[i+1] & 0xfc) << 3) | (pixelbuffer[i+2] >> 3);
        // *pu16Data++;


        x++;
#endif
    }
    free(bmp->pData);
    bmp->pData = (HI_VOID*) pu16Data;
    return;
}

//=========================================================================

void render_init() {
    pixelbuffer = (uint8_t*)malloc(_pixelbytesize);
    render_set_back_color(DEFALT_BG_COLOR);
    render_set_brush_color(DEFALT_BRUSH_COLOR);

    render_clear_screen();
}

//=========================================================================

void render_deinit() {
    free(pixelbuffer);
    // uint32_t screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    // munmap (fbp, screensize);
    // close (fp);
}
//=========================================================================

void render_clear_screen() {
    // uint16_t width = MIN(WIDTH, vinfo.xres);
    // uint16_t height = MIN(HEIGHT, vinfo.yres);
    render_fill_rect(0, 0, WIDTH, HEIGHT, DEFALT_BG_COLOR);
}

//=========================================================================

void render_draw_pixel(uint16_t x, uint16_t y, color_t color) {
    if (!pixelbuffer || (pixelbuffer == (void*)-1)) {
        printf("!pixelbuffer ||  (pixelbuffer == (void*)-1)\n");
        return;
    }

    if (x >= WIDTH || y >= HEIGHT) {
        printf("x(%d) >= LCD_WIDTH || y(%d) >= LCD_HEIGHT\n", x, y);
        return;
    }

    uint32_t location = x * (BITS_PER_PIXEL / 8) + y * WIDTH * BYTES_PER_PIXEL;

    uint8_t r = RGB565_TO_R8(color);
    uint8_t g = RGB565_TO_G8(color);
    uint8_t b = RGB565_TO_B8(color);

#if 1
    *(pixelbuffer + location) = b;        // blue
    *(pixelbuffer + location + 1) = g;    // green
    *(pixelbuffer + location + 2) = r;    // red
    *(pixelbuffer + location + 3) = 0;    // alpha
#else
    *(pixelbuffer + location) = 0;        // blue
    *(pixelbuffer + location + 1) = r;    // green
    *(pixelbuffer + location + 2) = g;    // red
    *(pixelbuffer + location + 3) = b;    // alpha
    // *(pixelbuffer + location) = r;        // blue
    // *(pixelbuffer + location + 1) = g;    // green
    // *(pixelbuffer + location + 2) = b;    // red
    // *(pixelbuffer + location + 3) = 0;    // alpha
#endif
}

//=========================================================================

void render_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, color_t color) {
    render_set_bound(x, y, x + width-1, y + height - 1);

    uint32_t area = width * height;
    while (area--) {
        render_write_gram(color);
    }
}

//=========================================================================

void render_set_bound(uint16_t startx, uint16_t starty, uint16_t endx, uint16_t endy) {
    screen_bond.x0 = startx;
    screen_bond.y0 = starty;
    screen_bond.x1 = endx;
    screen_bond.y1 = endy;

    cur.x = startx;
    cur.y = starty;
}

//=========================================================================

void render_write_gram(color_t color) {
    render_draw_pixel(cur.x, cur.y, color);

    ++cur.x;
    if (cur.x > screen_bond.x1) {
        cur.x = screen_bond.x0;
        ++cur.y;
    }

    if (cur.y > screen_bond.y1) {
        cur.x = screen_bond.x0;
        cur.y = screen_bond.y0;
    }
}

//=========================================================================

#if (CONFIG_BPP == 2u || CONFIG_BPP == 4u)
static void aa_brush_color() {
    uint8_t lr;
    uint8_t lg;
    uint8_t lb;

#if (CONFIG_BPP == 2)
    uint8_t idxmax = 2;
#elif(CONFIG_BPP == 4)
    uint8_t idxmax = 14;
#endif

    for (int i = 1; i <= idxmax; i++) {
#if (CONFIG_BPP == 2)
        lr = (((uint16_t)RGB565_TO_R8(brush_color[3])) * i + ((uint16_t)RGB565_TO_R8(brush_color[0])) * (3 - i)) / 3;
        lg = (((uint16_t)RGB565_TO_G8(brush_color[3])) * i + ((uint16_t)RGB565_TO_G8(brush_color[0])) * (3 - i)) / 3;
        lb = (((uint16_t)RGB565_TO_B8(brush_color[3])) * i + ((uint16_t)RGB565_TO_B8(brush_color[0])) * (3 - i)) / 3;
#elif(CONFIG_BPP == 4)
        lr = (((uint16_t)RGB565_TO_R8(brush_color[15])) * i + ((uint16_t)RGB565_TO_R8(brush_color[0])) * (1 5 -i)) / 15;
        lg = (((uint16_t)RGB565_TO_G8(brush_color[15])) * i + ((uint16_t)RGB565_TO_G8(brush_color[0])) * (1 5 -i)) / 15;
        lb = (((uint16_t)RGB565_TO_B8(brush_color[15])) * i + ((uint16_t)RGB565_TO_B8(brush_color[0])) * (1 5 -i)) / 15;
#endif
        brush_color[i] = RGB888_TO_RGB565(lr, lg, lb);
    }
}
#endif

//=========================================================================
#if (CONFIG_FONT_MARGIN == 0u && CONFIG_FONT_ENC == 0u)
// encoding method: raw
static void font_render_engine_raw(const font_t *fnt, const font_symbol_t *sym)
{
    uint16_t i = 0;
    uint8_t j = 0;

#if (CONFIG_FONT_FIXED_WIDTH_HEIGHT > 0u)
    uint16_t area = fnt->width * fnt->height;
#else
    uint16_t area = sym->width * sym->height;
#endif

    const uint8_t *bmp = (const uint8_t*)(fnt->bmp_base + sym->bmp_index);

    while (area--) {
#if (CONFIG_BPP == 1u)
        uint8_t color_pixel = (bmp[i] >> j) & 0x01;
        render_write_gram(brush_color[color_pixel]);

        if (j == 7) { ++i; j = 0; } else { ++j; }
#elif (CONFIG_BPP == 2u)
        uint8_t color_pixel = (bmp[i] >> j) & 0x03;
        render_write_gram(brush_color[color_pixel]);

        if (j == 6) { ++i; j = 0; } else { j+=2; }
#elif (CONFIG_BPP == 4u)
        uint8_t color_pixel = (bmp[i] >> j) & 0x0F;
        render_write_gram(brush_color[color_pixel]);

        if (j == 4) { ++i; j = 0; } else { j+=4; }
#endif
    }
}
#endif

#if (CONFIG_FONT_ENC == 1u)
// encoding method: rawbb
static void font_render_engine_rawbb(const font_t *fnt, const font_symbol_t *sym)
{
#if (CONFIG_FONT_FIXED_WIDTH_HEIGHT > 0u)
    uint8_t font_width = fnt->width;
    uint8_t font_height = fnt->height;
#else
    uint8_t font_width = sym->width;
    uint8_t font_height = sym->height;
#endif

    uint8_t top = sym->margin_top;
    uint8_t bottom = font_height - sym->margin_bottom-1;
    uint8_t left = sym->margin_left;
    uint8_t right = font_width - sym->margin_right-1;

    uint16_t bi = sym->bmp_index;

    uint8_t h, w;
    uint8_t i=0;

    for (h = 0; h < font_height; h++) {
        for (w = 0; w < font_width; w++) {
            if (w < left || w > right || h < top || h > bottom) {
                // debug: render_write_gram(BLUE_COLOR);
                render_write_gram(brush_color[0]);
            } else {
#if (CONFIG_BPP == 1u)
                uint8_t color_pixel = (fnt->bmp_base[bi] >> i) & 0x01;
                render_write_gram(brush_color[color_pixel]);

                if (i == 7) { i = 0; ++bi; } else { ++i; }
#elif (CONFIG_BPP == 2u)
                uint8_t color_pixel = (fnt->bmp_base[bi] >> i) & 0x03;
                render_write_gram(brush_color[color_pixel]);

                if (i == 6) { i = 0; ++bi; } else { i+=2; }
#elif (CONFIG_BPP == 4u)
                uint8_t color_pixel = (fnt->bmp_base[bi] >> i) & 0x0F;
                render_write_gram(brush_color[color_pixel]);

                if (i == 4) { i = 0; ++bi; } else { i+=4; }
#endif
            }
        }
    }
}
#endif

#if (CONFIG_FONT_ENC == 3u)

typedef enum {
    RLE_STATE_SINGLE = 0,
    RLE_STATE_REPEATE,
    RLE_STATE_COUNTER,
} rle_state_t;

static uint32_t rle_rdp;
static const uint8_t * rle_in;
static uint8_t rle_bpp;
static uint8_t rle_prev_v;
static uint8_t rle_cnt;
static rle_state_t rle_state;

static inline void rle_init(const uint8_t * in,  uint8_t bpp);
static inline uint8_t rle_next(void);

static void font_render_engine_lvgl(const font_t *fnt, const font_symbol_t *sym)
{
#if (CONFIG_FONT_FIXED_WIDTH_HEIGHT > 0u)
    uint8_t font_width = fnt->width;
    uint8_t font_height = fnt->height;
#else
    uint8_t font_width = sym->width;
    uint8_t font_height = sym->height;
#endif

    uint8_t top = sym->margin_top;
    uint8_t bottom = font_height - sym->margin_bottom-1;
    uint8_t left = sym->margin_left;
    uint8_t right = font_width - sym->margin_right-1;

    uint16_t bi = sym->bmp_index;

    rle_init(&fnt->bmp_base[bi], CONFIG_BPP);

    for (int h = 0; h < font_height; h++) {
        for (int w = 0; w < font_width; w++) {
            if (w < left || w > right || h < top || h > bottom) {
                // debug: render_write_gram(BLUE_COLOR);
                render_write_gram(brush_color[0]);
            } else {
#if (CONFIG_BPP == 1u)
                uint8_t b = rle_next() & 0x01;
                render_write_gram(brush_color[b]);
#elif (CONFIG_BPP == 2u)
                uint8_t b = rle_next() & 0x03;
                render_write_gram(brush_color[b]);
#elif (CONFIG_BPP == 4u)
                uint8_t b = rle_next() & 0x0F;
                render_write_gram(brush_color[b]);
#endif
            }
        }
    }
}

/**
 * Read bits from an input buffer. The read can cross byte boundary.
 * @param in the input buffer to read from.
 * @param bit_pos index of the first bit to read.
 * @param len number of bits to read (must be <= 8).
 * @return the read bits
 */
static inline uint8_t get_bits(const uint8_t * in, uint32_t bit_pos, uint8_t len)
{
    uint8_t bit_mask;
    switch(len) {
        case 1:
            bit_mask = 0x1;
            break;
        case 2:
            bit_mask = 0x3;
            break;
        case 3:
            bit_mask = 0x7;
            break;
        case 4:
            bit_mask = 0xF;
            break;
        case 8:
            bit_mask = 0xFF;
            break;
        default:
            bit_mask = (uint16_t)((uint16_t) 1 << len) - 1;
    }

    uint32_t byte_pos = bit_pos >> 3;
    bit_pos = bit_pos & 0x7;

    if (bit_pos + len >= 8) {
        uint16_t in16 = (in[byte_pos] << 8) + in[byte_pos + 1];
        return (in16 >> (16 - bit_pos - len)) & bit_mask;
    } else {
        return (in[byte_pos] >> (8 - bit_pos - len)) & bit_mask;
    }
}

static inline void rle_init(const uint8_t * in,  uint8_t bpp) {
    rle_in = in;
    rle_bpp = bpp;
    rle_state = RLE_STATE_SINGLE;
    rle_rdp = 0;
    rle_prev_v = 0;
    rle_cnt = 0;
}

static inline uint8_t rle_next(void) {
    uint8_t v = 0;
    uint8_t ret = 0;

    if (rle_state == RLE_STATE_SINGLE) {
        ret = get_bits(rle_in, rle_rdp, rle_bpp);
        if (rle_rdp != 0 && rle_prev_v == ret) {
            rle_cnt = 0;
            rle_state = RLE_STATE_REPEATE;
        }

        rle_prev_v = ret;
        rle_rdp += rle_bpp;
    } else if (rle_state == RLE_STATE_REPEATE) {
        v = get_bits(rle_in, rle_rdp, 1);
        rle_cnt++;
        rle_rdp += 1;
        if (v == 1) {
            ret = rle_prev_v;
            if (rle_cnt == 11) {
                rle_cnt = get_bits(rle_in, rle_rdp, 6);
                rle_rdp += 6;
                if (rle_cnt != 0) {
                    rle_state = RLE_STATE_COUNTER;
                } else {
                    ret = get_bits(rle_in, rle_rdp, rle_bpp);
                    rle_prev_v = ret;
                    rle_rdp += rle_bpp;
                    rle_state = RLE_STATE_SINGLE;
                }
            }
        } else {
            ret = get_bits(rle_in, rle_rdp, rle_bpp);
            rle_prev_v = ret;
            rle_rdp += rle_bpp;
            rle_state = RLE_STATE_SINGLE;
        }

    } else if (rle_state == RLE_STATE_COUNTER) {
        ret = rle_prev_v;
        rle_cnt--;
        if (rle_cnt == 0) {
            ret = get_bits(rle_in, rle_rdp, rle_bpp);
            rle_prev_v = ret;
            rle_rdp += rle_bpp;
            rle_state = RLE_STATE_SINGLE;
        }
    }

    return ret;
}
#endif

//=========================================================================

void draw_char(uint16_t x, uint16_t y, const font_t *fnt, utf8_t c) {
    font_symbol_t sym;
    if (!fnt->lookup(c, &sym)) { return; }

#if (CONFIG_FONT_FIXED_WIDTH_HEIGHT > 0u)
    uint8_t font_width = fnt->width;
    uint8_t font_height = fnt->height;
#else
    uint8_t font_width = sym.width;
    uint8_t font_height = sym.height;
#endif

    render_set_bound(x, y, x + font_width - 1, y + font_height - 1);

#if (CONFIG_FONT_ENC == 0u)
    font_render_engine_raw(fnt, &sym);
#elif(CONFIG_FONT_ENC == 1u)
    font_render_engine_rawbb(fnt, &sym);
#elif(CONFIG_FONT_ENC == 2u)
    font_render_engine_u8g2(fnt, &sym);
#elif(CONFIG_FONT_ENC == 3u)
    font_render_engine_lvgl(fnt, &sym);
#else
    #error "Unsupported ENCODING_METHOD"
#endif

    render_set_bound(0, 0, WIDTH - 1, HEIGHT - 1);
}

//=========================================================================


void render_draw_string(uint16_t x, uint16_t y, const font_t *fnt, const char *s)
{
    uint16_t orgx = x;

    utf8_t c;
    while ((c = utf8_getchar(s)) != '\0') {
        if (c == '\r') {
            // no operation
        } else if (c == '\n') {
#if (CONFIG_FONT_FIXED_WIDTH_HEIGHT > 0u)
            y += (fnt->height + ROW_CLERANCE_SIZE);
#else
            y += (fnt->default_height + ROW_CLERANCE_SIZE);
#endif
            x = orgx;
        } else if (c == ' ') {
#if (CONFIG_FONT_FIXED_WIDTH_HEIGHT > 0u)
            x += fnt->width;
#else
            x += fnt->default_width;
#endif
        } else {
            draw_char(x, y, fnt, c);
#if (CONFIG_FONT_FIXED_WIDTH_HEIGHT > 0u)
            x += fnt->width;
#else
            font_symbol_t sym;
            if(!fnt->lookup(c, &sym)) {
                x += fnt->default_width;
            } else {
                x += sym.width;
            }
#endif
        }
        s += utf8_charlen(c);
    }
}

//=========================================================================

void render_set_back_color(color_t color) {
    brush_color[0] = color;

#if (CONFIG_BPP== 2u || CONFIG_BPP == 4u)
    aa_brush_color();
#endif
}

//=========================================================================

void render_set_brush_color(color_t color) {
    brush_color[(1 << CONFIG_BPP) - 1] = color;

#if (CONFIG_BPP== 2u || CONFIG_BPP == 4u)
    aa_brush_color();
#endif
}
