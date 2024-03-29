#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <hitiny/hitiny_vi.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include <stdint.h>
#include <sys/mman.h>

#define MAX_FRM_CNT     256
#define MAX_FRM_WIDTH   4096

int fdMemDev = -1;

//void *mmap(void *start, size_t len, int prot, int flags, int fd, uint32_t off);

void * xhi_MPI_SYS_Mmap(HI_U32 u32PhyAddr, HI_U32 u32Size)
{
    fprintf(stderr, "PhyAddr: to mmap %u (0x%x) bytes from 0x%x\n", u32Size, u32Size, u32PhyAddr);
    if (fdMemDev < 0)
    {
        fdMemDev = open("/dev/mem", 4162);
        if (fdMemDev < 0)
        {
            fprintf(stderr, "Can't open /dev/mem: (%d) %s\n", errno, strerror(errno));
            return 0;
        }

        unsigned alligned = u32PhyAddr & 0xFFFFF000;
        unsigned offset = u32PhyAddr - (u32PhyAddr & 0xFFFFF000);
        unsigned sz = ((u32Size + offset - 1) & 0xFFFFF000) + 0x1000;
        
        fprintf(stderr, "going to mmap %u (0x%x) bytes from 0x%x\n", sz, sz, alligned);
        void* ret = mmap64(0, sz, 3, 1, fdMemDev, alligned);
        if (!ret) fprintf(stderr, "Can't mmap: (%d) %s\n", errno, strerror(errno));
        return ret + offset;
    }
    return 0;
}

HI_S32 xhi_MPI_SYS_Munmap(HI_VOID* pVirAddr, HI_U32 u32Size)
{
    unsigned virtaddr = ((unsigned int)pVirAddr);
    unsigned alligned = virtaddr & 0xFFFFF000;
    unsigned offset = virtaddr - (virtaddr & 0xFFFFF000);
    unsigned sz = ((u32Size + offset - 1) & 0xFFFFF000) + 0x1000;

    int ret = munmap((void*)alligned, sz);
    //fprintf(stderr, "munmap(addr 0x%x, sz %u): ret=%d\n", alligned, sz, ret);
    return ret;
}


/* sp420 转存为 p420 ; sp422 转存为 p422  */
void vi_dump_save_one_frame(VIDEO_FRAME_S * pVBuf, FILE *pfd)
{
    unsigned int w, h;
    char * pVBufVirt_Y;
    char * pVBufVirt_C;
    char * pMemContent;
    unsigned char TmpBuff[MAX_FRM_WIDTH];                //如果这个值太小，图像很大的话存不了
    HI_U32 phy_addr,size;
	HI_CHAR *pUserPageAddr[2];
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;/* 存为planar 格式时的UV分量的高度 */

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
    pUserPageAddr[0] = (HI_CHAR *) xhi_MPI_SYS_Mmap(phy_addr, size);
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
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }
    fflush(pfd);


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
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);

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
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    xhi_MPI_SYS_Munmap(pUserPageAddr[0], size);
}

HI_S32 SAMPLE_MISC_ViDump(VI_CHN ViChn, HI_U32 u32Cnt)
{
    HI_S32 i, s32Ret, j;
    VIDEO_FRAME_INFO_S stFrame;
    VIDEO_FRAME_INFO_S astFrame[MAX_FRM_CNT];
    HI_CHAR szYuvName[128];
    HI_CHAR szPixFrm[10];
    FILE *pfd;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("usage: ./vi_dump [vichn] [frmcnt]. sample: ./vi_dump 0 5\n\n");

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
    strcpy(szPixFrm,
    (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == stFrame.stVFrame.enPixelFormat)?"p420":"p422");
    sprintf(szYuvName, "./vi_chn_%d_%d_%d_%s_%d.yuv", ViChn,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height,szPixFrm,u32Cnt);
	printf("Dump YUV frame of vi chn %d  to file: \"%s\"\n", ViChn, szYuvName);

    err = hitiny_MPI_VI_ReleaseFrame(ViChn, &stFrame);
    printf("release frame err=%d (0 is OK)\n", err);

    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (NULL == pfd)
    {
        return -1;
    }

    /* get VI frame  */
    for (i=0; i<u32Cnt; i++)
    {
        if (hitiny_MPI_VI_GetFrame(ViChn, &astFrame[i])<0)
        {
            printf("get vi chn %d frame err\n", ViChn);
            printf("only get %d frame\n", i);
            break;
        }
    }

    for(j=0; j<i; j++)
    {
        /* save VI frame to file */
        vi_dump_save_one_frame(&(astFrame[j].stVFrame), pfd);

        /* release frame after using */
        hitiny_MPI_VI_ReleaseFrame(ViChn, &astFrame[j]);

        #if 0
        if (astFrame[i].stVFrame.u32Width != stFrame.stVFrame.u32Width
            || astFrame[i].stVFrame.u32Height != stFrame.stVFrame.u32Height)
        {
            printf("vi size has changed when frame %d, break\n",i);
            break;
        }
        #endif
    }

    fclose(pfd);

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

    SAMPLE_MISC_ViDump(ViChn, 1);

    hitiny_MPI_VI_Done();

	return 0;
}



