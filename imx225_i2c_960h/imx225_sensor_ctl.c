/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sony225_sensor_ctl.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2014/11/07
  Description   : Sony IMX225 sensor driver
  History       :
  1.Date        : 2014/11/07
    Author      : MPP
    Modification: Created file

******************************************************************************/
#include <stdio.h>
// #include <sys/types.h>
// #include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_i2c.h"

int sony_sensor_write_packet(I2C_DATA_S i2c_data)
{
	int fd = -1;
	int ret;

	fd = open("/dev/hi_i2c", 0);
	if(fd < 0)
	{
		printf("Open /dev/hi_i2c error!\n");
		return -1;
	}

    ret = ioctl(fd, CMD_I2C_WRITE, &i2c_data);

	close(fd);
	return 0;
}


int sensor_write_register(int reg_addr, int reg_value)
{
    I2C_DATA_S i2c_data ;
    unsigned int reg_width = 2;
    unsigned int data_width = 1;
    unsigned int device_addr = 0x34;

    i2c_data.dev_addr = device_addr ; 
    i2c_data.reg_addr = reg_addr    ; 
    i2c_data.addr_byte_num = reg_width  ; 
    i2c_data.data     = reg_value         ; 
    i2c_data.data_byte_num = data_width ;
        
    return sony_sensor_write_packet(i2c_data);
}

void sensor_prog(int* rom) 
{
}


void sensor_init()
{
 
	sensor_write_register(0x3000, 0x01); //Standby
	usleep(200000);
    
	// chip_id = 0x2
	sensor_write_register(0x3007, 0x00); //Window mode: 1280x960 mode
	sensor_write_register(0x3009, 0x11); //(0x01) 30fps mode + (0x10) HCG Mode
	sensor_write_register(0x300f, 0x00); // magic
	sensor_write_register(0x3012, 0x2c); // magic
	sensor_write_register(0x3013, 0x01); // magic
	sensor_write_register(0x3016, 0x09); // magic?

	sensor_write_register(0x3018, 0x4C); // VMAX = 0x0528 for 25/50 frame/s
	sensor_write_register(0x3019, 0x04);

	sensor_write_register(0x301b, 0x90); // HMAX = 0x1194
	sensor_write_register(0x301c, 0x11);

	sensor_write_register(0x301d, 0xc2); // magic number
	sensor_write_register(0x3032, 0x4b); // magic number, from stock

    sensor_write_register(0x3044, 0x01); // (0x01) ODBIT 12bit + (0x00) OPORTSEL == CMOS
    sensor_write_register(0x3046, 0x03); // XVSLNG 
    sensor_write_register(0x3047, 0x06); // XHSLNG
    sensor_write_register(0x3048, 0xc2); // ??

	sensor_write_register(0x3054, 0x67); // CMOS == 0x67

	sensor_write_register(0x305C, 0x20); // INCKSEL1 (INCK 37.125 MHz)
	sensor_write_register(0x305D, 0x00); // INCKSEL2
	sensor_write_register(0x305E, 0x20); // INCKSEL3
	sensor_write_register(0x305F, 0x00); // INCKSEL4

	sensor_write_register(0x3070, 0x02);
	sensor_write_register(0x3071, 0x01);
	sensor_write_register(0x309E, 0x22);
	sensor_write_register(0x30A5, 0xFB);
	sensor_write_register(0x30A6, 0x02);
	sensor_write_register(0x30B3, 0xFF);
	sensor_write_register(0x30B4, 0x01);
	sensor_write_register(0x30B5, 0x42);
	sensor_write_register(0x30B8, 0x10);
	sensor_write_register(0x30C2, 0x01);
    
	// chip_id = 0x3
	sensor_write_register(0x310F, 0x0F);
	sensor_write_register(0x3110, 0x0E);
	sensor_write_register(0x3111, 0xE7);
	sensor_write_register(0x3112, 0x9C);
	sensor_write_register(0x3113, 0x83);
	sensor_write_register(0x3114, 0x10);
	sensor_write_register(0x3115, 0x42);
	sensor_write_register(0x3128, 0x1E);
	sensor_write_register(0x31ED, 0x38);
	
	// chip_id = 0x4
	sensor_write_register(0x320C, 0xCF);
	sensor_write_register(0x324C, 0x40);
	sensor_write_register(0x324D, 0x03);
	sensor_write_register(0x3261, 0xE0);
	sensor_write_register(0x3262, 0x02);
	sensor_write_register(0x326E, 0x2F);
	sensor_write_register(0x326F, 0x30);
	sensor_write_register(0x3270, 0x03);
	sensor_write_register(0x3298, 0x00);
	sensor_write_register(0x329A, 0x12);
	sensor_write_register(0x329B, 0xF1);
	sensor_write_register(0x329C, 0x0C);

	usleep(200000);
	sensor_write_register(0x3000, 0x00); //release standy
	usleep(200000);
	sensor_write_register(0x3002, 0x00); //Master mode start
	usleep(200000);
	sensor_write_register(0x3049, 0x80); //XVS & XHS output
	usleep(200000);

	printf("-------Sony IMX225 Sensor Initial OK!-------\n");
    
	return ; 

}



