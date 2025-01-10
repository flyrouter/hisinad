#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s NUM (/1000)\n", argv[0]);
        return -1;
    }

    unsigned duty = atoi(argv[1]);
    if (duty > 1000) duty = 1000;

    int fd_pwm = open("/dev/pwm", 0);

    if (fd_pwm < 0)
    {
        fprintf(stderr, "can't open /dev/pwm\n");
        return -1;
    }

    unsigned param[4];
    param[0] = 1; // PWM1
    param[1] = duty;
    param[2] = 1000;
    param[3] = (duty > 0) ? 1 : 0;
    
    ioctl(fd_pwm, 1, param);

    close(fd_pwm);

    return 0;
}

