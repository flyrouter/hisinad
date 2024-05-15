#include "hitiny_ive.h"
#include <sys/ioctl.h>

struct __IVE_Query_param_s
{
    IVE_HANDLE ive_handle;
    HI_BOOL block;
    HI_BOOL finish;
};

int hitiny_MPI_IVE_Query(IVE_HANDLE IveHandle, HI_BOOL *pbFinish, HI_BOOL bBlock)
{
    int fd = hitiny_ive_fd();
    if (fd < 0) return 0xa01d8010;

    struct __IVE_Query_param_s param;
    param.ive_handle = IveHandle;
    param.block = bBlock;
    param.finish = HI_FALSE;

    int result = ioctl(fd, 0xc00c460e, &param);
    if (result) return result;

    if (pbFinish) *pbFinish = param.finish;

    return 0;
}

struct __IVE_Hist_param_s
{
    IVE_HANDLE ive_handle;
    IVE_SRC_INFO_S src;
    IVE_MEM_INFO_S dest;
    HI_BOOL instant;
};

int hitiny_MPI_IVE_HIST(IVE_HANDLE *pIveHandle, IVE_SRC_INFO_S *pstSrc, IVE_MEM_INFO_S *pstDst, HI_BOOL bInstant)
{
    if (!pIveHandle || !pstSrc || !pstDst) return 0xa01d8006;

    int fd = hitiny_ive_fd();
    if (fd < 0) return 0xa01d8010;

    struct __IVE_Hist_param_s param;
    param.ive_handle = 0;
    param.src = *pstSrc;
    param.dest = *pstDst;
    param.instant = bInstant;

    int result = ioctl(fd, 0xc024460d, &param);
    if (result) return result;

    *pIveHandle = param.ive_handle;

    return 0;
}

struct __IVE_Filter_param_s
{
    IVE_HANDLE ive_handle;
    IVE_SRC_INFO_S src;
    IVE_MEM_INFO_S dest;
    HI_BOOL instant;
    IVE_FILTER_CTRL_S filter;
};

int hitiny_MPI_IVE_FILTER(IVE_HANDLE *pIveHandle, IVE_SRC_INFO_S *pstSrc, IVE_MEM_INFO_S *pstDst, IVE_FILTER_CTRL_S *pstFilterCtrl, HI_BOOL bInstant)
{
    if (!pIveHandle || !pstSrc || !pstDst || !pstFilterCtrl) return 0xa01d8006;

    int fd = hitiny_ive_fd();
    if (fd < 0) return 0xa01d8010;

    struct __IVE_Filter_param_s param;
    param.ive_handle = 0;
    param.src = *pstSrc;
    param.dest = *pstDst;
    param.instant = bInstant;
    param.filter = *pstFilterCtrl;

    int result = ioctl(fd, 0xc0304601, &param);
    if (result) return result;

    *pIveHandle = param.ive_handle;

    return 0;
}




