#include <platform/sdk.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>

#include <signal.h>
#include <unistd.h>
#include <aux/logger.h>
#include <cfg/sensor_config.h>
#include "mpi_venc.h"
#include "mpi_sys.h"
#include "mpi_region.h"
#include "hi_comm_region.h"

#include "aux/fonts/monaco.h"
#include "aux/fonts/render.h"

int stop_flag = 0;

font_t *select_fnt = &monaco32;

void action_on_signal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

void print_error(int ret)
{
    if (-1 == ret)
    {
        log_error("I/O error, %d: %s", errno, strerror(errno));
    }
    else
    {
        log_error("Error %s(%d) at line %u, pos %u", cfg_proc_err_msg(ret), ret, cfg_proc_err_line_num(), cfg_proc_err_line_pos());
        if (ret == CFG_PROC_WRONG_SECTION)
        {
            log_error("Wrong section name [%s]", cfg_sensor_read_error_value());
        }
        else if (ret == CFG_PROC_KEY_BAD)
        {
            log_error("Wrong key \"%s\"", cfg_sensor_read_error_key());
        }
        else if (ret == CFG_PROC_VALUE_BAD)
        {
            log_error("Wrong value \"%s\" for %s", cfg_sensor_read_error_value(), cfg_sensor_read_error_key());
        }
    }
}

#define PIXEL_FORMAT_4444 PIXEL_FORMAT_ARGB_4444
#define PIXEL_FORMAT_1555 PIXEL_FORMAT_ARGB_1555
#define PIXEL_FORMAT_2BPP PIXEL_FORMAT_ARGB_2BPP
#define PIXEL_FORMAT_8888 PIXEL_FORMAT_ARGB_8888

void venc_jpeg_init_and_test() {
    int s32Ret = HI_MPI_VENC_CreateGroup(0); // XXX
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_VENC_CreateGroup failed with %#x!", s32Ret);
        return;
    }

    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_ATTR_JPEG_S stJpegAttr;

    stVencChnAttr.stVeAttr.enType = PT_JPEG;

    stJpegAttr.u32MaxPicWidth  = 1280;
    stJpegAttr.u32MaxPicHeight = 720;
    stJpegAttr.u32PicWidth  = 1280;
    stJpegAttr.u32PicHeight = 720;
    stJpegAttr.u32BufSize = 1280 * 720 * 2;
    stJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
    stJpegAttr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame?*/
    stJpegAttr.u32Priority = 0;/*channels precedence level*/
    memcpy(&stVencChnAttr.stVeAttr.stAttrJpeg, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));

    s32Ret = HI_MPI_VENC_CreateChn(0, &stVencChnAttr);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_CreateChn failed with %#x!", s32Ret);
        return;
    }

    {
        MPP_CHN_S stSrcChn;
        MPP_CHN_S stDestChn;
        stSrcChn.enModId = HI_ID_VPSS;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = 2; // Which channel to use for jpeg?

        stDestChn.enModId = HI_ID_GROUP;
        stDestChn.s32DevId = 0;
        stDestChn.s32ChnId = 0;

        s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        if (HI_SUCCESS != s32Ret) {
            log_error("HI_MPI_SYS_Bind VPSS to GROUP failed with %#x!", s32Ret);
            return;
        }
    }

    s32Ret = HI_MPI_VENC_RegisterChn(0, 0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_RegisterChn failed with %#x!", s32Ret);
        return;
    }

    int x = 20;
    int y = 20;
    int width = 512;
    int height = 32;


    #define START_POINT_X_OFFSET 64
    #define START_POINT_Y_OFFSET 64


    HI_U32 au32BgColor[3] = {0xffffffff,0x0000000,1};

    RGN_HANDLE RgnHandle;
    RGN_ATTR_S stRgnAttr;
    RGN_CHN_ATTR_S stChnAttr = {0};
    RgnHandle = 0;
    stRgnAttr.enType = OVERLAY_RGN;
    stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = height; // 48;
    stRgnAttr.unAttr.stOverlay.stSize.u32Width = width; // 512;
    stRgnAttr.unAttr.stOverlay.u32BgColor = au32BgColor[1];
    s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
    if(s32Ret != HI_SUCCESS) {
        log_error("HI_MPI_RGN_Create failed with %#x!", s32Ret);
        return;
    }
    s32Ret = HI_MPI_RGN_GetAttr(RgnHandle, &stRgnAttr);
    if(s32Ret != HI_SUCCESS) {
        log_error("HI_MPI_RGN_GetAttr failed with %#x!", s32Ret);
        return;
    }
    // stRgnAttr.unAttr.stOverlay.u32BgColor = 0x0000cccc;
    // s32Ret = HI_MPI_RGN_SetAttr(RgnHandle, &stRgnAttr);
    // if(s32Ret != HI_SUCCESS) {
    //     log_error("HI_MPI_RGN_SetAttr failed with %#x!", s32Ret);
    //     return s32Ret;
    // }

    MPP_CHN_S stChn;

    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = 0; // VencGrp;
    stChn.s32ChnId = 0;

    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = OVERLAY_RGN;
    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0; // 128;
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    // stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = width;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = height;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    // #ifndef __16CV300__
    // stChnAttr.unChnAttr.stOverlayChn.u16ColorLUT[0] = 0x3e0;
    // stChnAttr.unChnAttr.stOverlayChn.u16ColorLUT[1] = 0x7FFF;
    // stChnAttr.unChnAttr.stOverlayChn.enAttachDest = ATTACH_JPEG_MAIN;
    // #endif
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = x;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = y;
    stChnAttr.unChnAttr.stOverlayChn.u32Layer = 7;

    s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
    if(HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_RGN_AttachToChn (%d) failed with %#x!", RgnHandle,s32Ret);
        return;
    }

    render_init();

    char text_test[128];

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(text_test, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    printf("text: '%s'\n", text_test);

    render_set_brush_color(PURPLE_COLOR);
    render_draw_string(0, 0, select_fnt, text_test);
    render_save_bitmap();

    BITMAP_S bmp = {0};
    get_hi_bmp(&bmp);

    s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle, &bmp);
    if (HI_SUCCESS != s32Ret) {
        printf("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
        return ; // HI_FAILURE;
    }

    render_deinit();


printf("wait for camera 3 sec...\n");
sleep(3);
printf("TAKE!\n");

    s32Ret = HI_MPI_VENC_StartRecvPic(0);
    if (HI_SUCCESS != s32Ret) {
        printf("HI_MPI_VENC_StartRecvPic failed with %#x!\n", s32Ret);
        return;
    }

    int s32VencFd = HI_MPI_VENC_GetFd(0);
    if (s32VencFd < 0)
    {
        printf("HI_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
        return;
    }

    struct timeval TimeoutVal;
    fd_set read_fds;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;

    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);

    printf("AAAAAAAAAAAAAAAAAAAA\n");
    TimeoutVal.tv_sec  = 20;
    TimeoutVal.tv_usec = 0;

    s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        printf("snap select failed!\n");
        return;
    }
    else if (0 == s32Ret)
    {
        printf("snap time out!\n");
        return;
    }
    else
    {
        if (FD_ISSET(s32VencFd, &read_fds))
        {
            s32Ret = HI_MPI_VENC_Query(0, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                printf("HI_MPI_VENC_Query failed with %#x!\n", s32Ret);
                return;
            }
            printf("stream Stat (bRegistered, u32LeftPics, u32LeftStreamBytes, u32LeftStreamFrames, u32CurPacks, u32LeftEncPics) = (%s, %u, %u, %u, %u, %u)\n", stStat.bRegistered ? "T" : "F", stStat.u32LeftPics, stStat.u32LeftStreamBytes, stStat.u32LeftStreamFrames, stStat.u32CurPacks, stStat.u32LeftEncPics);
            stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (NULL == stStream.pstPack)
            {
                printf("malloc memory failed!\n");
                return;
            }

            stStream.u32PackCount = stStat.u32CurPacks;
            int s32Ret = HI_MPI_VENC_GetStream(0, &stStream, HI_FALSE);
            if (HI_SUCCESS != s32Ret)
            {
                printf("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return;
            }

            FILE* FU = fopen("snap_rgn.jpg", "wb");
            {
                VENC_PACK_S*  pstData;
                for (unsigned iii = 0; iii < stStream.u32PackCount; iii++)
                {
                    pstData = &(stStream.pstPack[iii]);
                    printf("writing %u and %u bytes\n", pstData->u32Len[0], pstData->u32Len[1]);
                    fwrite(pstData->pu8Addr[0], pstData->u32Len[0], 1, FU);
                    fwrite(pstData->pu8Addr[1], pstData->u32Len[1], 1, FU);
                }
            }
            fclose(FU);
            printf("Poluchili?\n");
            // TOTOTOTOTOTOTOTOTODODODODDODODO
            s32Ret = HI_MPI_VENC_ReleaseStream(0, &stStream);
            if (s32Ret)
            {
                printf("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return;
            }

            free(stStream.pstPack);
            stStream.pstPack = NULL;
        }
    }
    s32Ret = HI_MPI_RGN_Destroy(RgnHandle);
    if (s32Ret != HI_SUCCESS) {
        return;
    }

    s32Ret = HI_MPI_VENC_StopRecvPic(0);
    if (HI_SUCCESS != s32Ret) {
        printf("HI_MPI_VENC_StopRecvPic failed with %#x!\n", s32Ret);
        return;
    }

    s32Ret = HI_MPI_VENC_UnRegisterChn(0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_UnRegisterChn failed with %#x!", s32Ret);
        return;
    }

    s32Ret = HI_MPI_VENC_DestroyChn(0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_DestroyChn failed with %#x!", s32Ret);
        return;
    }

    s32Ret = HI_MPI_VENC_DestroyGroup(0); // XXX
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_VENC_DestroyGroup failed with %#x!", s32Ret);
        return;
    }
}


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        log_error("./test_jpeg sensor_config.ini");
        return -1;

    }

    signal(SIGINT, action_on_signal);

    struct SensorConfig sc;
    memset(&sc, 0, sizeof(struct SensorConfig));

    int ret = cfg_sensor_read(argv[1], &sc);

    if (ret < 0)
    {
        print_error(ret);
        return ret;
    }

    ret = sdk_init(&sc);
    if (ret < 0)
    {
        log_error("sdk_init() failed at %s: 0x%X", __sdk_last_call, ret);
        return -1;
    }

    ret = sdk_sensor_init(&sc);
    if (ret < 0)
    {
        log_error("sdk_sensor_init() failed: %d", ret);
        sdk_done();
        return ret;
    }

    ret = sdk_isp_init(&sc);
    if (ret < 0)
    {
        log_error("sdk_isp_init() failed: %d", ret);
        sdk_sensor_done();
        sdk_done();
        return ret;
    }

    ret = sdk_vi_init(&sc);
    
    if (!ret)
    {
        sdk_vpss_init(&sc);
        venc_jpeg_init_and_test();
        
        log_info("Now sleep for 100 s...");
        sleep(3);
        // sleep(10);
        // sleep(10);

    }

    sdk_isp_done();
    sdk_sensor_done();
    sdk_done();

    return 0;
}




