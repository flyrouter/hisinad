#include "hitiny_region.h"
#include <hitiny/hitiny_sys.h>
#include <unistd.h>
#include <sys/ioctl.h>

int g_rgn_fd = -1;

int hitiny_region_init()
{
    if (g_rgn_fd >= 0) return 0;

    int fd = hitiny_open_dev("/dev/rgn");
    if (fd < 0) return fd;

    g_rgn_fd = fd;

    return 0;
}

struct mpi_rgn_create_param_s
{
    RGN_HANDLE Handle;
    RGN_ATTR_S stRegion;
};

int hitiny_MPI_RGN_Create(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion)
{
    hitiny_region_init();

    if (g_rgn_fd < 0) return 0xA0128010;
    if (!pstRegion) return 0xA0128006;

    struct mpi_rgn_create_param_s param;
    param.Handle = Handle;
    param.stRegion = *pstRegion;

    return ioctl(g_rgn_fd, 0xC0185200, &param);
}

struct mpi_rgn_attach_param_s
{
    RGN_HANDLE Handle;
    MPP_CHN_S stChn;
    RGN_CHN_ATTR_S stChnAttr;
};

int hitiny_MPI_RGN_AttachToChn(RGN_HANDLE Handle,const MPP_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
    hitiny_region_init();

    if (g_rgn_fd < 0) return 0xA0128010;
    if (!pstChn || !pstChnAttr) return 0xA0128006;

    struct mpi_rgn_attach_param_s param;
    param.stChn = *pstChn;
    param.Handle = Handle;
    param.stChnAttr = *pstChnAttr;

    return ioctl(g_rgn_fd, 0x40485208, &param);
};

struct mpi_rgn_detach_param_s
{
    RGN_HANDLE Handle;
    MPP_CHN_S stChn;
};

int hitiny_MPI_RGN_DetachFrmChn(RGN_HANDLE Handle, const MPP_CHN_S *pstChn)
{
    hitiny_region_init();

    if (g_rgn_fd < 0) return 0xA0128010;
    if (!pstChn) return 0xA0128006;

    struct mpi_rgn_detach_param_s param;
    param.Handle = Handle;
    param.stChn = *pstChn;

    return ioctl(g_rgn_fd, 0xC0105209, &param);
}

int hitiny_MPI_RGN_Destroy(RGN_HANDLE Handle)
{
    hitiny_region_init();

    if (g_rgn_fd < 0) return 0xA0128010;

    RGN_HANDLE param = Handle;
    return ioctl(g_rgn_fd, 0x40045201, &param);
}

void hitiny_region_done()
{
    if (g_rgn_fd >= 0)
    {
        close(g_rgn_fd);
        g_rgn_fd = -1;
    }
}

