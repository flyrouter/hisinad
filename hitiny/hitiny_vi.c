#include "hitiny_vi.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <aux/logger.h>

static int g_hitiny_fd_vi[VIU_MAX_CHN_NUM] = { -1 };

void hitiny_MPI_VI_Init()
{
    for (unsigned i = 0; i < VIU_MAX_CHN_NUM; i++)
    {
        g_hitiny_fd_vi[i] = -1;
    }
}

void hitiny_MPI_VI_Done()
{
    for (unsigned i = 0; i < VIU_MAX_CHN_NUM; i++)
    {
        if (g_hitiny_fd_vi[i] >= 0)
        {
            close(g_hitiny_fd_vi[i]);
            g_hitiny_fd_vi[i] = -1;
        }
    }
}

int open_vi_chn_fd(VI_CHN ViChn)
{
    char fname[32];
    struct stat stat_buf;
    snprintf(fname, 32, "/dev/vi");
    if (stat(fname, &stat_buf) < 0)
    {
        log_error("ERROR: can't stat() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    if ((stat_buf.st_mode & 0xF000) != 0x2000)
    {
        log_error("ERROR: %s is not a device", fname);
        return -1;
    }

    int _new_chn_fd = open(fname, 2, 0);
    if (_new_chn_fd < 0)
    {
        log_error("ERROR: can't open() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    unsigned req_data = ViChn;
    if (ioctl(_new_chn_fd, 0x40044923, &req_data))
    {
        close(_new_chn_fd);
        log_error("ERROR: can't ioctl(0x40044923) for vi channel %u", ViChn);

        return -1;
    }

    return _new_chn_fd;

}

int hitiny_MPI_VI_GetFd(VI_CHN ViChn)
{
    if (ViChn >= VIU_MAX_CHN_NUM) return 0xA0108002;

    int fd = g_hitiny_fd_vi[ViChn];

    if (fd < 0)
    {
        fd = open_vi_chn_fd(ViChn);
        g_hitiny_fd_vi[ViChn] = fd;

        if (-1 == fd)
        {
            return 0xA0108010;
        }
    }

    return fd;
}

int hitiny_MPI_VI_GetFrame(VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo)
{
    int fd = hitiny_MPI_VI_GetFd(ViChn);

    if (fd < 0) return fd;

    return ioctl(fd, 0x80584914, pstFrameInfo);
}

int hitiny_MPI_VI_GetFrameTimeOut(VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo, unsigned u32MilliSec)
{
    return -13;
}

int hitiny_MPI_VI_ReleaseFrame(VI_CHN ViChn, VIDEO_FRAME_INFO_S *pstFrameInfo)
{
    int fd = hitiny_MPI_VI_GetFd(ViChn);

    if (fd < 0) return fd;

    return ioctl(fd, 0x40584916, pstFrameInfo);
}

int hitiny_MPI_VI_SetFrameDepth(VI_CHN ViChn, unsigned u32Depth)
{
    int fd = hitiny_MPI_VI_GetFd(ViChn);

    if (fd < 0) return fd;

    unsigned param = u32Depth;

    return ioctl(fd, 0x40044912, &param);
}

int hitiny_MPI_VI_GetFrameDepth(VI_CHN ViChn, unsigned *pu32Depth)
{
    if (!pu32Depth) return 0xA0108006;

    int fd = hitiny_MPI_VI_GetFd(ViChn);

    if (fd < 0) return fd;

    return ioctl(fd, 0xC0044913, pu32Depth);
}


