#include "image.h"
#include <stdio.h>
#include <string.h>

#define TINYDRAW_BMP_planes 1
#define TINYDRAW_BMP_compression 0
#define TINYDRAW_BMP_xpixelpermeter 0x130B /* 2835 , 72 DPI */
#define TINYDRAW_BMP_ypixelpermeter 0x130B /* 2835 , 72 DPI */

#pragma pack(push,1)
typedef struct tinydraw_bmp_fileheader_s {
    uint8_t signature[2];
    uint32_t filesize;
    uint32_t reserved;
    uint32_t fileoffset_to_pixelarray;
} tinydraw_bmp_fileheader_t;

typedef struct tinydraw_bmp_bitmapinfoheader_s {
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
} tinydraw_bmp_bitmapinfoheader_t;

typedef struct {
    tinydraw_bmp_fileheader_t fileheader;
    tinydraw_bmp_bitmapinfoheader_t bitmapinfoheader;
} tinydraw_bmp_t;
#pragma pack(pop)


void tinydraw_renderer_save_bmp(tinydraw_renderer_ctx_t* ctx, const char* filename)
{
    FILE *fp = fopen(filename, "wb");

    tinydraw_bmp_t bmp;
    memset(&bmp, 0, sizeof(tinydraw_bmp_t));

    strcpy(bmp.fileheader.signature, "BM"); // fi.bfType    = 0x4D42;

    bmp.fileheader.filesize = ctx->sz_buffer + sizeof(tinydraw_bmp_t);
    bmp.fileheader.fileoffset_to_pixelarray = sizeof(tinydraw_bmp_t);

    bmp.bitmapinfoheader.dibheadersize = sizeof(tinydraw_bmp_bitmapinfoheader_t);
    bmp.bitmapinfoheader.width = ctx->width;
    bmp.bitmapinfoheader.height = -ctx->height;
    bmp.bitmapinfoheader.planes = TINYDRAW_BMP_planes;
    bmp.bitmapinfoheader.bitsperpixel = ctx->bits_per_px;
    bmp.bitmapinfoheader.compression = TINYDRAW_BMP_compression;
    bmp.bitmapinfoheader.imagesize = ctx->sz_buffer;
    bmp.bitmapinfoheader.ypixelpermeter = TINYDRAW_BMP_ypixelpermeter;
    bmp.bitmapinfoheader.xpixelpermeter = TINYDRAW_BMP_xpixelpermeter;
    bmp.bitmapinfoheader.numcolorspallette = 0;

    fwrite(&bmp, 1, sizeof(tinydraw_bmp_t), fp);
    fwrite(ctx->buffer, 1, ctx->sz_buffer, fp);

    fclose(fp);
}

