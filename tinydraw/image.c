#include "image.h"
#include <string.h>
#include <stdlib.h>

#define JUNK_CTXBBP 2

void tinydraw_renderer_ctx_init(tinydraw_renderer_ctx_t* ctx, uint32_t width, uint32_t height, uint8_t _bits_per_px)
{
    memset(ctx, 0, sizeof(tinydraw_renderer_ctx_t));

    ctx->width = width;
    ctx->height = height;

    ctx->bits_per_px = _bits_per_px;
    ctx->sz_buffer = height * width * JUNK_CTXBBP;
    ctx->buffer = (uint8_t*)malloc(ctx->sz_buffer);

    ctx->color_bg = DEFALT_BG_COLOR;
    ctx->color_brush = DEFALT_BRUSH_COLOR;

    tinydraw_renderer_clear_screen(ctx);
}

void tinydraw_renderer_ctx_done(tinydraw_renderer_ctx_t* ctx)
{
    free(ctx->buffer);
    ctx->buffer = 0;
}

void tinydraw_renderer_clear_screen(tinydraw_renderer_ctx_t* ctx)
{
    unsigned total_px_cnt = ctx->width * ctx->height * JUNK_CTXBBP;

    for (unsigned i = 0; i < total_px_cnt; i += JUNK_CTXBBP)
    {
        *(uint16_t*)(&ctx->buffer[i]) = ctx->color_bg;
    }
}

void tinydraw_renderer_draw_pixel(tinydraw_renderer_ctx_t* ctx, uint16_t x, uint16_t y, tinydraw_color_t color)
{
    if (x >= ctx->width) return;
    if (y >= ctx->height) return;

    unsigned ptr = 2 * (y * ctx->width + x);
    *(uint16_t*)(&ctx->buffer[ptr]) = color;
}


