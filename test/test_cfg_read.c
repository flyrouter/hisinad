#include <cfg/common.h>
#include <errno.h>
#include <string.h>

int my_simple_sec_cb(const char* s)
{
    printf("[%s]\n", s);
    return CFG_PROC_OK;
}

int my_simple_kv_cb(const char* k, const char* v)
{
    printf("%s=%s\n", k, v);
    return CFG_PROC_OK;
}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: ./test_cfg_reader config.ini\n");
        return -1;
    }

    int ret = cfg_proc_read(argv[1], my_simple_sec_cb, my_simple_kv_cb);
    switch (ret)
    {
        case CFG_PROC_IO:
        {
            printf("I/O error, %d: %s\n", errno, strerror(errno));
            break;
        }
        case CFG_PROC_SYNTAX:
        {
            printf("Syntax error at line %u, pos %u\n", cfg_proc_err_line_num(), cfg_proc_err_line_pos());
            break;
        }
        case CFG_PROC_WRONG_SECTION:
        {
            printf("Wrong section name at line %u, pos %u\n", cfg_proc_err_line_num(), cfg_proc_err_line_pos());
            break;
        }
        case CFG_PROC_OK:
        {
            break;
        }
        default:
        {
            printf("Unknown error %d\n", ret);
            break;
        }
    }

    return 0;
}

