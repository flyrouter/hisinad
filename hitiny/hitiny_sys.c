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
static int fd_MmzDev = -1;

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

    if (fd_MmzDev != -1)
    {
        close(fd_MmzDev);
        fd_MmzDev = -1;
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

int hitiny_sys_bind_VI_VDA(int vi_dev, int vi_chn, int vda_chn)
{
    if (fd_SysDev < 0)
    {
        fd_SysDev = hitiny_open_dev("/dev/sys");
        if (fd_SysDev < 0) return 0xA0028010;
    }

    hitiny_sys_bind_param_t param;
    param.src.enModId = HI_ID_VIU;
    param.src.s32DevId = vi_dev;
    param.src.s32ChnId =vi_chn;

    param.dest.enModId = HI_ID_VDA;
    param.dest.s32ChnId = vda_chn;
    param.dest.s32DevId = 0;

    return ioctl(fd_SysDev, 0x40185907, &param);
}

int hitiny_sys_unbind_VI_VDA(int vi_dev, int vi_chn, int vda_chn)
{
    if (fd_SysDev < 0)
    {
        fd_SysDev = hitiny_open_dev("/dev/sys");
        if (fd_SysDev < 0) return 0xA0028010;
    }

    hitiny_sys_bind_param_t param;
    param.src.enModId = HI_ID_VIU;
    param.src.s32DevId = vi_dev;
    param.src.s32ChnId =vi_chn;

    param.dest.enModId = HI_ID_VDA;
    param.dest.s32ChnId = vda_chn;
    param.dest.s32DevId = 0;

    return ioctl(fd_SysDev, 0x40185908, &param);
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
    }

    unsigned alligned = u32PhyAddr & 0xFFFFF000;
    unsigned offset = u32PhyAddr - (u32PhyAddr & 0xFFFFF000);
    unsigned sz = ((u32Size + offset - 1) & 0xFFFFF000) + 0x1000;
        
    void* ret = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd_MemDev, alligned);
    if (ret == MAP_FAILED)
    {
        log_error("Can't mmap: (%d) %s", errno, strerror(errno));
        return 0;
    }
    return ret + offset;
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

#define HIL_MMZ_NAME_LEN 32
#define HIL_MMB_NAME_LEN 16

struct _mmb_info
{
    unsigned long phys_addr;    /* phys-memory address */
    unsigned long align;        /* if you need your phys-memory have special align size */
    unsigned long size;     /* length of memory you need, in bytes */
    unsigned int order;
    void *mapped;           /* userspace mapped ptr */

    union
    {
        struct
        {
            unsigned long prot  :8; /* PROT_READ or PROT_WRITE */
            unsigned long flags :12;/* MAP_SHARED or MAP_PRIVATE */
        };
        unsigned long w32_stuf;
    };


    char mmb_name[HIL_MMB_NAME_LEN];
    char mmz_name[HIL_MMZ_NAME_LEN];
    unsigned long gfp;      /* reserved, do set to 0 */
    unsigned pid;
};

static int init_mmz_dev()
{
    if (fd_MmzDev >= 0) return 0;

    fd_MmzDev = hitiny_open_dev("/dev/mmz_userdev");

    if (fd_MmzDev < 0)
    {
        log_error("Can't open /dev/mmz_userdev: (%d) %s", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int hitiny_MPI_SYS_MmzAlloc(HI_U32 *pu32PhyAddr, HI_VOID **ppVirtAddr, const HI_CHAR *strMmb, const HI_CHAR *strZone, HI_U32 u32Len)
{
    if (init_mmz_dev()) return -1;

    struct _mmb_info _mmb;
    memset(&_mmb, 0, sizeof(struct _mmb_info));

    _mmb.prot = PROT_READ | PROT_WRITE;
    _mmb.flags = MAP_SHARED;
    _mmb.size = u32Len;

    strncpy(_mmb.mmb_name, strMmb ? strMmb : "User", HIL_MMB_NAME_LEN);
    if (strZone) strncpy(_mmb.mmz_name, strZone, HIL_MMZ_NAME_LEN);

    int ret = ioctl(fd_MmzDev, 0xc04c6d0a, &_mmb);
    if (ret)
    {
        log_error("MMZ Alloc error: %d", ret);
        return -1;
    }

    ret = ioctl(fd_MmzDev, 0xc04c6d14, &_mmb);
    if (ret)
    {
        log_error("MMZ remap failed: %d", ret);
        return -1;
    }

    *pu32PhyAddr = _mmb.phys_addr;
    *ppVirtAddr = _mmb.mapped;

    return 0;
}

int hitiny_MPI_SYS_MmzAlloc_Cached(HI_U32 *pu32PhyAddr, HI_VOID **ppVirtAddr, const HI_CHAR *strMmb, const HI_CHAR *strZone, HI_U32 u32Len)
{
    if (init_mmz_dev()) return -1;

    struct _mmb_info _mmb;
    memset(&_mmb, 0, sizeof(struct _mmb_info));

    _mmb.prot = PROT_READ | PROT_WRITE;
    _mmb.flags = MAP_SHARED;
    _mmb.size = u32Len;

    strncpy(_mmb.mmb_name, strMmb ? strMmb : "User", HIL_MMB_NAME_LEN);
    if (strZone) strncpy(_mmb.mmz_name, strZone, HIL_MMZ_NAME_LEN);

    int ret = ioctl(fd_MmzDev, 0xc04c6d0a, &_mmb);
    if (ret)
    {
        log_error("MMZ Alloc error: %d", ret);
        return -1;
    }

    ret = ioctl(fd_MmzDev, 0xc04c6d15, &_mmb);
    if (ret)
    {
        log_error("MMZ remap failed: %d", ret);
        return -1;
    }

    *pu32PhyAddr = _mmb.phys_addr;
    *ppVirtAddr = _mmb.mapped;

    return 0;
}

int hitiny_MPI_SYS_MmzFlushCache(HI_U32 u32PhyAddr, HI_VOID *pVirtAddr, HI_U32 u32Size)
{
    if (init_mmz_dev()) return -1;

    if (!u32PhyAddr)
    {
        int r = ioctl(fd_MmzDev, 0x6328, 0);
        if (r)
        {
            log_error("MMZ flush cache error %d (errno %d: %s)", r, errno, strerror(errno));
            return r;
        }

        return 0;
    }

    unsigned param[11];
    memset(&param, 0, sizeof(unsigned) * 11);
    param[0] = u32PhyAddr;
    param[1] = (unsigned)pVirtAddr;
    param[2] = u32Size;

    int r = ioctl(fd_MmzDev, 0x400c6432, param);
    if (r)
    {
        log_error("MMZ flush cache error %d (errno %d: %s)", r, errno, strerror(errno));
        return r;
    }

    return 0;
}

int hitiny_MPI_SYS_MmzFree(HI_U32 u32PhyAddr, HI_VOID *pVirtAddr)
{
    if (init_mmz_dev()) return -1;

    struct _mmb_info _mmb;
    memset(&_mmb, 0, sizeof(struct _mmb_info));

    _mmb.phys_addr = u32PhyAddr;
    _mmb.mapped = pVirtAddr;

    int ret = ioctl(fd_MmzDev, 0xc04c6d16, &_mmb);
    if (ret)
    {
        log_error("MMZ unmap failed: %d", ret);
        return -1;
    }

    ret = ioctl(fd_MmzDev, 0x404c6d0c, &_mmb);
    if (ret)
    {
        log_error("MMZ free failed: %d", ret);
        return -1;
    }

    return 0;
}


