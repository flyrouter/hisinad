#include "hitiny_vda.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <aux/logger.h>
#include <hitiny/hitiny_sys.h>
#include <stdint.h>

typedef struct vda_chn_info_s
{
    int fd;
    unsigned phy_addr;
    void* ptr;
    unsigned sz;
} vda_chn_info_t;

static vda_chn_info_t vda_chns[VDA_CHN_NUM_MAX] = { -1 };

void hitiny_vda_init()
{
    memset(vda_chns, 0, VDA_CHN_NUM_MAX * sizeof(vda_chn_info_t));
    for (int i = 0; i < VDA_CHN_NUM_MAX; i++) vda_chns[i].fd = -1;
}

void hitiny_vda_done()
{
    for (int i = 0; i < VDA_CHN_NUM_MAX; i++)
    {
        if (vda_chns[i].fd >= 0) close(vda_chns[i].fd);
    }

    memset(vda_chns, 0, VDA_CHN_NUM_MAX * sizeof(vda_chn_info_t));
    for (int i = 0; i < VDA_CHN_NUM_MAX; i++) vda_chns[i].fd = -1;
}

int hitiny_MPI_VDA_GetFd(unsigned VdaChn)
{
    if (VDA_CHN_NUM_MAX <= VdaChn) return 0xa0098002;

    if (vda_chns[VdaChn].fd >=0) return vda_chns[VdaChn].fd;

    int fd = hitiny_open_dev("/dev/vda");
    if (fd < 0)
    {
        log_error("Can't open /dev/vda: %s (%d)", strerror(errno), errno);
        return fd;
    }

    unsigned param = VdaChn;
    int res = ioctl(fd, 0x40044d0d, &param);
    if (res)
    {
        close(fd);
        return res;
    }

    vda_chns[VdaChn].fd = fd;

    return fd;
}

int hitiny_MPI_VDA_CreateChn(VDA_CHN VdaChn, const VDA_CHN_ATTR_S *pstAttr)
{
    int vda_chn_fd = hitiny_MPI_VDA_GetFd(VdaChn);
    if (vda_chn_fd < 0) return vda_chn_fd;

    int ret = ioctl(vda_chn_fd, 0x40a84d00, pstAttr);
    if (ret)
    {
        log_error("Can't ioctl 0x40a84d00: 0x%x", ret);
        return ret;
    }

    unsigned param[2];
    ret = ioctl(vda_chn_fd, 0xc0084d0e, param);
    if (ret)
    {
        log_error("Can't ioctl 0xc0084d0e: 0x%x", ret);
        return ret;
    }

    // DBG 
    //log_info("GOT mem at 0x%x, sz=%u", param[0], param[1]);
    vda_chns[VdaChn].phy_addr = param[0];
    vda_chns[VdaChn].sz = param[1];

    void* ptr = hitiny_MPI_SYS_Mmap(vda_chns[VdaChn].phy_addr, vda_chns[VdaChn].sz);
    if (ptr == (void*)(-1))
    {
        log_error("Can't mmap: (%d) %s", errno, strerror(errno));
        ioctl(vda_chn_fd, 0X4d01);
        return 0xa009800c;
    }

    vda_chns[VdaChn].ptr = ptr;

    //log_info("vda chnl ptr is 0x%x", vda_chns[VdaChn].ptr);
    return 0;
}

int hitiny_MPI_VDA_DestroyChn(VDA_CHN VdaChn)
{
    int vda_chn_fd = hitiny_MPI_VDA_GetFd(VdaChn);

    if (vda_chn_fd < 0) return vda_chn_fd;

    int ret = 0;

    if (vda_chns[VdaChn].ptr)
    {
        ret = hitiny_MPI_SYS_Munmap(vda_chns[VdaChn].ptr, vda_chns[VdaChn].sz);
        vda_chns[VdaChn].phy_addr = 0;
        vda_chns[VdaChn].ptr = 0;
        vda_chns[VdaChn].sz = 0;
        if (ret) log_error("munmap at destroy VDA chnl failed: %s (%d)", strerror(errno), errno);
    }

    ret = ioctl(vda_chn_fd, 0x4d01);

    close(vda_chn_fd);
    vda_chns[VdaChn].fd = -1;
    return ret;
}

int hitiny_MPI_VDA_StartRecvPic(VDA_CHN VdaChn)
{
    int vda_chn_fd = hitiny_MPI_VDA_GetFd(VdaChn);

    if (vda_chn_fd < 0) return vda_chn_fd;

    return ioctl(vda_chn_fd, 0x4d06);
}

int hitiny_MPI_VDA_StopRecvPic(VDA_CHN VdaChn)
{
    int vda_chn_fd = hitiny_MPI_VDA_GetFd(VdaChn);

    if (vda_chn_fd < 0) return vda_chn_fd;

    return ioctl(vda_chn_fd, 0x4d07);
}

struct vda_get_data_param_t
{
    VDA_DATA_S vda_data;
    HI_BOOL blocking;
    uint32_t reserved[5];
};

int hitiny_MPI_VDA_GetData(VDA_CHN VdaChn, VDA_DATA_S *pstVdaData, HI_BOOL bBlock)
{
    int fd = hitiny_MPI_VDA_GetFd(VdaChn);

    if (fd < 0) return fd;

    if (!pstVdaData) return 0xa0098006;

    struct vda_get_data_param_t param;
    memset(&param, 0, sizeof(struct vda_get_data_param_t));
    param.blocking = bBlock;

    int ret = ioctl(fd, 0xc0504d08, &param);

    if (ret) return ret;

//    log_info("Got VDA DATA: enWorkMode = %u, enMbSize = %u, razmer %ux%u", 
//        param.vda_data.enWorkMode, param.vda_data.enMbSize,
//        param.vda_data.u32MbWidth, param.vda_data.u32MbHeight);

    *pstVdaData = param.vda_data;
    if (param.vda_data.enWorkMode == VDA_WORK_MODE_MD)
    {
        if (param.vda_data.unData.stMdData.bMbSadValid)
        {
            pstVdaData->unData.stMdData.stMbSadData.pAddr = vda_chns[VdaChn].ptr + (unsigned)pstVdaData->unData.stMdData.stMbSadData.pAddr - vda_chns[VdaChn].phy_addr;
        }
        if (param.vda_data.unData.stMdData.bObjValid)
        {
            pstVdaData->unData.stMdData.stObjData.pstAddr = (VDA_OBJ_S *)(vda_chns[VdaChn].ptr + (unsigned)pstVdaData->unData.stMdData.stObjData.pstAddr - vda_chns[VdaChn].phy_addr);
        }
    }

    return 0;
}

int hitiny_MPI_VDA_ReleaseData(VDA_CHN VdaChn, const VDA_DATA_S* pstVdaData)
{
    int fd = hitiny_MPI_VDA_GetFd(VdaChn);
    if (fd < 0) return fd;

    if (!pstVdaData) return 0xa0098006;

    return ioctl(fd, 0x40484d09, pstVdaData);
}

