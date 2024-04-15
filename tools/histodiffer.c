#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <hitiny/hitiny_vi.h>
#include <hitiny/hitiny_sys.h>
#include <hitiny/hitiny_ive.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include <stdint.h>
#include <sys/mman.h>

#define MAX_FRM_CNT     256
#define MAX_FRM_WIDTH   4096

#define IVE_HIST_SZ 1024

unsigned cur_hist_buf = 0;
IVE_MEM_INFO_S stHist[2];
void *pVirHist[2] = { 0 };
unsigned NORMA;

unsigned DIFF()
{
    unsigned* b0 = (unsigned*)(pVirHist[0]);
    unsigned* b1 = (unsigned*)(pVirHist[1]);

    unsigned sum = 0;

    for (unsigned i = 0; i < 256; ++i)
    {
        int delta = b0[i] - b1[i];
        if (delta < 0) delta = -delta;

        sum += delta * i;
    }

    return sum / NORMA;
}

void hist_buf_init(const IVE_SRC_INFO_S* src)
{
    int s32Ret = hitiny_MPI_SYS_MmzAlloc(&(stHist[cur_hist_buf].u32PhyAddr), &(pVirHist[cur_hist_buf]), "tester", HI_NULL, IVE_HIST_SZ);
    if (s32Ret != HI_SUCCESS)
    {
        fprintf(stderr, "alloc hist mem error: 0x%x\n", s32Ret);
        exit(-1);
    }

    stHist[cur_hist_buf].u32Stride = src->u32Width;
    NORMA = src->u32Width * src->u32Height;
}

int do_hist(IVE_SRC_INFO_S* src)
{
    if (!pVirHist[cur_hist_buf]) hist_buf_init(src);

    HI_BOOL bFinished;
    IVE_HANDLE IveHandleHIST;
    int s32Ret = hitiny_MPI_IVE_HIST(&IveHandleHIST, src, &(stHist[cur_hist_buf]), HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        fprintf(stderr, "ive hist create err 0x%x\n", s32Ret);
        exit(-1);
    }

    printf("now QUERY hist\n");
    s32Ret = hitiny_MPI_IVE_Query(IveHandleHIST, &bFinished, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        fprintf(stderr, "ive query (hist) err 0x%x\n", s32Ret);
        exit(-1);
    }

    if (!bFinished) return -1;

    return 0;
}

void hist_buf_done()
{
    hitiny_MPI_SYS_MmzFree(stHist[0].u32PhyAddr, pVirHist[0]);
    pVirHist[0] = 0;
    hitiny_MPI_SYS_MmzFree(stHist[1].u32PhyAddr, pVirHist[1]);
    pVirHist[1] = 0;
}

HI_S32 do_process_vi(VI_CHN ViChn)
{
    usleep(5000);

    VIDEO_FRAME_INFO_S stFrame;
    int err = hitiny_MPI_VI_GetFrame(ViChn, &stFrame);

    if (err)
    {
        printf("hitiny_MPI_VI_GetFrame err, vi chn %d: %08X \n", ViChn, err);
        return -1;
    }


    IVE_SRC_INFO_S srcmem;
    srcmem.enSrcFmt = IVE_SRC_FMT_SINGLE;
    srcmem.stSrcMem.u32PhyAddr = stFrame.stVFrame.u32PhyAddr[0];
    srcmem.stSrcMem.u32Stride = stFrame.stVFrame.u32Stride[0];
    srcmem.u32Width = stFrame.stVFrame.u32Width;
    srcmem.u32Height = stFrame.stVFrame.u32Height;
    do_hist(&srcmem);

    hitiny_MPI_VI_ReleaseFrame(ViChn, &stFrame);

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

    if (hitiny_MPI_VI_SetFrameDepth(ViChn, 1))
    {
        printf("hitiny_MPI_VI_SetFrameDepth err, vi chn %d \n", ViChn);
        return -1;
    }

    hitiny_MPI_VI_Init();

    do_process_vi(ViChn);
    while (1)
    {
        cur_hist_buf = (cur_hist_buf + 1) % 2;
        do_process_vi(ViChn);
        unsigned diff = DIFF();
        printf("DIFF: %u\n", diff);
        usleep(1000000);
    }

    hitiny_MPI_VI_Done();

	return 0;
}



