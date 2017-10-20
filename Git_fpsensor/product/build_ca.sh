#!/bin/bash

source ./setup.sh

rm -r ca/Out/
./ca/Locals/Build/build.sh "$@"
if (( $? != 0 ))
then
    echo =====================Build Ca Fail!=====================
    exit 1
fi

if [ -d "./ca/Out/Bin/$APP_ABI/Release" ]; then
    cp -f ./ca/Out/Bin/${APP_ABI}/Release/libfpsensor_fingerprint.default.so ./tee_out/Release/
fi

if [ -d "./ca/Out/Bin/${APP_ABI}/Debug" ]; then
    cp -f ./ca/Out/Bin/${APP_ABI}/Debug/libfpsensor_fingerprint.default.so ./tee_out/Debug/
fi

if [ $ANDROID == 5 ]; then
    if [ -f "./tee_out/Release/libfpsensor_fingerprint.default.so" ]; then
        mv ./tee_out/Release/libfpsensor_fingerprint.default.so ./tee_out/Release/libfp_daemonNativeJni.so
    fi
    if [ -f "./tee_out/Debug/libfpsensor_fingerprint.default.so" ]; then
        mv ./tee_out/Debug/libfpsensor_fingerprint.default.so ./tee_out/Debug/libfp_daemonNativeJni.so
    fi
fi