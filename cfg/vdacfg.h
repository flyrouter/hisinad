#ifndef __hisinad_vda_config_h_
#define __hisinad_vda_config_h_

struct vda_cfg_vi_chn_s
{
    unsigned vi_chnl;
    unsigned width;
    unsigned height;
};

enum VPSS_CHN_E
{
    VPSS_CHN_UNSET  = -1,
    VPSS_CHN_CHN0   = 0,
    VPSS_CHN_CHN1   = 1,
    VPSS_CHN_BYPASS = 2,
};

extern const char *cfg_daemon_vals_vpss_chn[];

struct mjpeg_snap_s
{
    enum VPSS_CHN_E vpss_chn;
    unsigned grp_id;
    unsigned venc_chn_id;

    unsigned width;
    unsigned height;

    unsigned vbr_stat_time;
    unsigned vbr_vi_frm_rate;
    unsigned vbr_target_frm_rate;
    unsigned vbr_Max_bitrate;
    unsigned vbr_Max_Qfactor;
    unsigned vbr_Min_Qfactor;
};

struct vda_md_s
{
    unsigned VdaChn;
    unsigned VdaIntvl;
    unsigned SadTh;
    unsigned ObjNumMax;
};

struct vda_cfg_s
{
    struct vda_cfg_vi_chn_s vi;
    struct mjpeg_snap_s mjpeg_snap;
    struct vda_md_s md;
};

int vda_cfg_read(const char* fname, struct vda_cfg_s* vc);
const char* vda_cfg_read_error_key();
const char* vda_cfg_read_error_value();

#endif // __hisinad_vda_config_h_

