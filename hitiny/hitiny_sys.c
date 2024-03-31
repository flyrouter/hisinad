#include "hitiny_sys.h"
#include <errno.h>
#include <string.h>
#include <aux/logger.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "hi_common.h"

static int fd_SysDev = -1;
static int fd_MemDev = -1;

int hitiny_MPI_SYS_Init()
{
    if (fd_SysDev >= 0) return 0;

    fd_SysDev = hitiny_open_dev("/dev/sys");
    if (fd_SysDev < 0)
    {
        return -1;
    }

    return ioctl(fd_SysDev, 0x5900);
}

void hitiny_MPI_SYS_Done()
{
    if (fd_MemDev != -1)
    {
        close(fd_MemDev);
        fd_MemDev = -1;
    }

    if (fd_SysDev != -1)
    {
// We do NOT stop sys, because other daemons are working hard, we are not main daemon
//        ioctl(fd_SysDev, 0x5901);
        close(fd_SysDev);
        fd_SysDev = -1;
    }
}

typedef struct hitiny_sys_bind_param_s
{
    MPP_CHN_S src;
    MPP_CHN_S dest;
} hitiny_sys_bind_param_t;

int hitiny_sys_bind_VPSS_GROUP(int vpss_dev_id, int vpss_chn_id, int grp_id)
{
    if (fd_SysDev < 0)
    {
        fd_SysDev = hitiny_open_dev("/dev/sys");
        if (fd_SysDev < 0) return 0xA0028010;
    }

    hitiny_sys_bind_param_t param;
    param.src.enModId = HI_ID_VPSS;
    param.src.s32DevId = vpss_dev_id;
    param.src.s32ChnId = vpss_chn_id;

    param.dest.enModId = HI_ID_GROUP;
    param.dest.s32DevId = grp_id;
    param.dest.s32ChnId = 0;

    return ioctl(fd_SysDev, 0x40185907, &param);
}

int hitiny_sys_unbind_VPSS_GROUP(int vpss_dev_id, int vpss_chn_id, int grp_id)
{
    if (fd_SysDev < 0)
    {
        fd_SysDev = hitiny_open_dev("/dev/sys");
        if (fd_SysDev < 0) return 0xA0028010;
    }

    hitiny_sys_bind_param_t param;
    param.src.enModId = HI_ID_VPSS;
    param.src.s32DevId = vpss_dev_id;
    param.src.s32ChnId = vpss_chn_id;

    param.dest.enModId = HI_ID_GROUP;
    param.dest.s32DevId = grp_id;
    param.dest.s32ChnId = 0;

    return ioctl(fd_SysDev, 0x40185908, &param);
}

int hitiny_open_dev(const char* fname)
{
    struct stat stat_buf;

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

    int fd = open(fname, O_RDWR);
    if (fd < 0)
    {
        log_error("ERROR: can't open() on '%s', %d (%s)", fname, errno, strerror(errno));
        return -1;
    }

    return fd;
}


void* hitiny_MPI_SYS_Mmap(HI_U32 u32PhyAddr, HI_U32 u32Size)
{
    if (fd_MemDev < 0)
    {
        fd_MemDev = open("/dev/mem", 4162);
        if (fd_MemDev < 0)
        {
            log_error("Can't open /dev/mem: (%d) %s", errno, strerror(errno));
            return 0;
        }

        unsigned alligned = u32PhyAddr & 0xFFFFF000;
        unsigned offset = u32PhyAddr - (u32PhyAddr & 0xFFFFF000);
        unsigned sz = ((u32Size + offset - 1) & 0xFFFFF000) + 0x1000;
        
        void* ret = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd_MemDev, alligned);
        if (ret == MAP_FAILED) log_error("Can't mmap: (%d) %s", errno, strerror(errno));
        return ret + offset;
    }
    return 0;
}

int hitiny_MPI_SYS_Munmap(HI_VOID* pVirAddr, HI_U32 u32Size)
{
    unsigned virtaddr = ((unsigned int)pVirAddr);
    unsigned alligned = virtaddr & 0xFFFFF000;
    unsigned offset = virtaddr - (virtaddr & 0xFFFFF000);
    unsigned sz = ((u32Size + offset - 1) & 0xFFFFF000) + 0x1000;

    int ret = munmap((void*)alligned, sz);
    return ret;
}

