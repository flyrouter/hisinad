#ifndef __hitiny_region_h__
#define __hitiny_region_h__

#include "hi_comm_region.h"

int hitiny_region_init();
void hitiny_region_done();

int hitiny_MPI_RGN_Create(RGN_HANDLE Handle,const RGN_ATTR_S *pstRegion);
int hitiny_MPI_RGN_Destroy(RGN_HANDLE Handle);

int hitiny_MPI_RGN_GetAttr(RGN_HANDLE Handle,RGN_ATTR_S *pstRegion);
int hitiny_MPI_RGN_SetAttr(RGN_HANDLE Handle,const RGN_ATTR_S *pstRegion);

int hitiny_MPI_RGN_SetBitMap(RGN_HANDLE Handle,const BITMAP_S *pstBitmap);

//HI_S32 hitiny_MPI_RGN_SetAttachField(RGN_HANDLE Handle, RGN_ATTACH_FIELD_E enAttachField);
//HI_S32 hitiny_MPI_RGN_GetAttachField(RGN_HANDLE Handle, RGN_ATTACH_FIELD_E *penAttachField);

int hitiny_MPI_RGN_AttachToChn(RGN_HANDLE Handle,const MPP_CHN_S *pstChn,const RGN_CHN_ATTR_S *pstChnAttr);
int hitiny_MPI_RGN_DetachFrmChn(RGN_HANDLE Handle,const MPP_CHN_S *pstChn);


#endif //__hitiny_region_h__

