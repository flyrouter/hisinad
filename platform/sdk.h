#ifndef __platform_sdk_h__
#define __platform_sdk_h__

#include <cfg/sensor_config.h>
#include <stdio.h>

extern const char* __sdk_last_call;

void set_verbose_level(int lvl);

int sdk_init(const struct SensorConfig* sc);
void sdk_done();

int sdk_audio_init(int bMic, PAYLOAD_TYPE_E payload_type, int audio_rate);
void sdk_audio_done();

/* ret -1: error, 0: played ok to the end, 1: playing interrupted with stop_flag */
int sdk_audio_play(FILE* f, int* stop_flag);


int sdk_sensor_init(const struct SensorConfig* sc);
int sdk_isp_init(const struct SensorConfig* sc);
int sdk_vi_init(const struct SensorConfig* sc);
int sdk_vpss_init(const struct SensorConfig* sc);
void sdk_vi_done();
void sdk_isp_done();
void sdk_sensor_done();


#if HISILICON_SDK_GEN == 1
//#include <platform/hi_v1/sdk.h>
#elif HISILICON_SDK_GEN == 2
//# error "hisi v2 is not ready, sorry"
#else
# error "platform not defined"
#endif



#endif // __platform_sdk_h__

