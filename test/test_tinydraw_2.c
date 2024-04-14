#include <tinydraw/image.h>
#include <tinydraw/monaco32.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    tinydraw_renderer_ctx_t td_ctx;
    tinydraw_renderer_ctx_init(&td_ctx, 1100, 240, 16);

    tinydraw_renderer_draw_string(&td_ctx, 0, 0, &tinydraw_font_monaco32, "The quick brown fox jumps over the lazy dog.");

    char buf[64];
    struct tm tmloc;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tmloc);
    snprintf(buf, 64, "%04u-%02u-%02u %02u:%02u:%02u", tmloc.tm_year + 1900, tmloc.tm_mon + 1, tmloc.tm_mday, tmloc.tm_hour, tmloc.tm_min, tmloc.tm_sec);

    tinydraw_renderer_draw_string(&td_ctx, 0, 200, &tinydraw_font_monaco32, buf);
    tinydraw_renderer_draw_string(&td_ctx, 0, 120, &tinydraw_font_monaco32, "Cozy sphinx waves quart jug of bad milk.");

    td_ctx.font_draw_colors[0] = BLACK_COLOR;
    td_ctx.font_draw_colors[1] = (RED_COLOR / 3) & RED_COLOR;
    td_ctx.font_draw_colors[2] = (RED_COLOR / 2) & RED_COLOR;
    td_ctx.font_draw_colors[3] = RED_COLOR;
    tinydraw_renderer_draw_string(&td_ctx, 0, 40, &tinydraw_font_monaco32, "Crazy Fredrick bought many very exquisite opal jewels.");

    td_ctx.font_draw_colors[0] = BLUE_COLOR;
    td_ctx.font_draw_colors[1] = (BLUE_COLOR / 2) & BLUE_COLOR;
    td_ctx.font_draw_colors[2] = (BLUE_COLOR / 4) & BLUE_COLOR;
    td_ctx.font_draw_colors[3] = BLACK_COLOR;
    tinydraw_renderer_draw_string(&td_ctx, 0, 80, &tinydraw_font_monaco32, "The jay, pig, fox, zebra and my wolves quack!");

    td_ctx.font_draw_colors[0] = DARK_GREY_COLOR;
    td_ctx.font_draw_colors[1] = WHITE_COLOR;//(YELLOW_COLOR / 4) & YELLOW_COLOR;
    td_ctx.font_draw_colors[2] = WHITE_COLOR;//(YELLOW_COLOR / 2) & YELLOW_COLOR;
    td_ctx.font_draw_colors[3] = WHITE_COLOR;//YELLOW_COLOR;
    tinydraw_renderer_draw_string(&td_ctx, 0, 160, &tinydraw_font_monaco32, "(10 + 9 + ... + 1) = (10 + 1) * (10/2) = 55");

    tinydraw_renderer_save_bmp(&td_ctx, "test.bmp");
    tinydraw_renderer_ctx_done(&td_ctx);

    return 0;
}

