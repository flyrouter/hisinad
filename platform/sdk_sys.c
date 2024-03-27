#include "sdk.h"
#include <stdint.h>
#include <string.h>
#include <aux/logger.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "hi_comm_ao.h"
#include "mpi_ao.h"

#define SYS_ALIGN_WIDTH      64

const char* __sdk_last_call = 0;

static int platform_v1_mpi_sys_init(VB_CONF_S* pstVbConf)
{
    HI_S32 s32Ret = HI_FAILURE;

    MPP_SYS_CONF_S stSysConf;

    memset(&stSysConf, 0, sizeof(MPP_SYS_CONF_S));

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    __sdk_last_call = "HI_MPI_VB_SetConf()";
    s32Ret = HI_MPI_VB_SetConf(pstVbConf);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_VB_Init()";
    s32Ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_SYS_SetConf()";
    stSysConf.u32AlignWidth = SYS_ALIGN_WIDTH;

    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_SYS_Init()";
    s32Ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32Ret) return s32Ret;
}

static HI_S32 SYS_GetPicSize(VIDEO_NORM_E enNorm, PIC_SIZE_E enPicSize, SIZE_S *pstSize)
{
    switch (enPicSize)
    {
        case PIC_QCIF:
            pstSize->u32Width = 176;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL==enNorm)?144:120;
            break;
        case PIC_CIF:
            pstSize->u32Width = 352;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL==enNorm)?288:240;
            break;
        case PIC_D1:
            pstSize->u32Width = 720;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL==enNorm)?576:480;
            break;
        case PIC_960H:
            pstSize->u32Width = 960;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL==enNorm)?576:480;
            break;
        case PIC_2CIF:
            pstSize->u32Width = 360;
            pstSize->u32Height = (VIDEO_ENCODING_MODE_PAL==enNorm)?576:480;
            break;
        case PIC_QVGA:    /* 320 * 240 */
            pstSize->u32Width = 320;
            pstSize->u32Height = 240;
            break;
        case PIC_VGA:     /* 640 * 480 */
            pstSize->u32Width = 640;
            pstSize->u32Height = 480;
            break;
        case PIC_XGA:     /* 1024 * 768 */
            pstSize->u32Width = 1024;
            pstSize->u32Height = 768;
            break;
        case PIC_SXGA:    /* 1400 * 1050 */
            pstSize->u32Width = 1400;
            pstSize->u32Height = 1050;
            break;
        case PIC_UXGA:    /* 1600 * 1200 */
            pstSize->u32Width = 1600;
            pstSize->u32Height = 1200;
            break;
        case PIC_QXGA:    /* 2048 * 1536 */
            pstSize->u32Width = 2048;
            pstSize->u32Height = 1536;
            break;
        case PIC_WVGA:    /* 854 * 480 */
            pstSize->u32Width = 854;
            pstSize->u32Height = 480;
            break;
        case PIC_WSXGA:   /* 1680 * 1050 */
            pstSize->u32Width = 1680;
            pstSize->u32Height = 1050;
            break;
        case PIC_WUXGA:   /* 1920 * 1200 */
            pstSize->u32Width = 1920;
            pstSize->u32Height = 1200;
            break;
        case PIC_WQXGA:   /* 2560 * 1600 */
            pstSize->u32Width = 2560;
            pstSize->u32Height = 1600;
            break;
        case PIC_HD720:   /* 1280 * 720 */
            pstSize->u32Width = 1280;
            pstSize->u32Height = 720;
            break;
        case PIC_HD1080:  /* 1920 * 1080 */
            pstSize->u32Width = 1920;
            pstSize->u32Height = 1080;
            break;
        default:
            return HI_FAILURE;
    }
    return HI_SUCCESS;
}

#define VB_HEADER_STRIDE    16

#define VB_PIC_HEADER_SIZE(Width, Height, Type, size)\
    do{\
        if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == Type || PIXEL_FORMAT_RGB_BAYER == Type )\
        {\
            size = VB_HEADER_STRIDE * (Height) * 2;\
        }\
        else if(PIXEL_FORMAT_YUV_SEMIPLANAR_420 == Type)\
        {\
            size = (VB_HEADER_STRIDE * (Height) * 3) >> 1;\
        }\
    }while(0)

static HI_U32 CalcPicVbBlkSize(uint32_t width, uint32_t height, PIXEL_FORMAT_E enPixFmt, HI_U32 u32AlignWidth)
{
    return (CEILING_2_POWER(width, u32AlignWidth) * \
            CEILING_2_POWER(height, u32AlignWidth) * \
           ((PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixFmt)?2:1.5));
}

int sdk_init(const struct SensorConfig* sc)
{
    VB_CONF_S stVbConf;
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    if (sc && sc->vb_cnt)
    {
        stVbConf.u32MaxPoolCnt = 128;

        log_info("VbConf: VbCnt = %u", sc->vb_cnt);

        unsigned vb_pool_i = 0;
        for ( ; vb_pool_i < 1; vb_pool_i++)
        {
            log_info("VbConf: PixFormat = %s(%d)", cfg_sensor_vals_vichn_pixel_format[sc->vichn.pix_format], sc->vichn.pix_format);
            stVbConf.astCommPool[vb_pool_i].u32BlkSize = CalcPicVbBlkSize(sc->isp.isp_w, sc->isp.isp_h, sc->vichn.pix_format, SYS_ALIGN_WIDTH);
            stVbConf.astCommPool[vb_pool_i].u32BlkCnt = sc->vb_cnt;
        }

        if (sc->viext.width > 0 && sc->viext.height > 0)
        {
            log_info("vi ext size is %ux%u, PixFormat = %s(%d)", sc->viext.width, sc->viext.height, cfg_sensor_vals_vichn_pixel_format[sc->viext.pix_format], sc->viext.pix_format);
            stVbConf.astCommPool[vb_pool_i].u32BlkSize = CalcPicVbBlkSize(sc->viext.width, sc->viext.height, sc->viext.pix_format, SYS_ALIGN_WIDTH);
            stVbConf.astCommPool[vb_pool_i].u32BlkCnt = sc->vb_cnt;
            vb_pool_i++;
        }

        /* hist */
        stVbConf.astCommPool[vb_pool_i].u32BlkSize = (196*4);
        stVbConf.astCommPool[vb_pool_i].u32BlkCnt = 6;

        for (unsigned i = 0; i <= vb_pool_i; ++i)
        {
            log_info("VbConf pool %u: %u * %u", i, stVbConf.astCommPool[i].u32BlkSize, stVbConf.astCommPool[i].u32BlkCnt);
        }
    }
    else
    {
        log_info("No video buffers allocated ([vb_conf] section, VbCnt). Only audio?");
    }

    int s32Ret = platform_v1_mpi_sys_init(&stVbConf);

    return s32Ret;
}

void sdk_done()
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
}
