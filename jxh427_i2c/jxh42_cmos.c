#if !defined(__JXH42_CMOS_H_)
#define __JXH42_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hi_comm_sns.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_af.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define FULL_LINES_DEFAULT 750

/*

PROGRESS:
    cmos_get_ae_default
    cmos_set_wdr_mode
    cmos_get_isp_black_level


*/

#define JXH42_ID 42

HI_U32 gu32FullLinesStd = FULL_LINES_DEFAULT;
HI_U32 gu32FullLines = FULL_LINES_DEFAULT;

HI_U8 gu8SensorMode = 0; // which WDR mode default?


HI_VOID cmos_set_wdr_mode(HI_U8 u8Mode)
{
    fprintf(stderr, "DBG: cmos_set_wdr_mode(%d)\n", u8Mode);
    if (u8Mode >= 2)
    {
        fprintf(stderr, "NO support for this mode!");
    }
    else
    {
        gu8SensorMode = u8Mode;
    }
}

static HI_S32 cmos_get_ae_default(AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    fprintf(stderr, "DBG: cmos_get_ae_default(*)\n");
    if (HI_NULL == pstAeSnsDft)
    {
        printf("null pointer when get ae default value!\n");
        return -1;
    }

    gu32FullLinesStd = FULL_LINES_DEFAULT;

    pstAeSnsDft->au8HistThresh[0] = 0xd;
    pstAeSnsDft->au8HistThresh[1] = 0x28;
    pstAeSnsDft->au8HistThresh[2] = 0x60;
    pstAeSnsDft->au8HistThresh[3] = 0x80;

    pstAeSnsDft->u8AeCompensation = 0x40;

    pstAeSnsDft->u32LinesPer500ms = 11265; // FULL_LINES_DEFAULT * FPS30 / 2
    pstAeSnsDft->u32FullLinesStd = gu32FullLinesStd;
    pstAeSnsDft->u32FlickerFreq = 0;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->u32MaxIntTime = 747; // ??? FULL_LINES_DEFAULT - 1;
    pstAeSnsDft->u32MinIntTime = 8;
    pstAeSnsDft->u32MaxIntTimeTarget = 65535;
    pstAeSnsDft->u32MinIntTimeTarget = 2;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_DB;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 6.0;
    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 0.0625;

    pstAeSnsDft->u32MaxAgain = 3;
    pstAeSnsDft->u32MinAgain = 0;
    pstAeSnsDft->u32MaxDgain = 31;
    pstAeSnsDft->u32MinDgain = 16;

    pstAeSnsDft->u32MaxDgainTarget = 31;
    pstAeSnsDft->u32MinDgainTarget = 16;
    pstAeSnsDft->u32MaxAgainTarget = 3;
    pstAeSnsDft->u32MinAgainTarget = 0;

    pstAeSnsDft->u32ISPDgainShift = 8;
    pstAeSnsDft->u32MaxISPDgainTarget = 2 << pstAeSnsDft->u32ISPDgainShift; // 512
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift; // 256

    return 0;
}

HI_U32 cmos_get_isp_black_level(ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
    HI_S32  i;

    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    fprintf(stderr, "DBG: cmos_get_isp_black_level(*)\n");

    pstBlackLevel->bUpdate = HI_FALSE;
    pstBlackLevel->au16BlackLevel[0] = 0x14;
    pstBlackLevel->au16BlackLevel[1] = 0x14;
    pstBlackLevel->au16BlackLevel[2] = 0x14;
    pstBlackLevel->au16BlackLevel[3] = 0x14;
 
    return 0;
}

static HI_VOID cmos_inttime_update(HI_U32 u32IntTime)
{
    fprintf(stderr, "DBG: cmos_inttime_update(%u)\n", u32IntTime);
    // TODO large work
}

void sensor_global_init()
{
    gu8SensorMode = 0;
}

static ISP_CMOS_NOISE_TABLE_S g_stIspNoiseTable =
{
    /* bvalid */
    1,

    //nosie_profile_weight_lut
   {
        0x19, 0x20, 0x24, 0x27, 0x29, 0x2B, 0x2D, 0x2E, 0x2F, 0x31, 0x32, 0x33, 0x34, 0x34, 0x35, 0x36,
        0x37, 0x37, 0x38, 0x38, 0x39, 0x3A, 0x3A, 0x3B, 0x3B, 0x3B, 0x3C, 0x3C, 0x3D, 0x3D, 0x3D, 0x3E,
        0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x42, 0x42,
        0x43, 0x43, 0x43, 0x43, 0x44, 0x44, 0x44, 0x44, 0x44, 0x45, 0x45, 0x45, 0x45, 0x45, 0x46, 0x46,
        0x46, 0x46, 0x46, 0x46, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,
        0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A,
        0x4A, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4E, 0x4E, 0x4E
   },

    //demosaic_weight_lut
   {
        0x19, 0x20, 0x24, 0x27, 0x29, 0x2B, 0x2D, 0x2E, 0x2F, 0x31, 0x32, 0x33, 0x34, 0x34, 0x35, 0x36,
        0x37, 0x37, 0x38, 0x38, 0x39, 0x3A, 0x3A, 0x3B, 0x3B, 0x3B, 0x3C, 0x3C, 0x3D, 0x3D, 0x3D, 0x3E,
        0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x42, 0x42,
        0x43, 0x43, 0x43, 0x43, 0x44, 0x44, 0x44, 0x44, 0x44, 0x45, 0x45, 0x45, 0x45, 0x45, 0x46, 0x46,
        0x46, 0x46, 0x46, 0x46, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,
        0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A, 0x4A,
        0x4A, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
        0x4C, 0x4C, 0x4C, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4E, 0x4E, 0x4E
   }

};

static ISP_CMOS_AGC_TABLE_S g_stIspAgcTable =
{
    /* bvalid */
    1,

    /* sharpen_alt_d */
   {0x48, 0x40, 0x38, 0x30, 0x28, 0x20, 0x18, 0x10},

    /* sharpen_alt_ud */
   {0xb0, 0xa0, 0x90, 0x80, 0x70, 0x60, 0x50, 0x40},

    /* snr_thresh    */
   {0x01, 0x09, 0x13, 0x1e, 0x28, 0x32, 0x3c, 0x46},

    /*  demosaic_lum_thresh   */
   {0x40,0x60,0x80,0x80,0x80,0x80,0x80,0x80},

    /* demosaic_np_offset   */
   {0x02, 0x08, 0x12, 0x1a, 0x20, 0x28, 0x30, 0x30},

    /* ge_strength    */
   {0x55,0x55,0x55,0x55,0x55,0x55,0x37,0x37}
};


HI_U32 cmos_get_isp_default(ISP_CMOS_DEFAULT_S *pstDef)
{   
    if (HI_NULL == pstDef)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));

    pstDef->stComm.u8Rggb = 3;
    pstDef->stComm.u8BalanceFe = 1;

    pstDef->stDenoise.u8SinterThresh = 21;
    pstDef->stDenoise.u8NoiseProfile = 0;
    pstDef->stDenoise.u16Nr0 = 0;
    pstDef->stDenoise.u16Nr1 = 0;

    pstDef->stDrc.u8DrcBlack = 0;
    pstDef->stDrc.u8DrcVs = 4;
    pstDef->stDrc.u8DrcVi = 8;
    pstDef->stDrc.u8DrcSm = -96;
    pstDef->stDrc.u16DrcWl = 1279;

    memcpy(&pstDef->stNoiseTbl, &g_stIspNoiseTable, sizeof(ISP_CMOS_NOISE_TABLE_S));
    memcpy(&pstDef->stAgcTbl, &g_stIspAgcTable, sizeof(ISP_CMOS_AGC_TABLE_S));
//    memcpy(&pstDef->stDemosaic, &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));
}

HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init = sensor_init;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
    pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
//    pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_isp_black_level;
//    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
//    pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;
//    pstSensorExpFunc->pfn_cmos_get_sensor_max_resolution = cmos_get_sensor_max_resolution;

    return 0;
}


int sensor_register_callback(void)
{
    HI_S32 s32Ret;
    ALG_LIB_S stLib;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;

    cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret = HI_MPI_ISP_SensorRegCallBack(JXH42_ID, &stIspRegister);
    if (s32Ret)
    {   
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }

/*
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AE_LIB_NAME);
    cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret = HI_MPI_AE_SensorRegCallBack(&stLib, JXH42_ID, &stAeRegister);
    if (s32Ret)
    {   
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AWB_LIB_NAME);
    cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret = HI_MPI_AWB_SensorRegCallBack(&stLib, JXH42_ID, &stAwbRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }
*/

    return 0;
}
    


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif


