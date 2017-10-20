#!/bin/bash

# Source setup.sh
setup_script=setup.sh
#search_dir=$(dirname $(readlink -f ${BASH_SOURCE[0]}))
#while [[ ${setup_script} == "" ]]
#do
#  setup_script=$( find $search_dir -name "setup.sh" )
#  search_dir=$(dirname $search_dir)
#done
#if [[ ${setup_script} == "" ]]; then
#  echo "ERROR: setup.sh not found"
#  exit 1
#fi

echo $setup_script
source $setup_script

cd $(dirname $(readlink -f $0))
cd ../..

if [ ! -d Locals ]; then
  exit 1
fi

mkdir -p Out/Public
cp -f \
	Locals/Code/public/* \
	Out/Public/
cp -f Locals/Code/platform/${TEE}/public/* Out/Public

if [ $TEE == tbase ]; then
echo "------------------------------------------------------------- TEE:TBASE -----------------------------------------------------------------"  | set_color yellow
export TLSDK_DIR_SRC=${COMP_PATH_TlSdk}
export TLSDK_DIR=${COMP_PATH_TlSdk}
export TASDK_DIR_SRC=${COMP_PATH_TlSdk}
export TASDK_DIR=${COMP_PATH_TlSdk}
#add by rickon
export DRSPI_LIB_DIR=${COMP_PATH_ROOT}/tee_sdk/${TEE}/${TSDK_TYPE}/tl_drv_lib
export DRSEC_LIB_PATH=${DRSPI_LIB_DIR}/drsec.lib
export DRMEM_LIB_PATH=${DRSPI_LIB_DIR}/drutils.lib
#added by liuxn
export DRFP_LIB_DIR=${COMP_PATH_ROOT}/pay_lib/${TEE}/Out/Bin/${MODE}



echo "Running make..."
make -f Locals/Code/makefile_${TEE}.mk "$@"
if [ $? -ne 0 ]; then
    echo "[ERROR]:" $TEE "TA build failed! "  | set_color red
    exit 1;
fi
elif [ $TEE == nutlet ]; then
echo "------------------------------------------------------------- TEE:NUTLET -----------------------------------------------------------------"  | set_color yellow
export NUTLET_SDK_HOME=${t_nutlet_dev_kit}
export NUTLET_LIB="disable"
# echo ${NUTLET_LIB}

echo "Running make..."
make -f Locals/Code/makefile_${TEE}.mk "$@"

if [ $? -ne 0 ]; then
    make -f Locals/Code/makefile_${TEE}.mk clean
    echo "[ERROR] ta build failed!" | set_color red
    exit 1;
fi

make -f Locals/Code/makefile_${TEE}.mk clean

./Locals/Build/TASign RSA_SHA1 -i ./Out/Bin/${MODE}/7b30b820-a9ea-11e5-b1780002a5d5c51b.ta -k ./Locals/Build/key.txt -o ./Out/Bin/${MODE}/7b30b820-a9ea-11e5-b1780002a5d5c51b_bak.ta
rm ./Out/Bin/${MODE}/7b30b820-a9ea-11e5-b1780002a5d5c51b.ta
mv ./Out/Bin/${MODE}/7b30b820-a9ea-11e5-b1780002a5d5c51b_bak.ta ./Out/Bin/${MODE}/7b30b820-a9ea-11e5-b1780002a5d5c51b.ta


elif [ $TEE == qsee ]; then
echo "------------------------------------------------------------- TEE:QSEE -----------------------------------------------------------------" | set_color yellow
if [ ! -d ../tee_sdk/${TEE}/qsee_TZ.4.0.1 ]; then
    echo "qsee_TZ.4.0.1 SDK is not exist, copy and decompress it now..." | set_color red
    tar xf /home/pub/tee/sdk/qsee/qsee_TZ.4.0.1.tgz -C ../tee_sdk/${TEE}/
    echo "Done" | set_color green
fi
echo "Copy QSEE 4.0.1 project files..." | set_color yellow
    cp -arvf Locals/Code/platform/${TEE} ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/platform/
    cp -arvf Locals/Code/platform/${TEE}/SConscript.01 ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/build/SConscript
    cp -arvf Locals/Code/public ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/
    cp -rvf Locals/Code/fp_ta_config.c ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/
    cp -rvf Locals/Code/fp_lib/${MODE}/libfp_ta_${TEE}.a ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/lib/
    sleep 2 # wait copy finish
echo "Build QSEE TA now..." | set_color yellow
    ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/build/ms/build.sh CHIPSET=msm8996 fngap64 -c
    ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/build/ms/build.sh CHIPSET=msm8996 fngap64
if [ ! -f ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/build/ms/bin/IADAANAA/fngap64.mbn ]; then
    echo "------------------ Build fpsensor QSEE 4.0.1 TA failed, please check build log...------------------" | set_color red
    exit 1
fi

if [ ! -d ../tee_sdk/${TEE}/qsee_TZ.4.0.5 ]; then
    echo "qsee_TZ.4.0.5 SDK is not exist, copy and decompress it now..." | set_color red
    tar xf /home/pub/tee/sdk/qsee/qsee_TZ.4.0.5.tgz -C ../tee_sdk/${TEE}/
    echo "Done" | set_color green
fi
echo "Copy QSEE 4.0.5 project files..." | set_color yellow
    cp -arvf Locals/Code/platform/${TEE} ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/platform/
    cp -arvf Locals/Code/platform/${TEE}/SConscript.05 ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/SConscript
    cp -arvf Locals/Code/public ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/
    cp -rvf Locals/Code/fp_ta_config.c ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/
    cp -rvf Locals/Code/fp_lib/${MODE}/libfp_ta_${TEE}.a ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/core/securemsm/trustzone/qsapps/fngap64/src/lib/
    sleep 2 # wait copy finish
echo "Build QSEE 4.0.5 TA now..." | set_color yellow
    ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/build/ms/build.sh CHIPSET=msm8937 fngap64 -c
    ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/build/ms/build.sh CHIPSET=msm8937 fngap64
if [ ! -f ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/build/ms/bin/ZALAANAA/fngap64.mbn ]; then
    echo "------------------ Build fpsensor QSEE 4.0.5 TA failed, please check build log...------------------" | set_color red
    exit 1
fi
if [ ! -d ./Out/Bin/${MODE} ]; then
    mkdir -p Out/Bin/${MODE}/qsee_4.0.1
    mkdir -p Out/Bin/${MODE}/qsee_4.0.5
fi
cp -rvf ../tee_sdk/${TEE}/qsee_TZ.4.0.1/trustzone_images/build/ms/bin/PIL_IMAGES/SPLITBINS_IADAANAA/fngap64*   Out/Bin/${MODE}/qsee_4.0.1/
cp -rvf ../tee_sdk/${TEE}/qsee_TZ.4.0.5/trustzone_images/build/ms/bin/PIL_IMAGES/SPLITBINS_ZALAANAA/fngap64*   Out/Bin/${MODE}/qsee_4.0.5/

mkdir -p Out/Public
cp -vf Locals/Code/public/*  Out/Public/
cp -vf Locals/Code/platform/${TEE}/fp_ta_entry.h Out/Public/
exit 0



elif [ $TEE == tkcore ]; then
echo "------------------------------------------------------------- TEE:TKCORE -----------------------------------------------------------------" | set_color yellow

cd ${PATH_TA_CODE}
export OUTPUT_DIR=${PATH_TA_OUT}/Bin/${MODE}
mkdir -p ${OUTPUT_DIR}
export TEE_LIB_BUILD="disable"

echo "Running ta make..."
cp -f sub_${TEE}.mk sub.mk
make -f makefile_${TEE}.mk clean_ta
make -f makefile_${TEE}.mk O=${OUTPUT_DIR} build_ta
if [ $? -ne 0 ]; then
    echo "[ERROR]:" $TEE "TA build failed! "  | set_color red
    exit 1;
fi
rm -f sub.mk


elif [ $TEE == beanpod ] || [ $TEE == beanpod_old ]; then
echo "------------------------------------------------------------- TEE:BEANPOD -----------------------------------------------------------------" | set_color yellow

cd ${PATH_TA_CODE}
export OUTPUT_DIR=${PATH_TA_OUT}/Bin/${MODE}
mkdir -p ${OUTPUT_DIR}
export TEE_LIB_BUILD="disable"

echo "Running ta make..."
make -f makefile_${TEE}.mk clean_ta
make -f makefile_${TEE}.mk build_ta
if [ $? -ne 0 ]; then
    echo "[ERROR]:" $TEE "TA build failed! "  | set_color red
    exit 1;
fi
make -f makefile_${TEE}.mk copy_files


elif [ $TEE == spreadtrum ]; then
echo "------------------------------------------------------------- TEE:SPREADTRUM -----------------------------------------------------------------" | set_color yellow

cd ${PATH_TA_CODE}
export BUILDROOT=${PATH_TA_OUT}/Bin/${MODE}
export TEE_LIB_BUILD="disable"
PATH_ELF=$BUILDROOT/user_tasks${PATH_TA_CODE}

echo "Running ta make..."
cp -f makefile_${TEE}.mk rules.mk
rm -rf ${BUILDROOT}
mkdir -p ${BUILDROOT}
cd ${SPREADTRUM_SDK_TA}
make M="${PATH_TA_CODE}:TA"
if [ $? -ne 0 ]; then
    echo "[ERROR]:" $TEE "TA build failed! "  | set_color red
    exit 1;
fi
cd ${PATH_TA_CODE}
rm -f rules.mk

if [ ! -f $PATH_ELF/Code.elf ]; then
    echo "[ERROR] build failed!"
    exit 1;
fi

cp -f $PATH_ELF/Code.elf $BUILDROOT/fp_ta.elf
cp -f $PATH_ELF/Code.syms.elf $BUILDROOT/fp_ta.syms.elf

$SPREADTRUM_SDK/mktosimg --tos $TOS_BIN_PATH/tos.org.bin --ta $BUILDROOT/fp_ta.elf -o $BUILDROOT/tos.bin


elif [ $TEE == dummytee ]; then
echo "------------------------------------------------------------- REE:SO -----------------------------------------------------------------" | set_color yellow
ANDROID=${ANDROID:-"6"}
cd ${PATH_TA_CODE}
export ALGO_DIR=../../../../pb_algorithm
    ndk-build -B \
        NDK_APPLICATION_MK=Application_dummytee.mk \
        APP_BUILD_SCRIPT=Android_dummytee.mk \
        NDK_PROJECT_PATH=./ \
        NDK_OUT=../../Out/Bin/${MODE}/${TA_NAME}/obj/  \
        NDK_LIBS_OUT=../../Out/Bin/${MODE}/${TA_NAME}/ \
        APP_ABI=${APP_ABI} \
        TARGET_ANDRIOD_VERSION=$ANDROID
if [ $? -ne 0 ]; then
    echo "[ERROR] dummytee ta build failed!" | set_color red
    exit 1;
fi
mkdir -p ../../Out/Public
cp -vf ../Code/public/*  ../../Out/Public/
exit 0
else
echo "Input tee platform name error!!" | set_color red
exit 1
fi
