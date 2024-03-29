#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "mpi_sys.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "hi_comm_vi.h"
#include "mpi_vi.h"
#include "mpi_ive.h"

#include <stdint.h>
#include <sys/mman.h>

#define MAX_FRM_CNT     256
#define MAX_FRM_WIDTH   4096

#define SAMPLE_IVE_HIST_SIZE 1024


int fdMemDev = -1;

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
void vi_dump_save_one_frame(VIDEO_FRAME_S * pVBuf, FILE *pfd_y, FILE *pfd_uv, FILE *pfd_erode)
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

// IVE HIST
    IVE_MEM_INFO_S stHist;
    void *pVirHist;

    int s32Ret = HI_MPI_SYS_MmzAlloc(&stHist.u32PhyAddr, &pVirHist, "User", HI_NULL, SAMPLE_IVE_HIST_SIZE);
    if (s32Ret != HI_SUCCESS)
    {
        printf("alloc hist mem error\n");
        return;
    }
    memset(pVirHist, 0, SAMPLE_IVE_HIST_SIZE);
    int* hist_buf = (unsigned*)pVirHist;
    hist_buf[0] = 18;
    hist_buf[25] = 77;
    stHist.u32Stride = 1280;

    IVE_SRC_INFO_S srcmem;
    srcmem.enSrcFmt = IVE_SRC_FMT_SINGLE;
    srcmem.stSrcMem.u32PhyAddr = pVBuf->u32PhyAddr[0];
    srcmem.stSrcMem.u32Stride = pVBuf->u32Stride[0];
    srcmem.u32Width = 1280;
    srcmem.u32Height = 960;
    printf("IVE src PhyAddr=0x%x, stride=%u\n", srcmem.stSrcMem.u32PhyAddr, srcmem.stSrcMem.u32Stride);

    HI_BOOL bFinished;
    IVE_HANDLE IveHandleHIST;
    s32Ret = HI_MPI_IVE_HIST(&IveHandleHIST, &srcmem, &stHist, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        printf("ive hist create err 0x%x\n", s32Ret);
        return;
    }

    printf("now QUERY hist\n");
    s32Ret=HI_MPI_IVE_Query(IveHandleHIST, &bFinished, HI_TRUE);

    srcmem.u32Width = 1280;
    srcmem.u32Height = 960;
    printf("now Dilate\n");

// IVE ERODE
    IVE_MEM_INFO_S stDilate;
    IVE_MEM_INFO_S stErode;
    void *pVirDilate;
    void *pVirErode;
    unsigned sz_pic_mem = 1280 * 960;

    s32Ret = HI_MPI_SYS_MmzAlloc(&stDilate.u32PhyAddr, &pVirDilate, "User", HI_NULL, sz_pic_mem);
    s32Ret = HI_MPI_SYS_MmzAlloc(&stErode.u32PhyAddr, &pVirErode, "User", HI_NULL, sz_pic_mem);
    if (s32Ret != HI_SUCCESS)
    {
        printf("alloc hist mem error\n");
        return;
    }
    memset(pVirErode, 0x7f, sz_pic_mem);
    stDilate.u32Stride = 1280;
    stErode.u32Stride = 1280;

    IVE_ERODE_CTRL_S stErodeCtrl;
    memset(&stErodeCtrl, 0xff, sizeof(IVE_ERODE_CTRL_S));
    stErodeCtrl.au8Mask[0] = 0;
    stErodeCtrl.au8Mask[2] = 0;
    stErodeCtrl.au8Mask[6] = 0;
    stErodeCtrl.au8Mask[8] = 0;
    IVE_DILATE_CTRL_S stDilateCtrl;
    memcpy(&stDilateCtrl, &stErodeCtrl, sizeof(IVE_DILATE_CTRL_S));

    printf("IVE DILATE dest PhyAddr=0x%x, stride=%u\n", stDilate.u32PhyAddr, stDilate.u32Stride);
    IVE_HANDLE IveHandleDILATE;
    s32Ret = HI_MPI_IVE_DILATE(&IveHandleDILATE, &srcmem, &stDilate, &stDilateCtrl, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        printf("ive erode create err 0x%x\n", s32Ret);
        return;
    }
    s32Ret=HI_MPI_IVE_Query(IveHandleDILATE, &bFinished, HI_TRUE);

    srcmem.stSrcMem.u32PhyAddr = stDilate.u32PhyAddr;
    srcmem.stSrcMem.u32Stride = stErode.u32Stride;

    printf("IVE ERODE dest PhyAddr=0x%x, stride=%u\n", stErode.u32PhyAddr, stErode.u32Stride);
    IVE_HANDLE IveHandleERODE;
    s32Ret = HI_MPI_IVE_ERODE(&IveHandleERODE, &srcmem, &stErode, &stErodeCtrl, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        printf("ive erode create err 0x%x\n", s32Ret);
        return;
    }
    s32Ret=HI_MPI_IVE_Query(IveHandleERODE, &bFinished, HI_TRUE);

    printf("ERODE finished? %s\n", bFinished ? "Y" : "N");
    fwrite(pVirErode, 1280*960, 1, pfd_erode);
    

    for (unsigned i = 0; i < 256; i += 8)
    {
        for (unsigned j = 0; j < 7; ++j)
        {
            printf("\t%d", hist_buf[i+j]);
        }
        printf("\n");
    }

    HI_MPI_SYS_MmzFree(stHist.u32PhyAddr, pVirHist);

// IVE HIST end

    xhi_MPI_SYS_Munmap(pUserPageAddr[0], size);
}

HI_S32 SAMPLE_MISC_ViDump(VI_CHN ViChn, HI_U32 u32Cnt)
{
    HI_S32 i, s32Ret, j;
    VIDEO_FRAME_INFO_S stFrame;
    VIDEO_FRAME_INFO_S astFrame[MAX_FRM_CNT];
    HI_CHAR szYuvName[128];
    HI_CHAR szPixFrm[10];
    FILE *pfd_y;
    FILE *pfd_uv;
    FILE *pfd_erode;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("usage: ./vi_dump [vichn] [frmcnt]. sample: ./vi_dump 0 5\n\n");

    if (HI_MPI_VI_SetFrameDepth(ViChn, 1))
    {
        printf("HI_MPI_VI_SetFrameDepth err, vi chn %d \n", ViChn);
        return -1;
    }

    usleep(5000);

    unsigned err = HI_MPI_VI_GetFrame(ViChn, &stFrame);

    if (err)
    {
        printf("HI_MPI_VI_GetFrame err, vi chn %d: %08X \n", ViChn, err);
        return -1;
    }

    /* make file name */
    strcpy(szPixFrm,
    (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == stFrame.stVFrame.enPixelFormat)?"p420":"p422");
    sprintf(szYuvName, "./vi_chn_%d_%d_%d_%s_%d.yuv", ViChn,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height,szPixFrm,u32Cnt);
	printf("Dump YUV frame of vi chn %d  to file: \"%s\"\n", ViChn, szYuvName);

    HI_MPI_VI_ReleaseFrame(ViChn, &stFrame);

    /* open file */
    char fname_y[256];
    char fname_uv[256];
    char fname_erode[256];
    snprintf(fname_y, 256, "%s-y", szYuvName);
    snprintf(fname_uv, 256, "%s-uv", szYuvName);
    snprintf(fname_erode, 256, "%s-erode", szYuvName);
    pfd_y = fopen(fname_y, "wb");
    pfd_uv = fopen(fname_uv, "wb");
    pfd_erode = fopen(fname_erode, "wb");

    if (NULL == pfd_y || NULL == pfd_uv)
    {
        return -1;
    }

    /* get VI frame  */
    for (i=0; i<u32Cnt; i++)
    {
        if (HI_MPI_VI_GetFrame(ViChn, &astFrame[i])<0)
        {
            printf("get vi chn %d frame err\n", ViChn);
            printf("only get %d frame\n", i);
            break;
        }
    }

    for(j=0; j<i; j++)
    {
        /* save VI frame to file */
        vi_dump_save_one_frame(&(astFrame[j].stVFrame), pfd_y, pfd_uv, pfd_erode);

        /* release frame after using */
        HI_MPI_VI_ReleaseFrame(ViChn, &astFrame[j]);

        #if 0
        if (astFrame[i].stVFrame.u32Width != stFrame.stVFrame.u32Width
            || astFrame[i].stVFrame.u32Height != stFrame.stVFrame.u32Height)
        {
            printf("vi size has changed when frame %d, break\n",i);
            break;
        }
        #endif
    }

    fclose(pfd_y);
    fclose(pfd_uv);
    fclose(pfd_erode);

	return 0;
}

HI_S32 main(int argc, char *argv[])
{
    VI_CHN ViChn = 0;
    HI_U32 u32FrmCnt = 1;

    SAMPLE_MISC_ViDump(ViChn, u32FrmCnt);

	return HI_SUCCESS;
}



