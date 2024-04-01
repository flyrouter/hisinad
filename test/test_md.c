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
#include <hitiny/hitiny_vda.h>
#include <stdint.h>


int stop_flag = 0;

void action_on_signal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
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

    if (!pstVdaData->unData.stMdData.bPelsNumValid)
    {
        log_info("bMbObjValid = FALSE");
    }
    else
    {
        log_info("AlarmPixelCount=%d", pstVdaData->unData.stMdData.u32AlarmPixCnt);
    }
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




