#ifndef __hitiny_vda_h__
#define __hitiny_vda_h__

#include "hi_comm_vda.h"

void hitiny_vda_init();
void hitiny_vda_done();

int hitiny_MPI_VDA_CreateChn(VDA_CHN VdaChn, const VDA_CHN_ATTR_S *pstAttr);
int hitiny_MPI_VDA_DestroyChn(VDA_CHN VdaChn);

int hitiny_MPI_VDA_GetFd(unsigned VdaChn);

int hitiny_MPI_VDA_StartRecvPic(VDA_CHN VdaChn);
int hitiny_MPI_VDA_StopRecvPic(VDA_CHN VdaChn);

int hitiny_MPI_VDA_GetData(VDA_CHN VdaChn, VDA_DATA_S *pstVdaData, HI_BOOL bBlock);
int hitiny_MPI_VDA_ReleaseData(VDA_CHN VdaChn, const VDA_DATA_S* pstVdaData);


#endif //__hitiny_vda_h__

