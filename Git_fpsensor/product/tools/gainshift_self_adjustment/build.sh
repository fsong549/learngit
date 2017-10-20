#!/bin/bash
rm -rf out
NDK_PATH=/opt/android-ndk-r13
$NDK_PATH/ndk-build -B NDK_PROJECT_PATH=./ \
    NDK_APPLICATION_MK=Application.mk \
    APP_BUILD_SCRIPT=Android.mk \
    NDK_OUT=./out/obj \
    NDK_LIBS_OUT=./out/libs

if [ $? != 0 ]; then
    echo "build gainshift_self_adjustment failed"
    exit 1
fi

cp -rf ./out/libs/arm* prebuild/
rm -rf out