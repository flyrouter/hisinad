#include "sdk.h"
#include <dlfcn.h>
#include <inttypes.h>
#include <aux/logger.h>

int (*sensor_register_callback_fn)(void);
int (*sensor_unregister_callback_fn)(void);
#if HISILICON_SDK_GEN < 2
void (*sensor_init_fn)(void);
#endif
void *libsns_so = NULL;

static int tryLoadLibrary(const char *path) {
    log_info("try to load: %s", path);
    libsns_so = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    log_info("libsns_so 0x%016" PRIXPTR, (uintptr_t)libsns_so);
    if (libsns_so == NULL) {
        log_error("dlopen \"%s\" error: %s", path, dlerror());
        return 0;
    }
    return 1;
}

int LoadSensorLibrary(const char *libsns_name) {
    char path[250];
    sprintf(path, "%s", libsns_name);
    if (tryLoadLibrary(path) != 1) {
        sprintf(path, "./%s", libsns_name);
        if (tryLoadLibrary(path) != 1) {
            sprintf(path, "/usr/lib/%s", libsns_name);
            if (tryLoadLibrary(path) != 1) {
                return 0;
            }
        }
    }

#if HISILICON_SDK_GEN < 2
    sensor_init_fn = dlsym(libsns_so, "sensor_init");
#endif

    sensor_register_callback_fn = dlsym(libsns_so, "sensor_register_callback");
    sensor_unregister_callback_fn =
        dlsym(libsns_so, "sensor_unregister_callback");
    return 1;
}

int sensor_register_callback(void) { return sensor_register_callback_fn(); }
int sensor_unregister_callback(void) { return sensor_unregister_callback_fn(); }

int sdk_sensor_init(const struct SensorConfig* sc)
{
    if (!LoadSensorLibrary(sc->dll_file))
    {
        log_error("Can't load %s", sc->dll_file);
        return -1;
    }

#if HISILICON_SDK_GEN < 2
    sensor_init_fn();
#endif

    int s32Ret = sensor_register_callback();
    if (HI_SUCCESS != s32Ret) {
        log_error("sensor_register_callback() failed with %#x!", s32Ret);
        return -1;
    }
}

void sdk_sensor_done()
{
    sensor_unregister_callback();
    dlclose(libsns_so);
    libsns_so = NULL;
}



