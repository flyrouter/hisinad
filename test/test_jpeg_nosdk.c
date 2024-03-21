#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <unistd.h>
#include <aux/logger.h>
#include <cfg/sensor_config.h>
#include <hitiny/hitiny_venc.h>
#include <hitiny/hitiny_sys.h>

#include <sys/time.h>

long long prev_milliseconds = 0;

void print_time(const char * s)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = tv.tv_sec * 1000000LL + tv.tv_usec;
    printf("TIME: %lld (diff %lld) %s\n", milliseconds, milliseconds - prev_milliseconds, s);
    prev_milliseconds = milliseconds;
}

void venc_jpeg_init_and_test()
{
    print_time("START");
    hitiny_MPI_VENC_DestroyGroup(0); // XXX
    int s32Ret = hitiny_MPI_VENC_CreateGroup(0); // XXX
    print_time("AFTER hitiny_MPI_VENC_CreateGroup");
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

    print_time("BEFORE hitiny_MPI_VENC_CreateChn");
    s32Ret = hitiny_MPI_VENC_CreateChn(0, &stVencChnAttr);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_CreateChn failed with %#x!", s32Ret);
        return;
    }

    s32Ret = hitiny_sys_bind_VPSS_GROUP(0, 0, 0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_SYS_Bind VPSS to GROUP failed with %#x!", s32Ret);
        return;
    }

    s32Ret = hitiny_MPI_VENC_RegisterChn(0, 0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_RegisterChn failed with %#x!", s32Ret);
        return;
    }
    print_time("AFTER HI_MPI_VENC_RegisterChn");

printf("wait for camera 3 sec...\n");
sleep(3);
printf("TAKE!\n");

    s32Ret = hitiny_MPI_VENC_SetColor2GreyConf(
        &(GROUP_COLOR2GREY_CONF_S){ .bEnable = HI_TRUE, .u32MaxWidth = 1280, .u32MaxHeight = 720 }
    );
    if (HI_SUCCESS != s32Ret) {
        log_error("%d: HI_MPI_VENC_SetColor2GreyConf failed with %#x!", __LINE__, s32Ret);
        return;
    }

    s32Ret = hitiny_MPI_VENC_SetGrpColor2Grey(
        0, // grp id
        &(GROUP_COLOR2GREY_S){ .bColor2Grey = HI_TRUE }
    );
    if (HI_SUCCESS != s32Ret) {
        log_error("%d: 0 HI_MPI_VENC_SetGrpColor2Grey failed with %#x!", __LINE__, s32Ret);
        return;
    }
    sleep(10);

    s32Ret = hitiny_MPI_VENC_StartRecvPic(0);
    if (HI_SUCCESS != s32Ret) {
        printf("HI_MPI_VENC_StartRecvPic failed with %#x!\n", s32Ret);
        return;
    }

    int s32VencFd = hitiny_MPI_VENC_GetFd(0);
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
    TimeoutVal.tv_sec  = 10;
    TimeoutVal.tv_usec = 0;

unsigned CNT = 2;
while (CNT--)
{
    s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
    printf("snap %s (%u)\n", s32Ret > 0 ? "GOOD" : "FAILED", CNT);
    if (s32Ret > 0)
    {
        s32Ret = hitiny_MPI_VENC_Query(0, &stStat);
        stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
        stStream.u32PackCount = stStat.u32CurPacks;
        hitiny_MPI_VENC_GetStream(0, &stStream, HI_FALSE);
                VENC_PACK_S*  pstData;
                for (unsigned iii = 0; iii < stStream.u32PackCount; iii++)
                {
                    pstData = &(stStream.pstPack[iii]);
                    printf("\t skipping %u and %u bytes\n", pstData->u32Len[0], pstData->u32Len[1]);
                }

        hitiny_MPI_VENC_ReleaseStream(0, &stStream);
        free(stStream.pstPack);
        stStream.pstPack = NULL;
    }
}

    s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        printf("snap select failed!\n");
    }
    else if (0 == s32Ret)
    {
        printf("snap time out!\n");
    }
    else
    {
        if (FD_ISSET(s32VencFd, &read_fds))
        {
            s32Ret = hitiny_MPI_VENC_Query(0, &stStat);
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
            int s32Ret = hitiny_MPI_VENC_GetStream(0, &stStream, HI_FALSE);
            if (HI_SUCCESS != s32Ret)
            {
                printf("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return;
            }

            FILE* FU = fopen("snap.jpg", "wb");
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
            s32Ret = hitiny_MPI_VENC_ReleaseStream(0, &stStream);
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

    s32Ret = hitiny_MPI_VENC_SetGrpColor2Grey(
        0, // grp id
        &(GROUP_COLOR2GREY_S){ .bColor2Grey = HI_FALSE }
    );
    if (HI_SUCCESS != s32Ret) {
        log_error("%d: 0 HI_MPI_VENC_SetGrpColor2Grey failed with %#x!", __LINE__, s32Ret);
        return;
    }

    s32Ret = hitiny_MPI_VENC_StopRecvPic(0);
    if (HI_SUCCESS != s32Ret) {
        printf("HI_MPI_VENC_StopRecvPic failed with %#x!\n", s32Ret);
        return;
    }

    s32Ret = hitiny_sys_unbind_VPSS_GROUP(0, 0, 0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_SYS_Bind VPSS to GROUP failed with %#x!", s32Ret);
        return;
    }

    s32Ret = hitiny_MPI_VENC_UnRegisterChn(0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_UnRegisterChn failed with %#x!", s32Ret);
        return;
    }

    s32Ret = hitiny_MPI_VENC_DestroyChn(0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_DestroyChn failed with %#x!", s32Ret);
        return;
    }

    s32Ret = hitiny_MPI_VENC_DestroyGroup(0); // XXX
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_VENC_DestroyGroup failed with %#x!", s32Ret);
        return;
    }
}


int main(int argc, char** argv)
{
    hitiny_MPI_VENC_Init();

    venc_jpeg_init_and_test();

    hitiny_MPI_VENC_Done();
        
    return 0;
}




