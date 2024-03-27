#include "sdk.h"
#include <aux/logger.h>
#include "mpi_sys.h"
#include "mpi_isp.h"
#include "mpi_vb.h"
#include "mpi_ae.h"
#include "mpi_af.h"
#include "mpi_vi.h"
#include "mpi_awb.h"
#include "mpi_sys.h"
#include "mpi_vpss.h"
#include <pthread.h>
#include <unistd.h>

HI_S32 HI_MPI_SYS_GetChipId(HI_U32 *pu32ChipId);

static pthread_t th_isp_runner = 0;

void* isp_thread_proc(void *p)
{
    HI_S32 s32Ret = HI_MPI_ISP_Run();

    if (HI_SUCCESS != s32Ret)
    {
        log_crit("HI_MPI_ISP_Run failed with %#x!", s32Ret);
    }
    log_info("Shutdown isp_run thread\n");

    return 0;
}

int sdk_isp_init(const struct SensorConfig* sc)
{
    int s32Ret;

    s32Ret = HI_MPI_AE_Register(
        &(ALG_LIB_S){.s32Id = 0, .acLibName = "hisi_ae_lib"});
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_AE_Register/hisi_ae_lib failed with %#x!", s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_AF_Register(
        &(ALG_LIB_S){.s32Id = 0, .acLibName = "hisi_af_lib"});
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_AF_Register/hisi_af_lib failed with %#x!", s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_AWB_Register(
        &(ALG_LIB_S){.s32Id = 0, .acLibName = "hisi_awb_lib"});
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_AWB_Register/hisi_awb_lib failed with %#x!", s32Ret);
        return -1;
    }

    HI_U32 chipId;
    s32Ret = HI_MPI_SYS_GetChipId(&chipId);
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_SYS_GetChipId failed with %#x!", s32Ret);
        return -1;
    }
    log_info("HI_MPI_SYS_GetChipId: %#X", chipId);

    s32Ret = HI_MPI_ISP_Init();
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_ISP_Init failed with %#x!", s32Ret);
        return -1;
    }

    ISP_IMAGE_ATTR_S img_attr;
    img_attr.u16Width = sc->isp.isp_w;
    img_attr.u16Height = sc->isp.isp_h;
    img_attr.u16FrameRate = sc->isp.isp_frame_rate;
    img_attr.enBayer = sc->isp.isp_bayer;
    s32Ret = HI_MPI_ISP_SetImageAttr(&img_attr);
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_ISP_SetImageAttr failed with %#x!", s32Ret);
        return -1;
    }

    ISP_INPUT_TIMING_S stInputTiming;
    stInputTiming.u16HorWndStart = sc->isp.isp_x;
    stInputTiming.u16HorWndLength = sc->isp.isp_w;
    stInputTiming.u16VerWndStart = sc->isp.isp_y;
    stInputTiming.u16VerWndLength = sc->isp.isp_h;
    stInputTiming.enWndMode = ISP_WIND_ALL;
    s32Ret = HI_MPI_ISP_SetInputTiming(&stInputTiming);
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_ISP_SetInputTiming failed with %#x!", s32Ret);
        return -1;
    }

    ISP_AE_ATTR_S stAEAttr;
    HI_MPI_ISP_GetAEAttr(&stAEAttr);
    log_info("ExpTime in [%u, %u]", stAEAttr.u16ExpTimeMin, stAEAttr.u16ExpTimeMax);
    log_info("AGain in [%u, %u]", stAEAttr.u16AGainMin, stAEAttr.u16AGainMax);
    log_info("DGain in [%u, %u]", stAEAttr.u16DGainMin, stAEAttr.u16DGainMax);
    log_info("ExpTime in [%u, %u]", stAEAttr.u16ExpTimeMin, stAEAttr.u16ExpTimeMax);
    log_info("AGain in [%u, %u]", stAEAttr.u16AGainMin, stAEAttr.u16AGainMax);
    log_info("DGain in [%u, %u]", stAEAttr.u16DGainMin, stAEAttr.u16DGainMax);
    s32Ret = HI_MPI_ISP_SetAEAttr(&stAEAttr);
    if (HI_SUCCESS != s32Ret) {
        log_crit("HI_MPI_ISP_SetAEAttr failed with %#x!", s32Ret);
        return -1;
    }

    ISP_SHARPEN_ATTR_S sharpen_attr;
    HI_MPI_ISP_GetSharpenAttr(&sharpen_attr);

    log_info("Sharpen bEnable = %c, bManualEnable = %c, u8StrengthTarget=%u, u8StrengthMin=%u", sharpen_attr.bEnable ? 'Y':'N', sharpen_attr.bManualEnable ? 'Y':'N', sharpen_attr.u8StrengthTarget, sharpen_attr.u8StrengthMin);

    sharpen_attr.bManualEnable = HI_TRUE;
    sharpen_attr.u8StrengthTarget = 160;
    
    log_info("Sharpen bEnable = %c, bManualEnable = %c, u8StrengthTarget=%u, u8StrengthMin=%u", sharpen_attr.bEnable ? 'Y':'N', sharpen_attr.bManualEnable ? 'Y':'N', sharpen_attr.u8StrengthTarget, sharpen_attr.u8StrengthMin);

    HI_MPI_ISP_SetSharpenAttr(&sharpen_attr);

    pthread_create(&th_isp_runner, 0, isp_thread_proc, NULL);

    sleep(1);

    return 0;
}

void sdk_isp_done()
{
    log_info("HI_MPI_ISP_Exit");

    HI_MPI_ISP_Exit();

    log_info("Join ISP thread if running...");
    if (th_isp_runner)
    {
        pthread_join(th_isp_runner, 0);
        th_isp_runner = 0;
    }

    log_info("ISP done: OK");
}

int sdk_vi_init(const struct SensorConfig* sc)
{
#ifdef ISP_EX_ATTR
    VI_DEV_ATTR_EX_S vi_dev_attr = {0};
    vi_dev_attr.enInputMode = sc->videv.input_mod;
#else
    VI_DEV_ATTR_S vi_dev_attr = {0};
    vi_dev_attr.enIntfMode = sc->videv.input_mod;
#endif
    // memset(&vi_dev_attr, 0, sizeof(VI_DEV_ATTR_EX_S));
    vi_dev_attr.au32CompMask[0] = sc->videv.mask_0;
    vi_dev_attr.au32CompMask[1] = sc->videv.mask_1;
    vi_dev_attr.enScanMode = sc->videv.scan_mode;
    vi_dev_attr.s32AdChnId[0] = -1;
    vi_dev_attr.s32AdChnId[1] = -1;
    vi_dev_attr.s32AdChnId[2] = -1;
    vi_dev_attr.s32AdChnId[3] = -1;
    vi_dev_attr.enDataSeq = sc->videv.data_seq;

    vi_dev_attr.stSynCfg.enVsync = sc->videv.vsync;
    vi_dev_attr.stSynCfg.enVsyncNeg = sc->videv.vsync_neg;
    vi_dev_attr.stSynCfg.enHsync = sc->videv.hsync;
    vi_dev_attr.stSynCfg.enHsyncNeg = sc->videv.hsync_neg;
    vi_dev_attr.stSynCfg.enVsyncValid = sc->videv.vsync_valid;
    vi_dev_attr.stSynCfg.enVsyncValidNeg = sc->videv.vsync_valid_neg;
    vi_dev_attr.stSynCfg.stTimingBlank.u32HsyncHfb = sc->videv.timing_blank_hsync_hfb;
    vi_dev_attr.stSynCfg.stTimingBlank.u32HsyncAct = sc->videv.timing_blank_hsync_act;
    vi_dev_attr.stSynCfg.stTimingBlank.u32HsyncHbb = sc->videv.timing_blank_hsync_hbb;
    vi_dev_attr.stSynCfg.stTimingBlank.u32VsyncVfb = sc->videv.timing_blank_vsync_vfb;
    vi_dev_attr.stSynCfg.stTimingBlank.u32VsyncVact = sc->videv.timing_blank_vsync_vact;
    vi_dev_attr.stSynCfg.stTimingBlank.u32VsyncVbb = sc->videv.timing_blank_vsync_vbb;
    vi_dev_attr.stSynCfg.stTimingBlank.u32VsyncVbfb = sc->videv.timing_blank_vsync_vbfb;
    vi_dev_attr.stSynCfg.stTimingBlank.u32VsyncVbact = sc->videv.timing_blank_vsync_vbact;
    vi_dev_attr.stSynCfg.stTimingBlank.u32VsyncVbbb = sc->videv.timing_blank_vsync_vbbb;
    vi_dev_attr.enDataPath = sc->videv.data_path;
    vi_dev_attr.enInputDataType = sc->videv.input_data_type;
    vi_dev_attr.bDataRev = sc->videv.data_rev;

#ifdef ISP_EX_ATTR
    int s32Ret = HI_MPI_VI_SetDevAttrEx(0, &vi_dev_attr);
#else
    int s32Ret = HI_MPI_VI_SetDevAttr(0, &vi_dev_attr);
#endif
    if (HI_SUCCESS != s32Ret)
    {
#ifdef ISP_EX_ATTR
        log_error("HI_MPI_VI_SetDevAttrEx failed with %#x!", s32Ret);
#else
        log_error("HI_MPI_VI_SetDevAttr failed with %#x!", s32Ret);
#endif
        return -1;
    }

    s32Ret = HI_MPI_VI_EnableDev(0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VI_EnableDev failed with %#x!", s32Ret);
        return -1;
    }

    VI_CHN_ATTR_S chn_attr;
    memset(&chn_attr, 0, sizeof(VI_CHN_ATTR_S));
    chn_attr.stCapRect.s32X = sc->vichn.cap_rect_x;
    chn_attr.stCapRect.s32Y = sc->vichn.cap_rect_y;
    chn_attr.stCapRect.u32Width = sc->vichn.cap_rect_width;
    chn_attr.stCapRect.u32Height = sc->vichn.cap_rect_height;
    chn_attr.stDestSize.u32Width = sc->vichn.dest_size_width;
    chn_attr.stDestSize.u32Height = sc->vichn.dest_size_height;
    chn_attr.enCapSel = sc->vichn.cap_sel;
    chn_attr.enPixFormat = sc->vichn.pix_format;
    chn_attr.bMirror = HI_FALSE;
    chn_attr.bFlip = HI_FALSE;
    chn_attr.s32SrcFrameRate = 30;
    chn_attr.bChromaResample = HI_FALSE;
    chn_attr.s32FrameRate = 30;
    s32Ret = HI_MPI_VI_SetChnAttr(0, &chn_attr);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VI_SetChnAttr failed with %#x!", s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_VI_EnableChn(0);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VI_EnableChn failed with %#x!", s32Ret);
        return -1;
    }

    if (sc->viext.width > 0 && sc->viext.height > 0)
    {
        log_info("starting ext vi chnl 1: %ux%u, PixFormat = %s(%d), src_fr/fr = %d/%d", sc->viext.width, sc->viext.height, cfg_sensor_vals_vichn_pixel_format[sc->viext.pix_format], sc->viext.pix_format, sc->viext.src_frame_rate, sc->viext.frame_rate);

        VI_EXT_CHN_ATTR_S stViExtChnAttr;
        stViExtChnAttr.s32BindChn           = 0;
        stViExtChnAttr.stDestSize.u32Width  = sc->viext.width;
        stViExtChnAttr.stDestSize.u32Height = sc->viext.height;
        stViExtChnAttr.s32SrcFrameRate      = sc->viext.src_frame_rate;
        stViExtChnAttr.s32FrameRate         = sc->viext.frame_rate;
        stViExtChnAttr.enPixFormat          = sc->viext.pix_format;

        s32Ret = HI_MPI_VI_SetExtChnAttr(1, &stViExtChnAttr);
        if (HI_SUCCESS != s32Ret) {
            log_error("HI_MPI_VI_SetExtChnAttr failed with %#x!", s32Ret);
            return -1;
        }

        s32Ret = HI_MPI_VI_EnableChn(1);
        if (HI_SUCCESS != s32Ret) {
            log_error("HI_MPI_VI_SetExtChnAttr failed with %#x!", s32Ret);
            return -1;
        }
    }

    return 0;
}

int sdk_vpss_init(const struct SensorConfig* sc)
{
    VPSS_GRP_ATTR_S vpss_grp_attr;
    memset(&vpss_grp_attr, 0 ,sizeof(VPSS_GRP_ATTR_S));
    vpss_grp_attr.u32MaxW = sc->vichn.dest_size_width;
    vpss_grp_attr.u32MaxH = sc->vichn.dest_size_height;
    vpss_grp_attr.enPixFmt = sc->vichn.pix_format;
    vpss_grp_attr.bIeEn = HI_TRUE;
    vpss_grp_attr.bDrEn = HI_TRUE;
    vpss_grp_attr.bDbEn = HI_TRUE;
    vpss_grp_attr.bHistEn = HI_TRUE;
    vpss_grp_attr.enDieMode = VPSS_DIE_MODE_NODIE;
    vpss_grp_attr.bNrEn = HI_TRUE;
    log_info("VPSS params %u %u %u\n", sc->vichn.dest_size_width, sc->vichn.dest_size_height, sc->vichn.pix_format);

    int s32Ret = HI_MPI_VPSS_CreateGrp(0, &vpss_grp_attr);
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_VPSS_CreateGrp failed with %#x!", s32Ret);
        return -1;
    }

    VPSS_GRP_PARAM_S stVpssParam;
    HI_MPI_VPSS_GetGrpParam(0, &stVpssParam);

    stVpssParam.u32MotionThresh = 0;
    stVpssParam.u32SfStrength = 128;
    stVpssParam.u32TfStrength = 32;
    stVpssParam.u32ChromaRange = 32;
    s32Ret = HI_MPI_VPSS_SetGrpParam(0, &stVpssParam);
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_VPSS_SetGrpParam failed with %#x!", s32Ret);
        return -1;
    }

    s32Ret = HI_MPI_VPSS_StartGrp(0);
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_VPSS_StartGrp failed with %#x!", s32Ret);
        return -1;
    }

    log_info("VPSS bind start");
    for (unsigned chi = 0; chi <= 2; ++chi)
    {
        log_info("EnableChn(0, %u)\n", chi);
        s32Ret = HI_MPI_VPSS_EnableChn(0, chi);
        if (HI_SUCCESS != s32Ret)
        {
            log_error("HI_MPI_VPSS_EnableChn for VPSS(0:%u) failed with %#x!", chi, s32Ret);
            return -1;
        }
    }

    MPP_CHN_S src_chn = {
        .enModId = HI_ID_VIU,
        .s32DevId = 0,
        .s32ChnId = 0,  // ViChn
    };
    MPP_CHN_S dest_chn = {
        .enModId = HI_ID_VPSS,
        .s32DevId = 0,
        .s32ChnId = 0,
    };
    s32Ret = HI_MPI_SYS_Bind(&src_chn, &dest_chn);
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_SYS_Bind VI(0:0) to VPSS(0:0) failed with %#x!", s32Ret);
        return -1;
    }
    log_info("VPSS bind done");
}

void sdk_vi_done()
{
    
}


