#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ev.h>
#include <evcurl/evcurl.h>
#include <signal.h>
#include <unistd.h>
#include <aux/logger.h>
#include <cfg/sensor_config.h>
#include <hitiny/hitiny_sys.h>
#include <hitiny/hitiny_vda.h>
#include <hitiny/hitiny_venc.h>
#include <stdint.h>
#include <cfg/vdacfg.h>

int mjpeg_snap_init(const struct vda_cfg_s* vc, struct ev_loop* loop);
void mjpeg_snap_done(const struct vda_cfg_s* vc);
void mjpeg_snap_start_record();
void mjpeg_snap_stop_record();

evcurl_processor_t* g_evcurl_proc = 0;

int tuner_mode = 0;
int stop_flag = 0;
struct vda_cfg_s vda_cfg = { 0 };

void action_on_signal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

void print_error(int ret)
{
    if (-1 == ret)
    {
        log_error("I/O error, %d: %s", errno, strerror(errno));
    }
    else
    {
        log_error("Error %s(%d) at line %u, pos %u", cfg_proc_err_msg(ret), ret, cfg_proc_err_line_num(), cfg_proc_err_line_pos());
        if (ret == CFG_PROC_WRONG_SECTION)
        {
            log_error("Wrong section name [%s]", vda_cfg_read_error_value());
        }
        else if (ret == CFG_PROC_KEY_BAD)
        {
            log_error("Wrong key \"%s\"", vda_cfg_read_error_key());
        }
        else if (ret == CFG_PROC_VALUE_BAD)
        {
            log_error("Wrong value \"%s\" for %s", vda_cfg_read_error_value(), vda_cfg_read_error_key());
        }
    }
}

void print_md_data(const VDA_DATA_S *pstVdaData)
{
    if (pstVdaData->unData.stMdData.bObjValid && pstVdaData->unData.stMdData.stObjData.u32ObjNum > 0)
    {
        log_info("ObjNum=%d, IndexOfMaxObj=%d, SizeOfMaxObj=%d, SizeOfTotalObj=%d", \
                   pstVdaData->unData.stMdData.stObjData.u32ObjNum, \
             pstVdaData->unData.stMdData.stObjData.u32IndexOfMaxObj, \
             pstVdaData->unData.stMdData.stObjData.u32SizeOfMaxObj,\
             pstVdaData->unData.stMdData.stObjData.u32SizeOfTotalObj);
        for (int i=0; i<pstVdaData->unData.stMdData.stObjData.u32ObjNum; i++)
        {
            VDA_OBJ_S *pstVdaObj = pstVdaData->unData.stMdData.stObjData.pstAddr + i;
            log_info("[%d]\t left=%d, top=%d, right=%d, bottom=%d", i, \
              pstVdaObj->u16Left, pstVdaObj->u16Top, \
              pstVdaObj->u16Right, pstVdaObj->u16Bottom);
        }
    }

    if (!pstVdaData->unData.stMdData.bPelsNumValid)
    {
        log_info("bMbObjValid = FALSE");
    }
    else if (pstVdaData->unData.stMdData.u32AlarmPixCnt)
    {
        log_info("AlarmPixelCount=%d", pstVdaData->unData.stMdData.u32AlarmPixCnt);
    }
}

struct __ev_vda_md_watcher_s
{
    ev_io _ev_io;
    unsigned VdaChn;
};

struct __ev_vda_md_watcher_s* mdw = 0;

static void __md_tuner_cb(struct ev_loop *loop, ev_io* _w, int revents)
{
    struct __ev_vda_md_watcher_s* mdw = (struct __ev_vda_md_watcher_s*)_w;

    VDA_DATA_S stVdaData;
    int s32Ret = hitiny_MPI_VDA_GetData(mdw->VdaChn, &stVdaData, HI_FALSE);

    if (!s32Ret)
    {
        print_md_data(&stVdaData);
    }
    else
    {
        log_error("MPI_VDA_GetData returned error 0x%x", s32Ret);
    }

    hitiny_MPI_VDA_ReleaseData(mdw->VdaChn,&stVdaData);
}

static unsigned mjpeg_running = 0;

static void __md_work_cb(struct ev_loop *loop, ev_io* _w, int revents)
{
    struct __ev_vda_md_watcher_s* mdw = (struct __ev_vda_md_watcher_s*)_w;

    VDA_DATA_S stVdaData;
    int s32Ret = hitiny_MPI_VDA_GetData(mdw->VdaChn, &stVdaData, HI_FALSE);

    if (!s32Ret)
    {
        unsigned obj_cnt = stVdaData.unData.stMdData.bObjValid ? stVdaData.unData.stMdData.stObjData.u32ObjNum : 0;

        log_info("AlarmPixCnt %u, obj %u", stVdaData.unData.stMdData.u32AlarmPixCnt, obj_cnt);

        if (obj_cnt && (stVdaData.unData.stMdData.u32AlarmPixCnt > 100))
        {
            if (!mjpeg_running)
            {
                log_info("AlarmPixelCount=%d, objcnt=%u: MJPEG ON", stVdaData.unData.stMdData.u32AlarmPixCnt, obj_cnt);
                mjpeg_snap_start_record();
            }

            mjpeg_running = vda_cfg.md.VdaIntvl;
        }

        if (!obj_cnt && (stVdaData.unData.stMdData.u32AlarmPixCnt < 30))
        {
            if (mjpeg_running)
            {
                //log_info("going to stop video: %u", mjpeg_running);
                mjpeg_running--;
                if (!mjpeg_running)
                {
                    mjpeg_snap_stop_record();
                    log_info("MJPEG STOP");
                }
            }
        }
    }
    else
    {
        log_error("MPI_VDA_GetData returned error 0x%x", s32Ret);
    }

    hitiny_MPI_VDA_ReleaseData(mdw->VdaChn,&stVdaData);
}

void vda_md_init(struct ev_loop* loop)
{
    VDA_CHN_ATTR_S stVdaChnAttr;

    stVdaChnAttr.enWorkMode = VDA_WORK_MODE_MD;
    stVdaChnAttr.u32Width   = vda_cfg.vi.width;
    stVdaChnAttr.u32Height  = vda_cfg.vi.height;

    stVdaChnAttr.unAttr.stMdAttr.enVdaAlg      = VDA_ALG_REF;
    stVdaChnAttr.unAttr.stMdAttr.enMbSize      = VDA_MB_16PIXEL;
    stVdaChnAttr.unAttr.stMdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
    stVdaChnAttr.unAttr.stMdAttr.enRefMode     = VDA_REF_MODE_DYNAMIC;
    stVdaChnAttr.unAttr.stMdAttr.u32MdBufNum   = 8;
    stVdaChnAttr.unAttr.stMdAttr.u32VdaIntvl   = vda_cfg.md.VdaIntvl;
    stVdaChnAttr.unAttr.stMdAttr.u32BgUpSrcWgt = 128;
    stVdaChnAttr.unAttr.stMdAttr.u32SadTh      = vda_cfg.md.SadTh;
    stVdaChnAttr.unAttr.stMdAttr.u32ObjNumMax  = vda_cfg.md.ObjNumMax;

    log_info("going to init VDA chnl %u, size %ux%u", vda_cfg.md.VdaChn, vda_cfg.vi.width, vda_cfg.vi.height);

    int fd = hitiny_MPI_VDA_GetFd(vda_cfg.md.VdaChn);
    if(fd < 0)
    {
        log_error("Can't open VDA chn: 0x%x", fd);
        return;
    }

    hitiny_MPI_VDA_DestroyChn(vda_cfg.md.VdaChn);

    int s32Ret = hitiny_MPI_VDA_CreateChn(vda_cfg.md.VdaChn, &stVdaChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't create VDA chn: 0x%x", s32Ret);
        return;
    }

    log_info("going to bind");

    s32Ret = hitiny_sys_bind_VI_VDA(0, vda_cfg.vi.vi_chnl, vda_cfg.md.VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't bind VDA to VI: 0x%x", s32Ret);
        hitiny_MPI_VDA_DestroyChn(vda_cfg.md.VdaChn);
        return;
    }

    log_info("going to start recv");
    s32Ret = hitiny_MPI_VDA_StartRecvPic(vda_cfg.md.VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't HI_MPI_VDA_StartRecvPic: 0x%x", s32Ret);
        hitiny_sys_unbind_VI_VDA(0, vda_cfg.vi.vi_chnl, vda_cfg.md.VdaChn);
        hitiny_MPI_VDA_DestroyChn(vda_cfg.md.VdaChn);
        return;
    }

    log_info("VDA init OK!");

    mdw = (struct __ev_vda_md_watcher_s*)malloc(sizeof(struct __ev_vda_md_watcher_s));
    if (!mdw) return;
    mdw->VdaChn = vda_cfg.md.VdaChn;
    if (tuner_mode)
    {
        ev_io_init(&(mdw->_ev_io), __md_tuner_cb, fd, EV_READ);
    }
    else
    {
        ev_io_init(&(mdw->_ev_io), __md_work_cb, fd, EV_READ);
    }
    ev_io_start(loop, &(mdw->_ev_io));
}

void vda_md_done()
{
    int s32Ret = hitiny_MPI_VDA_StopRecvPic(vda_cfg.md.VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't VDA chnl stop recv pic: 0x%x", s32Ret);
    }
    log_info("done StopRecvPic");

    s32Ret = hitiny_sys_unbind_VI_VDA(0, vda_cfg.vi.vi_chnl, vda_cfg.md.VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't unbind VDA chn: 0x%x", s32Ret);
    }
    log_info("done unbind_VI_VDA");

    s32Ret = hitiny_MPI_VDA_DestroyChn(vda_cfg.md.VdaChn);
    if(s32Ret != HI_SUCCESS)
    {
        log_error("Can't destroy VDA chn: 0x%x", s32Ret);
    }
    log_info("done DestroyChn");
}

static void timeout_cb(struct ev_loop *loop, ev_timer* w, int revents)
{
    //fprintf(stderr, "DBG: active gpio watchers: ");
    //evgpio_watcher_print_list();
    ev_break(EV_A_ EVBREAK_ONE);

    ev_timer_again(loop, w);
}

int main(int argc, char** argv)
{
    int no_daemon_mode = 0;

    if (argc < 2 || !strncmp(argv[argc - 1], "--", 2))
    {
        fprintf(stderr, "Usage: %s [--tuner|--no-daemon] vda-md.ini\n", argv[0]);
        fprintf(stderr, " --tuner: tuner mode, read MD config, watch for events and dump info.\n");
        fprintf(stderr, " --no-daemon: work, but do not fork, logs to stderr.\n");
        return -1;
    }

    if (!strcmp(argv[1], "--tuner"))
    {
        tuner_mode = 1;
        no_daemon_mode = 1;
    }
    else if (!strcmp(argv[1], "--no-daemon"))
    {
        no_daemon_mode = 1;
    }

    fprintf(stderr, "Starting with config '%s'...\n", argv[argc - 1]);

    int ret = vda_cfg_read(argv[argc - 1], &vda_cfg);

    if (ret < 0)
    {
        print_error(ret);
        return ret;
    }

    if (!no_daemon_mode)
    {
        char buf[128];
        snprintf(buf, 128, "/tmp/hisina-md.log");
        log_info("Daemonize and log to %s", buf);
        log_create(buf, LOG_info);

        if (fork()) exit(0);
    }

    hitiny_MPI_VENC_Init();
    hitiny_vda_init();

    signal(SIGINT, action_on_signal);
    signal(SIGTERM, action_on_signal);

    struct ev_loop* loop = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);

    g_evcurl_proc = evcurl_create(loop);

    ev_timer timeout_watcher;
    ev_timer_init(&timeout_watcher, timeout_cb, 1., 0.);
    timeout_watcher.repeat = 2.;
    ev_timer_start(loop, &timeout_watcher);

    vda_md_init(loop);

    ret = mjpeg_snap_init(&vda_cfg, loop);

    if (ret < 0)
    {
        log_crit("Can't start MJPEG snap, stop!");
        goto FINISH;
    }

    do
    {
        ret = ev_run(loop, 0);
        if (stop_flag) break;
    }
    while (ret);

    mjpeg_snap_done(&vda_cfg);    

FINISH:
    vda_md_done();
    free(mdw);
    mdw = 0;

    hitiny_vda_done();
    hitiny_MPI_VENC_Done();

    evcurl_destroy(g_evcurl_proc);
    g_evcurl_proc = 0;


    return 0;
}

