#ifndef __TINYDRAW_FONT_H__
#define __TINYDRAW_FONT_H__

#include <stdint.h>
#include <stdbool.h>
#include "image.h"

#define FONT_MARGIN_DATABIT_SIZE        6

typedef uint32_t utf8_char_t;

typedef struct
{
    uint8_t margin_top      :FONT_MARGIN_DATABIT_SIZE;
    uint8_t margin_bottom   :FONT_MARGIN_DATABIT_SIZE;
    uint8_t margin_left     :FONT_MARGIN_DATABIT_SIZE;
    uint8_t margin_right    :FONT_MARGIN_DATABIT_SIZE;

    uint16_t bmp_index;

} tinydraw_font_symbol_t;

typedef bool (*fnt_lookup_fp)(utf8_char_t c, tinydraw_font_symbol_t *sym);

typedef struct
{
    uint8_t width;
    uint8_t height;

    const uint8_t *bmp_base;

    fnt_lookup_fp lookup;
} tinydraw_font_t;

void tinydraw_renderer_draw_char(tinydraw_renderer_ctx_t *ctx, uint16_t x, uint16_t y, const tinydraw_font_t *fnt, utf8_char_t c);
void tinydraw_renderer_draw_string(tinydraw_renderer_ctx_t *ctx, uint16_t x, uint16_t y, const tinydraw_font_t *fnt, const char* s);

#endif // __TINYDRAW_FONT_H__

