## hisinad

Experiments with hi3516cv100 (and similar v1 Hisilicon SoCs).

Please contact pianisteg.mobile@gmail.com for any questions.

## How to compile?

You can build hisinad for arm and for your desktop platform. Just create two folders — for example, `build_arm` and `build_pc`.

Some utilities and tests can be compiled for any platform:

```
cmake -DPLATFORM_SDK_DIR=/path/to/hisilicon_sdk/ ../hisinad/
```

Hardware-specific software — only for target platform, please set path to toolchain:

```
cmake -DPLATFORM_SDK_DIR=/path/to/hisilicon_sdk/ -DCMAKE_TOOLCHAIN_FILE=/path/to/openipc-firmware/output/host/share/buildroot/toolchainfile.cmake ../hisinad/
```

test line

