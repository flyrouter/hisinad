#include "vdacfg.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <cfg/common.h>


#define SECTION_vi          1
#define SECTION_mjpeg_snap  5
#define SECTION_md          10

static int __vda_cfg_current_section = 0;
static char __vda_cfg_error_key[32] = { 0 };
static char __vda_cfg_error_value[256] = { 0 };

#define ADD_SECTION(section) if (strcmp(s, #section) == 0) { __vda_cfg_current_section = SECTION_##section; return CFG_PROC_OK; }

static int __vda_cfg_read_section_cb(const char* s)
{
    ADD_SECTION(vi)
    ADD_SECTION(mjpeg_snap)
    ADD_SECTION(md)

    snprintf(__vda_cfg_error_value, 256, "%s", s);
    return CFG_PROC_WRONG_SECTION;
}

static struct vda_cfg_s* __current_vc;
#define KEYVAL_PARAM_COPY_STR(key, dest, MAX) if (strcasecmp(key, k) == 0) { snprintf(dest, MAX, "%s", v); return CFG_PROC_OK; }
#define KEYVAL_PARAM_IGN(key) if (strcasecmp(key, k) == 0) { return CFG_PROC_OK; }

#define KEYVAL_PARAM_SL_dec(key, dest) if (strcasecmp(key, k) == 0) {   \
        char* endptr;                                                   \
        dest = strtol(v, &endptr, 10);                                  \
        if (!*endptr) return CFG_PROC_OK;                               \
        snprintf(__vda_cfg_error_key, 256, "%s", k);                    \
        snprintf(__vda_cfg_error_value, 256, "%s", v);                  \
        return CFG_PROC_VALUE_BAD;                                      \
    }

#define KEYVAL_PARAM_UL_dec(key, dest) if (strcasecmp(key, k) == 0) {   \
        char* endptr;                                                   \
        dest = strtoul(v, &endptr, 10);                                 \
        if (!*endptr) return CFG_PROC_OK;                               \
        snprintf(__vda_cfg_error_key, 256, "%s", k);                    \
        snprintf(__vda_cfg_error_value, 256, "%s", v);                  \
        return CFG_PROC_VALUE_BAD;                                      \
    }

#define KEYVAL_PARAM_ENUM(key, dest, pvs) if (strcasecmp(key, k) == 0) {        \
        unsigned n = 0;                                                         \
        while (pvs[n]) {                                                        \
            if (strcmp(pvs[n], v) == 0) {                                       \
                dest = n; return CFG_PROC_OK;                                   \
            }                                                                   \
            n++;                                                                \
        }                                                                       \
        char* endptr;                                                           \
        unsigned x = strtoul(v, &endptr, 10);                                   \
        if (!*endptr && (n > x)) {                                              \
            dest = x; return CFG_PROC_OK;                                       \
        }                                                                       \
        snprintf(__vda_cfg_error_key, 256, "%s", k);                            \
        snprintf(__vda_cfg_error_value, 256, "%s", v);                          \
        return CFG_PROC_VALUE_BAD;                                              \
    }


static int __vda_cfg_read_keyval_cb_vi(const char* k, const char* v)
{
    KEYVAL_PARAM_UL_dec("vi_chnl", __current_vc->vi.vi_chnl);

    KEYVAL_PARAM_UL_dec("width", __current_vc->vi.width);
    KEYVAL_PARAM_UL_dec("height", __current_vc->vi.height);

    snprintf(__vda_cfg_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

const char *cfg_daemon_vals_vpss_chn[] = { "VPSS_CHN_CHN0", "VPSS_CHN_CHN1", "VPSS_CHN_BYPASS", 0 };

static int __vda_cfg_read_keyval_cb_mjpeg_snap(const char* k, const char* v)
{
    KEYVAL_PARAM_ENUM("vpss_chn", __current_vc->mjpeg_snap.vpss_chn, cfg_daemon_vals_vpss_chn);
    KEYVAL_PARAM_UL_dec("grp_id", __current_vc->mjpeg_snap.grp_id);
    KEYVAL_PARAM_UL_dec("venc_chn_id", __current_vc->mjpeg_snap.venc_chn_id);

    KEYVAL_PARAM_UL_dec("width", __current_vc->mjpeg_snap.width);
    KEYVAL_PARAM_UL_dec("height", __current_vc->mjpeg_snap.height);

    KEYVAL_PARAM_UL_dec("vbr_stat_time", __current_vc->mjpeg_snap.vbr_stat_time);
    KEYVAL_PARAM_UL_dec("vbr_vi_frm_rate", __current_vc->mjpeg_snap.vbr_vi_frm_rate);
    KEYVAL_PARAM_UL_dec("vbr_target_frm_rate", __current_vc->mjpeg_snap.vbr_target_frm_rate);
    KEYVAL_PARAM_UL_dec("vbr_Max_bitrate", __current_vc->mjpeg_snap.vbr_Max_bitrate);
    KEYVAL_PARAM_UL_dec("vbr_Max_Qfactor", __current_vc->mjpeg_snap.vbr_Max_Qfactor);
    KEYVAL_PARAM_UL_dec("vbr_Min_Qfactor", __current_vc->mjpeg_snap.vbr_Min_Qfactor);

    KEYVAL_PARAM_COPY_STR("upload_url", __current_vc->mjpeg_snap.upload_url, 256);
    KEYVAL_PARAM_COPY_STR("upload_username", __current_vc->mjpeg_snap.upload_username, 64);
    KEYVAL_PARAM_COPY_STR("upload_password", __current_vc->mjpeg_snap.upload_password, 64);

    snprintf(__vda_cfg_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

static int __vda_cfg_read_keyval_cb_md(const char* k, const char* v)
{
    KEYVAL_PARAM_UL_dec("VdaChn", __current_vc->md.VdaChn);
    KEYVAL_PARAM_UL_dec("VdaIntvl", __current_vc->md.VdaIntvl);
    KEYVAL_PARAM_UL_dec("SadTh", __current_vc->md.SadTh);
    KEYVAL_PARAM_UL_dec("ObjNumMax", __current_vc->md.ObjNumMax);

    snprintf(__vda_cfg_error_key, 256, "%s", k);
    return CFG_PROC_KEY_BAD;
}

#define KEYVAL_CASE(section) case SECTION_##section: { return __vda_cfg_read_keyval_cb_##section(k, v); }

static int __vda_cfg_read_keyval_cb(const char* k, const char* v)
{
    switch (__vda_cfg_current_section)
    {
        KEYVAL_CASE(vi)
        KEYVAL_CASE(mjpeg_snap)
        KEYVAL_CASE(md)
    }
    return CFG_PROC_OK;
}

int vda_cfg_read(const char* fname, struct vda_cfg_s* vc)
{
    __vda_cfg_current_section = 0;
    __current_vc = vc;

    memset(vc, 0, sizeof(struct vda_cfg_s));

    vc->mjpeg_snap.vpss_chn = VPSS_CHN_UNSET;

    return cfg_proc_read(fname, __vda_cfg_read_section_cb, __vda_cfg_read_keyval_cb);
}

const char* vda_cfg_read_error_key()
{
    return __vda_cfg_error_key;
}

const char* vda_cfg_read_error_value()
{
    return __vda_cfg_error_value;
}

