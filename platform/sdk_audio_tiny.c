#include "sdk.h"

#include <string.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>

#include "acodec.h"

const char* __sdk_last_call = 0;


#define AUDIO_PTNUMPERFRM   320

#define ACODEC_FILE "/dev/acodec"
#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

int init_sdk_audio_CfgAcodec(AUDIO_SAMPLE_RATE_E enSample, HI_BOOL bMicIn)
{
    HI_S32 fdAcodec = -1;

    __sdk_last_call = "open(" ACODEC_FILE ")";
    fdAcodec = open(ACODEC_FILE,O_RDWR);
    if (fdAcodec < 0) return -1;

    HI_S32 s32Ret = 0;
    s32Ret = ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL);
    if (0 != s32Ret)
    {
        __sdk_last_call = "acodec ACODEC_SOFT_RESET_CTRL";
        close(fdAcodec);
        return s32Ret;
    }

    unsigned int i2s_fs_sel = 0;
    switch (enSample)
    {
        case AUDIO_SAMPLE_RATE_8000:
        case AUDIO_SAMPLE_RATE_11025:
        case AUDIO_SAMPLE_RATE_12000:
            i2s_fs_sel = 0x18;
            break;

        case AUDIO_SAMPLE_RATE_16000:
        case AUDIO_SAMPLE_RATE_22050:
        case AUDIO_SAMPLE_RATE_24000:
            i2s_fs_sel = 0x19;
            break;

        case AUDIO_SAMPLE_RATE_32000:
        case AUDIO_SAMPLE_RATE_44100:
        case AUDIO_SAMPLE_RATE_48000:
            i2s_fs_sel = 0x1a;
            break;

        default:
            __sdk_last_call = "AUDIO_SAMPLE_RATE";
            close(fdAcodec);
            return -1;
    }


    if (ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel))
    {
        __sdk_last_call = "acodec ACODEC_SET_I2S1_FS";
        s32Ret = -1;
    }

    close(fdAcodec);
    return s32Ret;
}

int hitiny_MPI_AO_GetFd(AUDIO_DEV AoDevId, AO_CHN AoChn)
{
    int fd = open("/dev/ao", 2);

    if (fd < 0) return fd;

    unsigned param = AoDevId * 2 + AoChn;

    int ret = ioctl(fd, 0x40045800, &param);
    if (ret)
    {
        close(fd);
        return ret;
    }

    return fd;
}

int hitiny_MPI_AO_SetPubAttr(int fd, AIO_ATTR_S *pstAioAttr)
{
    return ioctl(fd, 0x40245801, pstAioAttr);
}

int hitiny_MPI_AO_EnableDev(int fd)
{
    return ioctl(fd, 0x5803);
}

int xhitiny_MPI_AO_EnableChn(int fd)
{
    return ioctl(fd, 0x5808);
}

int hitiny_MPI_AO_DisableDev(int fd)
{
    return ioctl(fd, 0x5804);
}

int hitiny_MPI_AO_DisableChn(int fd)
{
    return ioctl(fd, 0x5809);
}

int fd_AO = -1;

int init_sdk_audio_StartAo(AUDIO_DEV AoDevId, AO_CHN AoChn, AIO_ATTR_S *pstAioAttr, AUDIO_RESAMPLE_ATTR_S *pstAoReSmpAttr)
{
    HI_S32 s32Ret;

    __sdk_last_call = "hitiny_MPI_AO_GetFd";
    int fd = hitiny_MPI_AO_GetFd(0, 0);
    if (fd < 0) return fd;

    fprintf(stderr, "AO fd=%d (chn 0)\n", fd);

    __sdk_last_call = "HI_MPI_AO_SetPubAttr";
    s32Ret = hitiny_MPI_AO_SetPubAttr(fd, pstAioAttr);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_AO_Enable";
    s32Ret = hitiny_MPI_AO_EnableDev(fd);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    s32Ret = hitiny_MPI_AO_EnableChn(fd);

    fd_AO = fd;

    return s32Ret;
}


int sdk_audio_init(int bMic, PAYLOAD_TYPE_E payload_type, int audio_rate)
{
    AUDIO_DEV AoDev = 0;
    AO_CHN AoChn = 0;

    AIO_ATTR_S stAioAttr;
    memset(&stAioAttr, 0, sizeof(AIO_ATTR_S));

    stAioAttr.enSamplerate = audio_rate;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt = 2;
    stAioAttr.u32ClkSel = 1;

    __sdk_last_call = "init_sdk_audio_StartAo";
    HI_S32 s32Ret = init_sdk_audio_StartAo(AoDev, AoChn, &stAioAttr, 0);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    return 0;
}

void sdk_audio_done()
{
    if (fd_AO > 0)
    {
        hitiny_MPI_AO_DisableChn(fd_AO);
        hitiny_MPI_AO_DisableDev(fd_AO);
        close(fd_AO);
        fd_AO = -1;
    }

}

int xhitiny_MPI_AO_SendFrame(int fd, AUDIO_FRAME_S* frame, int blocking_mode)
{
    if (!blocking_mode)
    {
        struct pollfd x;
        x.fd = fd;
        x.events = POLLOUT;
        poll(&x, 1, 0);
        if (x.events != POLLOUT) return 0xA016800F;
    }

    int result = ioctl(fd, 0x40305805, frame);

    return result;
}

