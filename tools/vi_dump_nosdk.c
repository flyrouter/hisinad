#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <hitiny/hitiny_vi.h>
#include <hitiny/hitiny_sys.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include <stdint.h>
#include <sys/mman.h>

#define MAX_FRM_CNT     256
#define MAX_FRM_WIDTH   4096

void vi_dump_save_one_frame(VIDEO_FRAME_S * pVBuf, FILE *pfd_y, FILE *pfd_uv)
{
    unsigned int w, h;
    char * pVBufVirt_Y;
    char * pVBufVirt_C;
    char * pMemContent;
    unsigned char TmpBuff[MAX_FRM_WIDTH];
    HI_U32 phy_addr,size;
	HI_CHAR *pUserPageAddr[2];
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;

    if (pVBuf->u32Width > MAX_FRM_WIDTH)
    {
        printf("Over max frame width: %d, can't support.\n", MAX_FRM_WIDTH);
        return;
    }

    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
        //size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*3/2;
        u32UvHeight = pVBuf->u32Height/2;
    }
    else
    {
        //size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*2;
        u32UvHeight = pVBuf->u32Height;
    }
    size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)+(pVBuf->u32Stride[1])*u32UvHeight;

    phy_addr = pVBuf->u32PhyAddr[0];

    printf("phy_addr:%x, size:%d\n", phy_addr, size);
    pUserPageAddr[0] = (HI_CHAR *) hitiny_MPI_SYS_Mmap(phy_addr, size);
    if (NULL == pUserPageAddr[0])
    {
        printf("ERR: SYS_Mmap %d\n", errno);
        return;
    }
    printf("stride: %d,%d\n",pVBuf->u32Stride[0],pVBuf->u32Stride[1] );

	pVBufVirt_Y = pUserPageAddr[0];
	pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0])*(pVBuf->u32Height);

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);
    for(h=0; h<pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h*pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd_y);
    }
    fflush(pfd_y);


    /* save U ----------------------------------------------------------------*/
    fprintf(stderr, "U......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        pMemContent += 1;

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd_uv);
    }
    fflush(pfd_uv);

    /* save V ----------------------------------------------------------------*/
    fprintf(stderr, "V......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd_uv);
    }
    fflush(pfd_uv);

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    hitiny_MPI_SYS_Munmap(pUserPageAddr[0], size);
}

HI_S32 do_ViDump(VI_CHN ViChn)
{
    HI_S32 i, s32Ret, j;
    VIDEO_FRAME_INFO_S stFrame;
    HI_CHAR szYuv_Y_Name[128];
    HI_CHAR szYuv_uv_Name[128];
    HI_CHAR szPixFrm[10];
    FILE *pfd_y;
    FILE *pfd_uv;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");

    if (hitiny_MPI_VI_SetFrameDepth(ViChn, 1))
    {
        printf("hitiny_MPI_VI_SetFrameDepth err, vi chn %d \n", ViChn);
        return -1;
    }

    usleep(5000);

    unsigned err = hitiny_MPI_VI_GetFrame(ViChn, &stFrame);

    if (err)
    {
        printf("hitiny_MPI_VI_GetFrame err, vi chn %d: %08X \n", ViChn, err);
        return -1;
    }

    /* make file name */
    strcpy(szPixFrm, (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == stFrame.stVFrame.enPixelFormat) ? "p420" : "p422");
    sprintf(szYuv_Y_Name, "/tmp/vi_chn_%d_%d_%d_%s.y", ViChn, stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height, szPixFrm);
    sprintf(szYuv_uv_Name, "/tmp/vi_chn_%d_%d_%d_%s.uv", ViChn, stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height, szPixFrm);
	printf("Dump YUV frame of vi chn %d  to files: %s and %s\n", ViChn, szYuv_Y_Name, szYuv_uv_Name);

    err = hitiny_MPI_VI_ReleaseFrame(ViChn, &stFrame);
    printf("release frame err=%d (0 is OK)\n", err);

    /* open file */
    pfd_y = fopen(szYuv_Y_Name, "wb");
    pfd_uv = fopen(szYuv_uv_Name, "wb");

    if (!pfd_y || !pfd_uv)
    {
        printf("Can't open file for writing\n");
        return -1;
    }

    if (hitiny_MPI_VI_GetFrame(ViChn, &stFrame))
    {
       printf("get vi chn %d frame err\n", ViChn);
       printf("only get %d frame\n", i);
    }
    else
    {
        vi_dump_save_one_frame(&(stFrame.stVFrame), pfd_y, pfd_uv);

        hitiny_MPI_VI_ReleaseFrame(ViChn, &stFrame);
    }

    fclose(pfd_y);
    fclose(pfd_uv);

	return 0;
}

HI_S32 main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s VI_CHN\n", argv[0]);
        return -1;
    }
    VI_CHN ViChn = atoi(argv[1]);

    hitiny_MPI_VI_Init();

    unsigned frame_depth;
    int ret = hitiny_MPI_VI_GetFrameDepth(ViChn, &frame_depth);
    fprintf(stderr, "hitiny_MPI_VI_GetFrameDepth return 0x%x, frame depth = %u\n", ret, frame_depth);

    ret = hitiny_MPI_VI_SetFrameDepth(ViChn, 2);
    fprintf(stderr, "hitiny_MPI_VI_SetFrameDepth return 0x%x\n", ret);

    do_ViDump(ViChn);

    hitiny_MPI_VI_Done();

	return 0;
}



