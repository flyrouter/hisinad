#ifndef __hitiny_mpi_h__
#define __hitiny_mpi_h__

#include "hi_comm_vi.h"

void hitiny_MPI_VI_Init();
void hitiny_MPI_VI_Done();

int hitiny_MPI_VI_GetFd(VI_CHN ViChn);

int hitiny_MPI_VI_GetFrame(VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo);
int hitiny_MPI_VI_GetFrameTimeOut(VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo, unsigned u32MilliSec);
int hitiny_MPI_VI_ReleaseFrame(VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo);
int hitiny_MPI_VI_SetFrameDepth(VI_CHN ViChn, unsigned u32Depth);
int hitiny_MPI_VI_GetFrameDepth(VI_CHN ViChn, unsigned *pu32Depth);

#endif // __hitiny_mpi_h__

