#include <hitiny/hitiny_vi.h>
#include <aux/logger.h>
#include <cfg/sensor_config.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

int stop_flag = 0;

void action_on_signal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

int create_vi_ext_chnl(unsigned width, unsigned height, unsigned pix_format, int src_frame_rate, int frame_rate)
{
    log_info("starting ext vi chnl 1: %ux%u, PixFormat = %s(%d), src_fr/fr = %d/%d", width, height, cfg_sensor_vals_vichn_pixel_format[pix_format], pix_format, src_frame_rate, frame_rate);

    VI_EXT_CHN_ATTR_S stViExtChnAttr;
    stViExtChnAttr.s32BindChn           = 0;
    stViExtChnAttr.stDestSize.u32Width  = width;
    stViExtChnAttr.stDestSize.u32Height = height;
    stViExtChnAttr.s32SrcFrameRate      = src_frame_rate;
    stViExtChnAttr.s32FrameRate         = frame_rate;
    stViExtChnAttr.enPixFormat          = pix_format;

    int s32Ret;
    s32Ret = hitiny_MPI_VI_SetExtChnAttr(1, &stViExtChnAttr);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VI_SetExtChnAttr failed with %#x!", s32Ret);
        return s32Ret;
    }

    s32Ret = hitiny_MPI_VI_EnableChn(1);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VI_SetExtChnAttr failed with %#x!", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

int main(int argc, char** argv)
{
    hitiny_MPI_VI_Init();

    signal(SIGINT, action_on_signal);
    signal(SIGTERM, action_on_signal);

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s width height\n", argv[0]);
        return -1;
    }

    int ret = create_vi_ext_chnl(atoi(argv[1]), atoi(argv[2]), 19, -1, -1);
    if (ret != 0)
    {
        fprintf(stderr, "Error, can't emable chnl\n");
        return -1;
    }

    for (int i = 10; i && !stop_flag; --i)
    {
        fprintf(stderr, "waiting for %d sec\n", i);
        sleep(1);
    }

    ret = hitiny_MPI_VI_DisableChn(1);
    printf("Disable cnhl: %#x\n", ret);

    hitiny_MPI_VI_Done();

    return 0;
}

