#include <platform/sdk.h>
#include <errno.h>
#include <string.h>
#include "mpi_sys.h"

#include <signal.h>
#include <aux/logger.h>

int stop_flag = 0;

void action_on_sognal(int signum)
{
    log_info("STOP");
    stop_flag = 1;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        log_error("./test_ao file_to_play.pcm");
        return -1;

    }

    signal(SIGINT, action_on_sognal);

    int ret = 0;
//    int ret = sdk_init(0);
// not to break VBs and working camera, we just init sdk
//    HI_MPI_SYS_Exit();
//    int ret = HI_MPI_SYS_Init();
//    if (ret < 0)
//    {
//        log_error("sdk_init() failed at %s: 0x%X", __sdk_last_call, ret);
//        return -1;
//    }

    log_warn("Warning! If segfaults here - broken libmpi.so/stat, please use patched version and libhisicompat.");
    ret = sdk_audio_init(0, PT_LPCM, 48000);
    if (ret < 0)
    {
        log_crit("init_sdk() failed at %s: 0x%X", __sdk_last_call, ret);
        return -1;
    }

    FILE* f = fopen(argv[1], "r");
    if (f)
    {
        log_info("Now play %s!", argv[1]);
        sdk_audio_play(f, &stop_flag);
        fclose(f);
    }
    else
    {
        log_error("ERROR %d", errno);
        log_error("Error %d: %s", errno, strerror(errno));
    }

    sdk_audio_done();

    return 0;
}




