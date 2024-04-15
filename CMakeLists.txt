cmake_minimum_required(VERSION 2.8.12)
project(hisinad C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(HiSiliconSDK REQUIRED)
find_package(Threads REQUIRED)
find_library(CURL_LIBRARY NAMES curl)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${VENDOR_SDK_INCLUDE_DIRS})
include_directories(libev)

if (HISILICON_SDK_VERSION STREQUAL "1.0.B.0.")
add_definitions(-DISP_EX_ATTR)
endif()

add_library(hisinad_aux STATIC aux/logger.c aux/system.c)
add_library(hisinad_cfg STATIC cfg/common.c cfg/sensor_config.c cfg/sensor_cfg_print.c)
add_library(libevx STATIC libev/ev.c)
add_library(evcurl STATIC evcurl/evcurl.c)

add_executable(test_cfg_read test/test_cfg_read.c cfg/common.c)
add_executable(test_cfg_sensor test/test_cfg_sensor.c)
target_link_libraries(test_cfg_sensor hisinad_cfg)

add_executable(test_tinydraw_0 test/test_tinydraw_0.c tinydraw/image.c tinydraw/save_bmp.c)
add_executable(test_tinydraw_1 test/test_tinydraw_1.c tinydraw/image.c tinydraw/save_bmp.c tinydraw/monaco32_fixedsize_rawbb_2bpp.c tinydraw/font.c)
add_executable(test_tinydraw_2 test/test_tinydraw_2.c tinydraw/image.c tinydraw/save_bmp.c tinydraw/monaco32_fixedsize_rawbb_2bpp.c tinydraw/font.c)

add_executable(test_pumap_vi test/test_pumap_vi.c pumap/vi.c)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL armv5l)
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")

add_executable(test_ao test/test_ao.c platform/sdk_sys.c platform/sdk_audio.c platform/sdk_ugly.c)
target_link_libraries(test_ao ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat hisinad_aux hisinad_cfg)

add_executable(test_ao_tiny hitiny/hitiny_sys.c hitiny/hitiny_aio.c hitiny/hitiny_ao.c test/test_ao_tiny.c)
target_link_libraries(test_ao_tiny gcc_s hisinad_aux hisinad_cfg)

add_executable(hisinad hisinad/daemon.c cfg/common.c platform/sdk_sys.c platform/sdk_ugly.c platform/sdk_sensor.c platform/sdk_isp.c)
target_link_libraries(hisinad ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat hisinad_aux hisinad_cfg)

add_executable(test_jpeg test/test_jpeg.c cfg/common.c platform/sdk_sys.c platform/sdk_audio.c platform/sdk_ugly.c platform/sdk_sensor.c platform/sdk_isp.c)
target_link_libraries(test_jpeg ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat hisinad_aux hisinad_cfg)

add_executable(test_md test/test_md.c cfg/common.c hitiny/hitiny_sys.c hitiny/hitiny_vda.c)
target_link_libraries(test_md hisinad_aux)

add_executable(test_jpeg_dn test/test_jpeg_dn.c cfg/common.c platform/sdk_sys.c platform/sdk_audio.c platform/sdk_ugly.c platform/sdk_sensor.c platform/sdk_isp.c)
target_link_libraries(test_jpeg_dn ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat hisinad_aux hisinad_cfg)

add_executable(test_jpeg_nosdk test/test_jpeg_nosdk.c hitiny/hitiny_venc_fd.c hitiny/hitiny_sys.c)
target_link_libraries(test_jpeg_nosdk hisinad_aux)

add_executable(test_jpeg_rgn_nosdk test/test_jpeg_rgn_nosdk.c hitiny/hitiny_venc_fd.c hitiny/hitiny_sys.c hitiny/hitiny_region.c tinydraw/image.c tinydraw/save_bmp.c tinydraw/monaco32_fixedsize_rawbb_2bpp.c tinydraw/font.c)
target_link_libraries(test_jpeg_rgn_nosdk hisinad_aux)

add_executable(test_mmz_nosdk test/test_mmz.c hitiny/hitiny_sys.c)
target_link_libraries(test_mmz_nosdk hisinad_aux)

#add_executable(test_mmz_sdk test/test_mmz_sdk.c)
#target_link_libraries(test_mmz_sdk ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat hisinad_aux)

add_executable(tools_vi_dump tools/vi_dump.c)
target_link_libraries(tools_vi_dump ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat)

add_executable(tools_vi_dump_nosdk tools/vi_dump_nosdk.c hitiny/hitiny_vi.c hitiny/hitiny_sys.c hitiny/hitiny_ive.c)
target_link_libraries(tools_vi_dump_nosdk hisinad_aux)

add_executable(hisina-md-tune tools/hisina-md-tune/mjpeg_snap.c tools/hisina-md-tune/main.c hitiny/hitiny_sys.c hitiny/hitiny_vda.c cfg/vdacfg.c cfg/common.c hitiny/hitiny_venc_fd.c)
target_link_libraries(hisina-md-tune evcurl hisinad_aux ${CURL_LIBRARY} libevx)

add_executable(tools_vi_ive_hist tools/vi_ive_hist.c)
target_link_libraries(tools_vi_ive_hist ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat)

add_executable(tools_hi_vi_reg tools/hi_vireg.c platform/sdk_ugly.c)

add_library(sns_imx225_i2c_720p SHARED imx225_i2c/imx225_cmos.c imx225_i2c/imx225_sensor_ctl.c)
add_library(sns_imx225_i2c_960h SHARED imx225_i2c_960h/imx225_cmos.c imx225_i2c_960h/imx225_sensor_ctl.c)
add_library(sns_imx225_spi_720p SHARED imx225_spi/imx225_cmos.c imx225_spi/imx225_sensor_ctl.c)
add_library(sns_jxh42_i2c SHARED jxh42_i2c/jxh42_sensor_ctl.c jxh42_i2c/jxh42_cmos.c)

add_executable(sample_audio test/sample_audio/sample_audio.c test/sample_audio/sample_comm_audio.c test/sample_audio/sample_comm_sys.c platform/sdk_ugly.c)
target_link_libraries(sample_audio ${VENDOR_SDK_LIBRARIES} gcc_s hisicompat)

endif()

