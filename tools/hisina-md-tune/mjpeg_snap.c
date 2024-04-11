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
#include <evcurl/evcurl.h>


/* stream buffers */

#define BUFFERS_TOTAL_COUNT 8

struct stream_buffer_t
{
    void* ptr;
    unsigned pos;
    unsigned sz;
    unsigned sz_max;
};

static struct stream_buffer_t __upload_buffers[BUFFERS_TOTAL_COUNT] = { 0 };
static unsigned __upload_buffers_cur_read = 0;
static unsigned __upload_buffers_cur_write = 0;

struct stream_buffer_t* __upload_buffers_get_write(unsigned sz)
{
    unsigned __new_cur_write = (__upload_buffers_cur_write + 1) % BUFFERS_TOTAL_COUNT;
    if (__new_cur_write == __upload_buffers_cur_read) return 0;

    struct stream_buffer_t* ret = &__upload_buffers[__upload_buffers_cur_write];

    if (ret->ptr && (ret->sz_max < sz))
    {
        //log_info("too small (sz_max = %u), need %u", ret->sz_max, sz);
        free(ret->ptr);
        ret->ptr = 0;
        ret->pos = 0;
        ret->sz = 0;
        ret->sz_max = 0;
    }

    if (!ret->ptr)
    {
        ret->sz_max = (sz & 0xFFFFF000) + 0x1000;
        //log_info("malloc %u, needs %u", ret->sz_max, sz);
        ret->ptr = malloc(ret->sz_max);
        ret->pos = 0;
        ret->sz = 0;
    }

    ret->pos = 0;
    ret->sz = 0;

    __upload_buffers_cur_write = __new_cur_write;

    return ret;
}

struct stream_buffer_t* __upload_buffers_get_read()
{
    struct stream_buffer_t* ret = &__upload_buffers[__upload_buffers_cur_read];

    //log_info("at curr read buf pos/sz %u/%u", ret->pos, ret->sz);
    if (ret->pos < ret->sz) return ret;

    if (__upload_buffers_cur_read == __upload_buffers_cur_write) return 0;

    __upload_buffers_cur_read = (__upload_buffers_cur_read + 1) % BUFFERS_TOTAL_COUNT;
    ret = &__upload_buffers[__upload_buffers_cur_read];
    if (ret->pos < ret->sz) return ret;

    return 0;
}

/* libcurl uploader */

extern evcurl_processor_t* g_evcurl_proc;

CURL *g_uploader_easy_handler = 0;

static size_t xxxREADER(char *buffer, size_t size, size_t nitems, void *userdata)
{
    struct stream_buffer_t* data_buf = __upload_buffers_get_read();
    
    if (!data_buf || !data_buf->sz)
    {
        //log_info("nothing to UPLOAD...");
        if (!g_uploader_easy_handler) return 0;

        return CURL_READFUNC_PAUSE;
    }

    // fake upload
    //log_info("pos of sz: %u of %u", data_buf->pos, data_buf->sz);
    unsigned sz_to_send = size * nitems;
    unsigned sz_left = data_buf->sz - data_buf->pos;

    if (sz_to_send > sz_left) sz_to_send = sz_left;

    memcpy(buffer, data_buf->ptr + data_buf->pos, sz_to_send);
    data_buf->pos += sz_to_send;

    //snprintf(buffer, 32, "fake %u upload of %u\n", sz_to_send, data_buf->sz);
    //log_info("SENDING '%s'", buffer);
    return sz_to_send;
}

static void PUT_mjpeg_snap_req_end_cb(evcurl_req_result_t* res, void* src_req_data)
{
    g_uploader_easy_handler = 0;

    log_info("PUT mjpeg snap Req DONE: %d", res->result);
    evcurl_upload_chunked_req_t *put_req = (evcurl_upload_chunked_req_t*)src_req_data;
    log_info("    effective URL: %s", res->effective_url);
    log_info("    %ld", res->response_code);
    log_info("    Content-Type: %s", res->content_type ? res->content_type : "(none)");

    if (!res->sz_body) return;

    log_info("\n\n%.*s\n--------------------------------------------------------------", (int)res->sz_body, (const char*)res->body);

    free(put_req);
}

/* config and init */

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

static ev_io venc_mjpeg_snap_ev_io;
static int venc_mjpeg_snap_chn = -1;

static void __venc_mjpeg_snap_cb(struct ev_loop *loop, ev_io* _w, int revents)
{
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;

    int ret = hitiny_MPI_VENC_Query(venc_mjpeg_snap_chn, &stStat);
    if (ret)
    {
        log_error("hitiny_MPI_VENC_Query error 0x%x", ret);
        hitiny_MPI_VENC_StopRecvPic(venc_mjpeg_snap_chn);
        return;
    }

    stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
    stStream.u32PackCount = stStat.u32CurPacks;
    ret = hitiny_MPI_VENC_GetStream(venc_mjpeg_snap_chn, &stStream, HI_FALSE);

    if (ret)
    {
        log_warn("hitiny_MPI_VENC_GetStream error 0x%x", ret);
    }
    else
    {
        unsigned sz = 0;
        for (unsigned iii = 0; iii < stStream.u32PackCount; iii++)
        {
            VENC_PACK_S* pstData = &(stStream.pstPack[iii]);
            sz += pstData->u32Len[0] + pstData->u32Len[1];
        }

        log_info("UPLOAD %u bytes", sz);
        struct stream_buffer_t* copy_buf = __upload_buffers_get_write(sz);
        if (copy_buf)
        {
            void* ptr = copy_buf->ptr;

            for (unsigned iii = 0; iii < stStream.u32PackCount; iii++)
            {
                VENC_PACK_S* pstData = &(stStream.pstPack[iii]);
                memcpy(ptr, pstData->pu8Addr[0], pstData->u32Len[0]);
                ptr += pstData->u32Len[0];
                memcpy(ptr, pstData->pu8Addr[1], pstData->u32Len[1]);
                ptr += pstData->u32Len[1];
            }

            copy_buf->sz = sz;
        }
        else
        {
            log_info("NO BUF to store mjpeg");
        }

        hitiny_MPI_VENC_ReleaseStream(venc_mjpeg_snap_chn, &stStream);
    }

    free(stStream.pstPack);
    stStream.pstPack = NULL;

    if (g_uploader_easy_handler)
    {
        curl_easy_pause(g_uploader_easy_handler, CURLPAUSE_CONT);
    }
}

int mjpeg_snap_init(const struct vda_cfg_s* vc, struct ev_loop* loop)
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
    stMJpegAttr.u32MaxPicHeight = vc->mjpeg_snap.height;
    stMJpegAttr.u32PicWidth  = vc->mjpeg_snap.width;
    stMJpegAttr.u32PicHeight = vc->mjpeg_snap.height;
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

    int fd = hitiny_MPI_VENC_GetFd(vc->mjpeg_snap.venc_chn_id);
    if (fd < 0)
    {
        log_error("HI_MPI_VENC_GetFd failed with %#x!\n", s32Ret);
        hitiny_MPI_VENC_UnRegisterChn(vc->mjpeg_snap.venc_chn_id);
        hitiny_MPI_VENC_DestroyChn(vc->mjpeg_snap.venc_chn_id);
        hitiny_sys_unbind_VPSS_GROUP(0, (int)vc->mjpeg_snap.vpss_chn, vc->mjpeg_snap.grp_id);
        hitiny_MPI_VENC_DestroyGroup(vc->mjpeg_snap.grp_id);
        return fd;
    }

    ev_io_init(&venc_mjpeg_snap_ev_io, __venc_mjpeg_snap_cb, fd, EV_READ);
    ev_io_start(loop, &venc_mjpeg_snap_ev_io);

    venc_mjpeg_snap_chn = vc->mjpeg_snap.venc_chn_id;

    return 0;
}

void mjpeg_snap_start_record()
{
    hitiny_MPI_VENC_StartRecvPic(venc_mjpeg_snap_chn);

    evcurl_upload_chunked_req_t *uploader = (evcurl_upload_chunked_req_t*)malloc(sizeof(evcurl_upload_chunked_req_t));
    memset(uploader, 0, sizeof(evcurl_upload_chunked_req_t));

    uploader->timeout = 5;
    uploader->connect_timeout = 2;

    uploader->url = "";
    uploader->read_cb = xxxREADER;

    evcurl_new_UPLOAD_chunked(g_evcurl_proc, uploader, PUT_mjpeg_snap_req_end_cb, &g_uploader_easy_handler);
}

void mjpeg_snap_stop_record()
{
    hitiny_MPI_VENC_StopRecvPic(venc_mjpeg_snap_chn);

    if (g_uploader_easy_handler)
    {
        curl_easy_pause(g_uploader_easy_handler, CURLPAUSE_CONT);
        g_uploader_easy_handler = 0;
    }
}

void mjpeg_snap_done(const struct vda_cfg_s* vc)
{
    hitiny_MPI_VENC_StopRecvPic(vc->mjpeg_snap.venc_chn_id);
    hitiny_MPI_VENC_UnRegisterChn(vc->mjpeg_snap.venc_chn_id);
    hitiny_MPI_VENC_DestroyChn(vc->mjpeg_snap.venc_chn_id);
    hitiny_sys_unbind_VPSS_GROUP(0, (int)vc->mjpeg_snap.vpss_chn, vc->mjpeg_snap.grp_id);
    hitiny_MPI_VENC_DestroyGroup(vc->mjpeg_snap.grp_id);
    venc_mjpeg_snap_chn = -1;
}

