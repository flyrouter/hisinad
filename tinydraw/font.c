#include "font.h"
#include <stdio.h>

void tinydraw_renderer_draw_char(tinydraw_renderer_ctx_t *ctx, uint16_t x, uint16_t y, const tinydraw_font_t *fnt, utf8_char_t c)
{
    tinydraw_font_symbol_t sym;

    if (!fnt->lookup(c, &sym))
    {
        if (!fnt->lookup('?', &sym)) return;
    }

    uint8_t font_width = fnt->width;
    uint8_t font_height = fnt->height;

    uint8_t top = sym.margin_top;
    uint8_t bottom = font_height - sym.margin_bottom-1;
    uint8_t left = sym.margin_left;
    uint8_t right = font_width - sym.margin_right-1;

    uint16_t bi = sym.bmp_index;

    uint8_t i = 0;
    for (unsigned h = 0; (h < font_height) && (ctx->height > y + h); h++)
    {
        //unsigned pos_start = 2 * ctx->width * (y + h);
        uint16_t *pos_start = (uint16_t*)ctx->buffer + ctx->width * (y + h) + x;

        for (unsigned w = 0; w < font_width; w++)
        {
            uint8_t color_pixel = 0;
            //if (w < left || w > right || h < top || h > bottom)
            if (w >= left && w <= right && h >= top && h <= bottom)
            {
                color_pixel = (fnt->bmp_base[bi] >> i) & 0x03;

                if (i >= 6) { i = 0; ++bi; } else { i += 2; }
            }

            if (ctx->width > x + w) pos_start[w] = ctx->font_draw_colors[color_pixel];
        }
    }
}

void tinydraw_renderer_draw_string(tinydraw_renderer_ctx_t *ctx, uint16_t x, uint16_t y, const tinydraw_font_t *fnt, const char* s)
{

}

