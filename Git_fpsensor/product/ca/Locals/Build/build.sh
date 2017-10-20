#!/bin/bash

# Source setup.sh
t_sdk_root=`pwd`
setup_script=setup.sh
cur_dir=$(dirname $(readlink -f ${BASH_SOURCE[0]}))
source $setup_script


if [ "$MODE" == "Release" ]; then
  echo -e "Mode\t\t: Release"
  OUT_DIR=Out/Bin/$APP_ABI/Release
  OPTIM=release
else
  echo -e "Mode\t\t: Debug"
  OUT_DIR=Out/Bin/$APP_ABI/Debug
  OPTIM=debug
fi

# go to project root
cd $cur_dir/../..

### ---------------- Generic Build Command ----------------
	# V=1 \
	# NDK_LOG=1 \

APP_OUT_PATH=Out/_build

# run NDK build
${NDK_BUILD} \
    -B \
    NDK_PROJECT_PATH=Locals/Code \
    APP_PLATFORM=android-21 \
    NDK_MODULE_PATH=${t_sdk_root} \
    NDK_APP_OUT=$APP_OUT_PATH \
    APP_BUILD_SCRIPT=Locals/Code/Android_${TEE}.mk \
    APP_ABI=$APP_ABI

if [ $? -ne 0 ]; then
    echo "[ERROR] lib build failed!"
    exit 1;
fi

mkdir -p $OUT_DIR

cp -r $PWD/Locals/Code/libs/$APP_ABI/* $OUT_DIR

echo
echo Output directory of build is $PWD/$OUT_DIR



