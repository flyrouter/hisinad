#ifndef __HITINY_IVE_H__
#define __HITINY_IVE_H__

#include "hi_comm_ive.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

int hitiny_ive_fd();


int hitiny_MPI_IVE_HIST(IVE_HANDLE *pIveHandle, IVE_SRC_INFO_S *pstSrc, IVE_MEM_INFO_S *pstDst, HI_BOOL bInstant);
int hitiny_MPI_IVE_FILTER(IVE_HANDLE *pIveHandle, IVE_SRC_INFO_S *pstSrc, IVE_MEM_INFO_S *pstDst, IVE_FILTER_CTRL_S *pstFilterCtrl,HI_BOOL bInstant);

int hitiny_MPI_IVE_Query(IVE_HANDLE IveHandle, HI_BOOL *pbFinish, HI_BOOL bBlock);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif // __HITINY_IVE_H__

