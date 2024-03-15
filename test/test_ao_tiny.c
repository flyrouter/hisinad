#include <platform/sdk.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <aux/logger.h>
#include <hitiny/hitiny_sys.h>
#include <hitiny/hitiny_aio.h>

#define AUDIO_PTNUMPERFRM   320

int stop_flag = 0;

void action_on_sognal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

int hitiny_audio_play_file(FILE* f, int* stop_flag)
{
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_U8 *pu8AudioStream = NULL;

    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (!pu8AudioStream) return -1;

    unsigned id = 0;

    while (!stop_flag || !*stop_flag)
    {
        u32ReadLen = fread(pu8AudioStream, 1, u32Len, f);
        if (u32ReadLen <= 0)
        {
            free(pu8AudioStream);
            return 0;
        }

        AUDIO_FRAME_S frame;
        frame.enBitwidth = AUDIO_BIT_WIDTH_16;
        frame.enSoundmode = AUDIO_SOUND_MODE_MONO;
        frame.pVirAddr[0] = pu8AudioStream;
        frame.pVirAddr[1] = 0;
        frame.u32PhyAddr[0] = 0;
        frame.u32PhyAddr[1] = 0;
        frame.u64TimeStamp = 0;
        frame.u32Seq = id;
        frame.u32Len = u32ReadLen;
        frame.u32PoolId[0] = 0;
        frame.u32PoolId[1] = 0;

        int res = hitiny_MPI_AO_SendFrame(0, 0, &frame, HI_TRUE);
        if (res != 0)
        {
             fprintf(stderr, "hitiny_MPI_AO_SendFrame return: 0x%08x\n", res);
exit(-2);
        }

        id++;
    }
    free(pu8AudioStream);
    return 1;
}

int my_audio_init(int bMic, PAYLOAD_TYPE_E payload_type, int audio_rate)
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

    int ret = hitiny_MPI_AO_Init();
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_Init() failed: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_Disable(0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_DisableDev: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_SetPubAttr(0, &stAioAttr);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_SetPubAttr failed: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_Enable(0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_EnableDev: 0x%X", ret);
        return ret;
    }

    ret = hitiny_MPI_AO_EnableChn(0, 0);
    if (ret < 0)
    {
        log_error("hitiny_MPI_AO_EnableChn: 0x%X", ret);
        return ret;
    }

    return 0;
}



int main(int argc, char** argv)
{
    if (argc < 2)
    {
        log_error("./test_ao file_to_play.pcm");
        return -1;

    }

    signal(SIGINT, action_on_sognal);

    log_warn("Warning! No SDK version with!");

    int ret = hitiny_MPI_SYS_Init();
    if (ret < 0)
    {
        log_error("hitiny_MPI_SYS_Init() failed: 0x%X", ret);
        return -1;
    }
    ret = hitiny_init_acodec(48000, HI_TRUE);
    if (ret < 0)
    {
        log_error("hitiny_init_acodec() failed: 0x%X", ret);
        return -1;
    }

    ret = my_audio_init(0, PT_LPCM, 48000);
    if (ret < 0)
    {
        log_crit("my_audio_init 0x%X", ret);
        return -1;
    }

    FILE* f = fopen(argv[1], "r");
    if (f)
    {
        log_info("Now play %s!", argv[1]);
        hitiny_audio_play_file(f, &stop_flag);
        fclose(f);
    }
    else
    {
        log_error("ERROR %d", errno);
        log_error("Error %d: %s", errno, strerror(errno));
    }

    hitiny_MPY_SYS_Done();

    return 0;
}




