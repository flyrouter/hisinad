#include <cfg/common.h>
#include <cfg/sensor_config.h>
#include <errno.h>
#include <string.h>



int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: ./test_cfg_reader config.ini\n");
        return -1;
    }

    struct SensorConfig sc;
    memset(&sc, 0, sizeof(struct SensorConfig));

    int ret = cfg_sensor_read(argv[1], &sc);

    if (ret < 0)
    {
        if (-1 == ret)
        {
            printf("I/O error, %d: %s\n", errno, strerror(errno));
        }
        else
        {
            printf("Error %s(%d) at line %u, pos %u\n", cfg_proc_err_msg(ret), ret, cfg_proc_err_line_num(), cfg_proc_err_line_pos());
            if (ret == CFG_PROC_WRONG_SECTION)
            {
                printf("Wrong section name [%s]\n", cfg_sensor_read_error_value());
            }
            else if (ret == CFG_PROC_KEY_BAD)
            {
                printf("Wrong key \"%s\"\n", cfg_sensor_read_error_key());
            }
            else if (ret == CFG_PROC_VALUE_BAD)
            {
                printf("Wrong value \"%s\" for %s\n", cfg_sensor_read_error_value(), cfg_sensor_read_error_key());
            }
        }
        return ret;
    }

    cfg_sensor_pretty_print(&sc);

    return 0;
}

