#include <platform/sdk.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <unistd.h>
#include <aux/logger.h>
#include <cfg/sensor_config.h>
#include "mpi_venc.h"
#include "mpi_sys.h"

int stop_flag = 0;

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
            log_error("Wrong section name [%s]", cfg_sensor_read_error_value());
        }
        else if (ret == CFG_PROC_KEY_BAD)
        {
            log_error("Wrong key \"%s\"", cfg_sensor_read_error_key());
        }
        else if (ret == CFG_PROC_VALUE_BAD)
        {
            log_error("Wrong value \"%s\" for %s", cfg_sensor_read_error_value(), cfg_sensor_read_error_key());
        }
    }
}

void usage_exit()
{
    log_error("./test_ao [--no-daemon] sensor_config.ini");
    exit(-1);
}

int main(int argc, char** argv)
{
    if (argc < 2) usage_exit();

    int deamon_mode = strcmp(argv[1], "--no-daemon");
    if (argc == 2 && !deamon_mode) usage_exit();

    signal(SIGINT, action_on_signal);
    signal(SIGTERM, action_on_signal);

    struct SensorConfig sc;
    memset(&sc, 0, sizeof(struct SensorConfig));

    int ret = cfg_sensor_read(argv[argc - 1], &sc);

    if (ret < 0)
    {
        print_error(ret);
        return ret;
    }

    if (deamon_mode)
    {
        log_info("Daemonize and log to /tmp/hisinad.log");
        char buf[128];
        snprintf(buf, 128, "/tmp/hisinad.log");
        log_create(buf, LOG_info);

        if (fork()) exit(0);
    }

    ret = sdk_init(&sc);
    if (ret < 0)
    {
        log_error("sdk_init() failed at %s: 0x%X", __sdk_last_call, ret);
        return -1;
    }

    ret = sdk_sensor_init(&sc);
    if (ret < 0)
    {
        log_error("sdk_sensor_init() failed: %d", ret);
        sdk_done();
        return ret;
    }

    ret = sdk_isp_init(&sc);
    if (ret < 0)
    {
        log_error("sdk_isp_init() failed: %d", ret);
        sdk_sensor_done();
        sdk_done();
        return ret;
    }

    ret = sdk_vi_init(&sc);
    
    if (!ret)
    {
        sdk_vpss_init(&sc);
        
        log_info("daemon ready, running...\n");
        while (!stop_flag)
        {
            sleep(1);
        }
    }

    log_info("closing isp...");
    sdk_isp_done();
    log_info("done");
    sdk_sensor_done();
    sdk_done();
    log_info("daemon exit");

    return 0;
}




