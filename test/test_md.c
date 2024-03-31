#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <unistd.h>
#include <aux/logger.h>
#include <cfg/sensor_config.h>
#include <hitiny/hitiny_sys.h>
#include "hi_comm_vda.h"
#include <stdint.h>


int stop_flag = 0;

void action_on_signal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

typedef struct vda_chn_info_s
{
    int fd;
    unsigned phy_addr;
    void* ptr;
    unsigned sz;
} vda_chn_info_t;

static vda_chn_info_t vda_chns[VDA_CHN_NUM_MAX] = { 0 };

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

HI_S32 hitiny_MPI_VDA_DestroyChn(VDA_CHN VdaChn)
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

HI_S32 hitiny_MPI_VDA_StartRecvPic(VDA_CHN VdaChn)
{
    int vda_chn_fd = hitiny_MPI_VDA_GetFd(VdaChn);

    if (vda_chn_fd < 0) return vda_chn_fd;

    return ioctl(vda_chn_fd, 0x4d06);
}

HI_S32 hitiny_MPI_VDA_StopRecvPic(VDA_CHN VdaChn)
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

HI_S32 hitiny_MPI_VDA_GetData(VDA_CHN VdaChn, VDA_DATA_S *pstVdaData, HI_BOOL bBlock)
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

HI_S32 hitiny_MPI_VDA_ReleaseData(VDA_CHN VdaChn, const VDA_DATA_S* pstVdaData)
{
    int fd = hitiny_MPI_VDA_GetFd(VdaChn);
    if (fd < 0) return fd;

    if (!pstVdaData) return 0xa0098006;

    return ioctl(fd, 0x40484d09, pstVdaData);
}

void print_md_data(const VDA_DATA_S *pstVdaData)
{
    if (pstVdaData->unData.stMdData.bObjValid && pstVdaData->unData.stMdData.stObjData.u32ObjNum > 0)
    {
        log_info("ObjNum=%d, IndexOfMaxObj=%d, SizeOfMaxObj=%d, SizeOfTotalObj=%d", \
                   pstVdaData->unData.stMdData.stObjData.u32ObjNum, \
             pstVdaData->unData.stMdData.stObjData.u32IndexOfMaxObj, \
             pstVdaData->unData.stMdData.stObjData.u32SizeOfMaxObj,\
             pstVdaData->unData.stMdData.stObjData.u32SizeOfTotalObj);
        for (int i=0; i<pstVdaData->unData.stMdData.stObjData.u32ObjNum; i++)
        {
            VDA_OBJ_S *pstVdaObj = pstVdaData->unData.stMdData.stObjData.pstAddr + i;
            log_info("[%d]\t left=%d, top=%d, right=%d, bottom=%d", i, \
              pstVdaObj->u16Left, pstVdaObj->u16Top, \
              pstVdaObj->u16Right, pstVdaObj->u16Bottom);
        }
    }
/*
    if (!pstVdaData->unData.stMdData.bPelsNumValid)
    {
        log_info("bMbObjValid = FALSE");
    }
    else
    {
        log_info("AlarmPixelCount=%d\n", pstVdaData->unData.stMdData.u32AlarmPixCnt);
    }
*/
}

void test_RUN()
{
    unsigned VdaChn = 0;

    fd_set read_fds;
    struct timeval TimeoutVal;

    int fd = hitiny_MPI_VDA_GetFd(VdaChn);
    if (fd < 0) return;

    int maxfd = fd;

    while (!stop_flag)
    {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);

        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;
        int s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            log_error("select() error");
            return;
        }
        else if (s32Ret == 0)
        {
            log_info("TIMEOUT");
            break;
        }

        if (!FD_ISSET(fd, &read_fds))
        {
            log_error("WHAT???");
            return;
        }

        VDA_DATA_S stVdaData;

        s32Ret = hitiny_MPI_VDA_GetData(VdaChn, &stVdaData, HI_FALSE);

        if (!s32Ret)
        {
            print_md_data(&stVdaData);
        }
        else
        {
            log_error("MPI_VDA_GetData returned error 0x%x", s32Ret);
        }
        s32Ret = hitiny_MPI_VDA_ReleaseData(VdaChn,&stVdaData);
    }
}

void test_md()
{
    unsigned VdaChn = 0;

    VDA_CHN_ATTR_S stVdaChnAttr;

    stVdaChnAttr.enWorkMode = VDA_WORK_MODE_MD;
    stVdaChnAttr.u32Width   = 256;
    stVdaChnAttr.u32Height  = 144;

    stVdaChnAttr.unAttr.stMdAttr.enVdaAlg      = VDA_ALG_REF;
    stVdaChnAttr.unAttr.stMdAttr.enMbSize      = VDA_MB_16PIXEL;
    stVdaChnAttr.unAttr.stMdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
    stVdaChnAttr.unAttr.stMdAttr.enRefMode     = VDA_REF_MODE_DYNAMIC;
    stVdaChnAttr.unAttr.stMdAttr.u32MdBufNum   = 8;
    stVdaChnAttr.unAttr.stMdAttr.u32VdaIntvl   = 5;
    stVdaChnAttr.unAttr.stMdAttr.u32BgUpSrcWgt = 128;
    stVdaChnAttr.unAttr.stMdAttr.u32SadTh      = 2000;
    stVdaChnAttr.unAttr.stMdAttr.u32ObjNumMax  = 16;

    int fd = hitiny_MPI_VDA_GetFd(VdaChn);
    if(fd < 0)
    {
        log_error("Can't open VDA chn: 0x%x", fd);
        return;
    }

    hitiny_MPI_VDA_DestroyChn(VdaChn);

    int s32Ret = hitiny_MPI_VDA_CreateChn(VdaChn, &stVdaChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't create VDA chn: 0x%x", s32Ret);
        return;
    }

    log_info("going to bind");

    s32Ret = hitiny_sys_bind_VI_VDA(0, 1, VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't bind VDA to VI: 0x%x", s32Ret);
        hitiny_sys_unbind_VI_VDA(0, 1, VdaChn);
        hitiny_MPI_VDA_DestroyChn(VdaChn);
        return;
    }

    log_info("going to start recv");
    s32Ret = hitiny_MPI_VDA_StartRecvPic(VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't HI_MPI_VDA_StartRecvPic: 0x%x", s32Ret);
        hitiny_MPI_VDA_DestroyChn(VdaChn);
        return;
    }

    log_info("VDA init OK!");
    test_RUN();

    s32Ret = hitiny_MPI_VDA_StopRecvPic(VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't VDA chnl stop recv pic: 0x%x", s32Ret);
    }
    log_info("done StopRecvPic");

    s32Ret = hitiny_sys_unbind_VI_VDA(0, 1, VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't unbind VDA chn: 0x%x", s32Ret);
    }
    log_info("done unbind_VI_VDA");

    s32Ret = hitiny_MPI_VDA_DestroyChn(VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't destroy VDA chn: 0x%x", s32Ret);
    }
    log_info("done DestroyChn");
}


int main(int argc, char** argv)
{
    hitiny_vda_init();

    signal(SIGINT, action_on_signal);

    test_md();

    hitiny_vda_done();

    return 0;
}




