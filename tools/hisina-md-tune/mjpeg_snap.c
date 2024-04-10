#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ev.h>

#include <signal.h>
#include <unistd.h>
#include <aux/logger.h>
#include <hitiny/hitiny_sys.h>
#include <hitiny/hitiny_venc.h>
#include <stdint.h>
#include <cfg/vdacfg.h>

#define CHECK_VALUE_BETWEEN(v, s, m, M) do                                                          \
                                        {                                                           \
                                            if (((v)<m) || ((v)>M))                                 \
                                            {                                                       \
                                                log_error("%s should be in [%u, %u]", s, m, M);     \
                                                return -1;                                          \
                                            }                                                       \
                                        } while(0);

#define TMP_VALUES_BUFFER_SZ 1024

static int init_snap_machine_checkcfg(const struct vda_cfg_s* vc)
{
    CHECK_VALUE_BETWEEN(vc->mjpeg_snap.width, "mjpeg snap width", 160, 1280)
    CHECK_VALUE_BETWEEN(vc->mjpeg_snap.height, "mjpeg snap height", 90, 960)

    if (vc->mjpeg_snap.vpss_chn == VPSS_CHN_UNSET)
    {
        char buf[TMP_VALUES_BUFFER_SZ];
        unsigned buf_used = 0;

        const char** ptr = cfg_daemon_vals_vpss_chn;
        while (*ptr)
        {   
            buf_used += snprintf(buf + buf_used, TMP_VALUES_BUFFER_SZ - buf_used, "%s%s", buf_used ? ", " : "", *ptr);
            ptr++;
        }

        log_crit("snap vpss_chn is not set, possible values: %s", buf);
        
        return -1;
    }
    return 0;
}

int mjpeg_snap_init(const struct vda_cfg_s* vc)
{
    int ret = init_snap_machine_checkcfg(vc);

    if (ret) return ret;

    if (vc->mjpeg_snap.grp_id == 0)
    {
        log_warn("grp_id is 0, but we can start this...");
    }

    hitiny_MPI_VENC_DestroyGroup(vc->mjpeg_snap.grp_id);
    int s32Ret = hitiny_MPI_VENC_CreateGroup(vc->mjpeg_snap.grp_id);
    if (HI_SUCCESS != s32Ret)
    {
        log_error("HI_MPI_VENC_CreateGroup failed with %#x!", s32Ret);
        return s32Ret;
    }

    s32Ret = hitiny_sys_bind_VPSS_GROUP(0, (int)vc->mjpeg_snap.vpss_chn, vc->mjpeg_snap.grp_id);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_SYS_Bind VPSS to GROUP failed with %#x!", s32Ret);
        hitiny_MPI_VENC_DestroyGroup(vc->mjpeg_snap.grp_id);
        return s32Ret;
    }

    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_ATTR_MJPEG_S stMJpegAttr;

    memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    memset(&stMJpegAttr, 0, sizeof(VENC_ATTR_MJPEG_S));

    stVencChnAttr.stVeAttr.enType = PT_MJPEG;

    stMJpegAttr.u32MaxPicWidth  = vc->mjpeg_snap.width;
    stMJpegAttr.u32MaxPicHeight = vc->mjpeg_snap.width;
    stMJpegAttr.u32PicWidth  = vc->mjpeg_snap.width;
    stMJpegAttr.u32PicHeight = vc->mjpeg_snap.width;
    stMJpegAttr.u32BufSize = vc->mjpeg_snap.width * vc->mjpeg_snap.height * 2;
    stMJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
    stMJpegAttr.bMainStream = HI_TRUE;
    stMJpegAttr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame?*/
    memcpy(&stVencChnAttr.stVeAttr.stAttrMjpeg, &stMJpegAttr, sizeof(VENC_ATTR_MJPEG_S));

    stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGVBR;
    stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32StatTime = vc->mjpeg_snap.vbr_stat_time;
    stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32ViFrmRate = vc->mjpeg_snap.vbr_vi_frm_rate;
    stVencChnAttr.stRcAttr.stAttrMjpegeVbr.fr32TargetFrmRate = vc->mjpeg_snap.vbr_target_frm_rate;
    stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxBitRate = vc->mjpeg_snap.vbr_Max_bitrate;
    stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MaxQfactor = vc->mjpeg_snap.vbr_Max_Qfactor;
    stVencChnAttr.stRcAttr.stAttrMjpegeVbr.u32MinQfactor = vc->mjpeg_snap.vbr_Min_Qfactor;

    s32Ret = hitiny_MPI_VENC_CreateChn(vc->mjpeg_snap.venc_chn_id, &stVencChnAttr);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_CreateChn failed with %#x!", s32Ret);
        hitiny_sys_unbind_VPSS_GROUP(0, (int)vc->mjpeg_snap.vpss_chn, vc->mjpeg_snap.grp_id);
        hitiny_MPI_VENC_DestroyGroup(vc->mjpeg_snap.grp_id);
        return s32Ret;
    }

    s32Ret = hitiny_MPI_VENC_RegisterChn(vc->mjpeg_snap.grp_id, vc->mjpeg_snap.venc_chn_id);
    if (HI_SUCCESS != s32Ret) {
        log_error("HI_MPI_VENC_RegisterChn failed with %#x!", s32Ret);
        hitiny_MPI_VENC_DestroyChn(vc->mjpeg_snap.venc_chn_id);
        hitiny_sys_unbind_VPSS_GROUP(0, (int)vc->mjpeg_snap.vpss_chn, vc->mjpeg_snap.grp_id);
        hitiny_MPI_VENC_DestroyGroup(vc->mjpeg_snap.grp_id);
        return s32Ret;
    }

    return 0;
}

void mjpeg_snap_done(const struct vda_cfg_s* vc)
{
    hitiny_MPI_VENC_UnRegisterChn(vc->mjpeg_snap.venc_chn_id);
    hitiny_MPI_VENC_DestroyChn(vc->mjpeg_snap.venc_chn_id);
    hitiny_sys_unbind_VPSS_GROUP(0, (int)vc->mjpeg_snap.vpss_chn, vc->mjpeg_snap.grp_id);
    hitiny_MPI_VENC_DestroyGroup(vc->mjpeg_snap.grp_id);
}

