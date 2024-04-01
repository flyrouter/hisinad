#ifndef __hisinad_vda_config_h_
#define __hisinad_vda_config_h_

struct vda_cfg_vi_chn_s
{
    unsigned vi_chnl;
    unsigned width;
    unsigned height;
};

struct snap_s
{
    unsigned vpss_chnl_id;
    unsigned grp_id;
    unsigned venc_chnl_id;

    unsigned width;
    unsigned height;
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
    struct snap_s snap;
    struct vda_md_s md;
};

int vda_cfg_read(const char* fname, struct vda_cfg_s* vc);
const char* vda_cfg_read_error_key();
const char* vda_cfg_read_error_value();

#endif // __hisinad_vda_config_h_

