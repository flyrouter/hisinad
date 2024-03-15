#include "sdk.h"

#include <string.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "hi_comm_ao.h"
#include "mpi_ao.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "hi_comm_adec.h"
#include "mpi_adec.h"

#include "acodec.h"


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

int init_sdk_audio_StartAdec(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    HI_S32 s32Ret;
    ADEC_CHN_ATTR_S stAdecAttr;

    stAdecAttr.enType = enType;
    stAdecAttr.u32BufSize = 20;
    stAdecAttr.enMode = ADEC_MODE_STREAM;

    ADEC_ATTR_ADPCM_S stAdpcm;
    ADEC_ATTR_G711_S stAdecG711;
    ADEC_ATTR_G726_S stAdecG726;
    ADEC_ATTR_LPCM_S stAdecLpcm;

    if (PT_ADPCMA == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdpcm;
        stAdpcm.enADPCMType = AUDIO_ADPCM_TYPE;
    }
    else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
    {
        ADEC_ATTR_G711_S stAdecG711;
        stAdecAttr.pValue = &stAdecG711;
    }
    else if (PT_G726 == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG726;
        stAdecG726.enG726bps = G726_BPS;
    }
    else if (PT_LPCM == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecLpcm;
        stAdecAttr.enMode = ADEC_MODE_PACK;
    }
    else
    {
        __sdk_last_call = "stAdecAttr.enType";
        return -1;
    }

    __sdk_last_call = "HI_MPI_ADEC_CreateChn";
    s32Ret = HI_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);
    return s32Ret;
}

int init_sdk_audio_StartAo(AUDIO_DEV AoDevId, AO_CHN AoChn, AIO_ATTR_S *pstAioAttr, AUDIO_RESAMPLE_ATTR_S *pstAoReSmpAttr)
{
    HI_S32 s32Ret;

    __sdk_last_call = "HI_MPI_AO_SetPubAttr";
    s32Ret = HI_MPI_AO_SetPubAttr(AoDevId, pstAioAttr);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_AO_Enable";
    s32Ret = HI_MPI_AO_Enable(AoDevId);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_AO_EnableChn";
    s32Ret = HI_MPI_AO_EnableChn(AoDevId, AoChn);

    return s32Ret;
}


int sdk_audio_init(int bMic, PAYLOAD_TYPE_E payload_type, int audio_rate)
{
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

    HI_S32 s32Ret;

//    s32Ret = init_sdk_audio_CfgAcodec(stAioAttr.enSamplerate, bMic);
//    if (HI_SUCCESS != s32Ret) return s32Ret;

    AUDIO_DEV AoDev = 0;
    AO_CHN AoChn = 0;
    ADEC_CHN AdChn = 0;

//    s32Ret = init_sdk_audio_StartAdec(AoChn, payload_type);
//    if (HI_SUCCESS != s32Ret) return s32Ret;

    s32Ret = init_sdk_audio_StartAo(AoDev, AoChn, &stAioAttr, 0);
    if (HI_SUCCESS != s32Ret) return s32Ret;

fprintf(stderr, "don't bind ADEC to AO, testing\n");
return 0;

    MPP_CHN_S stSrcChn,stDestChn;
    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    return 0;
}

void sdk_audio_done()
{
    AUDIO_DEV AoDev = 0;
    AO_CHN AoChn = 0;
    ADEC_CHN AdChn = 0;

    HI_MPI_AO_DisableChn(AoDev, AoChn);
    HI_MPI_AO_Disable(AoDev);

    HI_MPI_ADEC_DestroyChn(AdChn);

    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32ChnId = AdChn;
    stSrcChn.s32DevId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

static void* th_adec_proc(void* p)
{
    fprintf(stderr, "TH\n");

    AUDIO_FRAME_INFO_S frame_info;

    FILE *f = fopen("/tmp/yyy.bin", "wb+");
    int ret = 0;
    while (!ret)
    {
        ret = HI_MPI_ADEC_GetFrame(0, &frame_info, HI_TRUE);

        if (!ret)
        {
            HI_MPI_AO_SendFrame(0, 0, frame_info.pstFrame, HI_TRUE);
            fprintf(stderr, "got frame, ret == %d!\n", ret);
            fprintf(stderr, "    enBitwidth = %u, enSoundmode = %u\n", frame_info.pstFrame->enBitwidth, frame_info.pstFrame->enSoundmode);
            fprintf(stderr, "    pVirAddr = [0x%08x, 0x%08x]\n", frame_info.pstFrame->pVirAddr[0], frame_info.pstFrame->pVirAddr[1]);
            fprintf(stderr, "    u32PhyAddr = [0x%08x, 0x%08x]\n", frame_info.pstFrame->u32PhyAddr[0], frame_info.pstFrame->u32PhyAddr[1]);
            fprintf(stderr, "    u32Seq = %u, u32Len = %u\n", frame_info.pstFrame->u32Seq, frame_info.pstFrame->u32Len);
            fprintf(stderr, "    u32PoolId = [%u, %u]\n", frame_info.pstFrame->u32PoolId[0], frame_info.pstFrame->u32PoolId[1]);
            fprintf(stderr, "    u64TimeStamp = %llu\n", frame_info.pstFrame->u64TimeStamp);
            fwrite(frame_info.pstFrame->pVirAddr[1], frame_info.pstFrame->u32Len, 1, f);
            HI_MPI_ADEC_ReleaseFrame(0, &frame_info);
        }
    }

    fclose(f);

    fprintf(stderr, "TH end\n");
}

int sdk_audio_play(FILE* f, int* stop_flag)
{
    AUDIO_STREAM_S stAudioStream;
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn;
    HI_U8 *pu8AudioStream = NULL;
    s32AdecChn = 0;

//    pthread_t th;
//    pthread_create(&th, 0, th_adec_proc, 0);

    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (!pu8AudioStream) return -1;

    unsigned id = 0;
    while (!stop_flag || !*stop_flag)
    {
        stAudioStream.pStream = pu8AudioStream;
        u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, f);
//fprintf(stderr, "read %u bytes from stream\n", u32ReadLen);
        if (u32ReadLen <= 0)
        {
            free(pu8AudioStream);
            return 0;
        }

        stAudioStream.u32Len = u32ReadLen; 
//        HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
        AUDIO_FRAME_S frame;
        frame.enBitwidth = AUDIO_BIT_WIDTH_16;
        frame.enSoundmode = AUDIO_SOUND_MODE_MONO;
        frame.pVirAddr[0] = stAudioStream.pStream;
        frame.pVirAddr[1] = 0;
        frame.u32PhyAddr[0] = 0;
        frame.u32PhyAddr[1] = 0;
        frame.u64TimeStamp = 0;
        frame.u32Seq = id;
        frame.u32Len = u32ReadLen;
        frame.u32PoolId[0] = 0;
        frame.u32PoolId[1] = 0;
        HI_MPI_AO_SendFrame(0, 0, &frame, HI_TRUE);

        id++;
    }
fprintf(stderr, "completed stream\n");
//    pthread_join(th, 0);
    free(pu8AudioStream);
    return 1;
}

