#!/bin/bash
#
# This file sets the path variables and resolves the dependencies for the
# different Makefiles included in the release.
#
# If you prefer to use the ARM DS-5 tools instead of GNU, please set
# 1) TOOLCHAIN=ARM
# 2) ARM_RVCT_PATH and LM_LICENSE_FILE according to your environment.
#
################### EDIT HERE ####################

export COMP_PATH_ROOT=$(dirname $(readlink -f ${BASH_SOURCE[0]}))


# TOOLCHAIN : ARM, GNU
# If variable is not set, use GNU by default
export TOOLCHAIN=${TOOLCHAIN:-"GNU"}

# Mode used for building samples
export MODE=${MODE:-"Release"}
#export MODE=${MODE:-"Debug"}

#zkli do not remove the next line: a stub for publish
export TEE=${TEE:-"dummytee"}

#export ANDROID=${ANDROID:-"8"}
#zkli do not remove the next line: a stub for publish
export ANDROID=${ANDROID:-"7"}


# Platform target : arm64-v8a,armeabi
#zkli do not remove the next line: a stub for publish
export APP_ABI=${APP_ABI:-"armeabi"}

export ALGO_VER=${ALGO_VER:-"R6A"}

echo "TEE PLATFORM: " ${TEE}
echo "Android Version: " ${ANDROID}
echo "APP_ABI: " ${APP_ABI}
echo "ALGO_VER: " ${ALGO_VER}

if [ $TOOLCHAIN == GNU ]; then
    # GCC Compiler variables
    export CROSS_GCC_PATH=/home/pub/tee/tool/gcc-arm-none-eabi-4_8-2014q2
    export CROSS_GCC_PATH_INC=${CROSS_GCC_PATH}/arm-none-eabi/include
    export CROSS_GCC_PATH_LIB=${CROSS_GCC_PATH}/arm-none-eabi/lib
    export CROSS_GCC_PATH_LGCC=${CROSS_GCC_PATH}/lib/gcc/arm-none-eabi/4.8.4
    export CROSS_GCC_PATH_BIN=${CROSS_GCC_PATH}/bin
fi

if [ $TOOLCHAIN == ARM ]; then
    export ARM_RVCT_PATH= # ARM DS-5         - e.g.: /usr/local/DS-5
    export LM_LICENSE_FILE= # DS-5 license     - e.g.: /home/user/DS5PE-*.dat
    export ARM_RVCT_PATH_BIN=$ARM_RVCT_PATH/bin
    export ARM_RVCT_PATH_INC=$ARM_RVCT_PATH/include
    export ARM_RVCT_PATH_LIB=$ARM_RVCT_PATH/lib
fi

# Android NDK path to ndk-build
export NDK_PATH=/opt/android-ndk-r13
export NDK_BUILD=${NDK_PATH}/ndk-build

# Android SDK Directory
export ANDROID_HOME=/home/pub/adt-bundle-linux-x86_64-20140702

# Java Home Directory
export JAVA_HOME=/usr/lib/jvm/jdk1.7.0_79

# Ant application for building TSdkSample
export ANT_PATH=/opt/apache-ant-1.9.6
export PATH=${ANT_PATH}:$PATH:$NDK_PATH


######################################################
# Checks
######################################################

if [ $TOOLCHAIN == GNU ]; then
    if [[ -z "$CROSS_GCC_PATH" ]] ;then
        echo "CROSS_GCC_PATH is not set in setup.sh"
        exit 1
    fi
fi

if [ $TOOLCHAIN == ARM ]; then
    if [[ -z "$ARM_RVCT_PATH" ]] ;then
        echo "ARM_RVCT_PATH is not set in setup.sh"
        exit 1
    fi
fi

if [[ -z "$ANDROID_HOME" ]] ;then
    echo "ANDROID_HOME is not set in setup.sh"
    exit 1
fi

if [[ -z "$JAVA_HOME" ]] ;then
    echo "JAVA_HOME is not set in setup.sh"
    exit 1
fi

#if [[ -z "$ANT_PATH" ]] ;then
#    echo "ANT_PATH is not set in setup.sh"
#    exit 1
#fi

# red: something wrong, critical
# green: process pass/ok
# yellow: just some infomation
function set_color()
{
    case "$1" in
        red)    nn="31";;
        green)  nn="32";;
        yellow) nn="33";;
    esac  
    color_begin=`echo -e -n "\033[${nn}m"`
    color_end=`echo -e -n "\033[0m"`
    while read line; do 
        echo "${color_begin}${line}${color_end}"
    done
}

#----------------------FEATURE CONFIG------------------------------------
export SUPPORT_NAV_REPORT_IOCTL=0
if [ $TEE == nutlet ]; then
    export SUPPORT_NAV_REPORT_IOCTL=1
fi

export FEATURE_TEE_STORAGE=1
export FEATURE_HW_AUTH=1
if [ $TEE == tbase ]; then
    export FEATURE_TEE_STORAGE=0
elif [ $TEE == nutlet ]; then
    export FEATURE_HW_AUTH=0
elif [[ $TEE == dummytee ]]; then
    export FEATURE_HW_AUTH=0
fi

#------------------------------------------------------------------------

######################################################
# Components
######################################################
export PATH_TA_CODE=${COMP_PATH_ROOT}/ta/Locals/Code
export PATH_TA_OUT=${COMP_PATH_ROOT}/ta/Out
export PATH_CA_OUT=${COMP_PATH_ROOT}/ca/Out
export PATH_SETUP=${COMP_PATH_ROOT}
export PATH_ASTYLE=${COMP_PATH_ROOT}/../tools/astyle

#--------------------------------------------- TEE:TBASE -----------------------------------------------
if [ $TEE == tbase ]; then
    # t-sdk-type for generating different ta 
    # value t-sdk: self signed, value t-sdk-raw: raw data
    export TSDK_TYPE=${TSDK_TYPE:-"t-sdk"}
    export TA_NAME=${TA_NAME:-"05220000000000000000000000000000.tlbin"}
    export t_base_dev_kit=${COMP_PATH_ROOT}
    export COMP_PATH_OTA=${t_base_dev_kit}/tee_sdk/${TEE}/${TSDK_TYPE}/OTA
    export COMP_PATH_Tools=${t_base_dev_kit}/Tools
    #relative path needed for COMP_PATH_MobiCoreClientLib_module (multi-OS compatibility for including library)
    export COMP_PATH_MobiCoreClientLib_module=${t_base_dev_kit}/tee_sdk/${TEE}/${TSDK_TYPE}/TlcSdk/Out
    export COMP_PATH_TlSdk=${t_base_dev_kit}/tee_sdk/${TEE}/${TSDK_TYPE}/TlSdk/Out

    #liuxn added for alipay 20160720
    export COMP_PATH_DrSdk=${t_base_dev_kit}/tee_sdk/${TEE}/${TSDK_TYPE}/DrSdk/Out
    export DRSDK_DIR=${COMP_PATH_DrSdk}     #for Alipay
    export TLSDK_DIR=${COMP_PATH_TlSdk}
    echo "Setup TBASE build environmet!" | set_color yellow


#--------------------------------------------- TEE:NUTLET -----------------------------------------------
elif [ $TEE == nutlet ]; then
    export TA_NAME=${TA_NAME:-"7b30b820-a9ea-11e5-b1780002a5d5c51b.ta"}
    #liuxn add for fp tac
    export t_nutlet_dev_kit=${COMP_PATH_ROOT}/tee_sdk/${TEE}/export-user_ta
    echo "Setup NUTLET build environmet!" | set_color yellow

#--------------------------------------------- TEE:QSEE --------------------------------------------------
elif [ $TEE == qsee ]; then
    export qsee_dev_kit=${PWD}/tee_sdk/qsee
    echo "Setup Qualcomm QSEE build environmet!" | set_color yellow

#--------------------------------------------- TEE:TKCORE --------------------------------------------------
elif [ $TEE == tkcore ]; then
    export TA_NAME=${TA_NAME:-"5b9e0e41-2636-11e1-ad9e0002a5d5c51c.ta"}
    export CROSS_COMPILE=/home/pub/tee/tool/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-
    export PATH=/home/pub/tee/tool/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin:$PATH

    export TKCORE_ARCH=arm64
    export TKCORE_VFP_ENABLE=1

    export TKCORE_SDK=/home/pub/tee/sdk/tkcore
    export TA_KIT=$TKCORE_SDK/ta_kit
        
    export APP_PLATFORM=android-19
    export APP_STL=gnustl_static
    export APP_CPPFLAGS=-std=c++11    
    echo "Setup TKCORE build environmet!" | set_color yellow
#--------------------------------------------- TEE:BEANPOD --------------------------------------------------
elif [ $TEE == beanpod ]; then
    export TA_NAME=${TA_NAME:-"fp_server"}

    #export UT_SDK_HOME=/opt/beanpod/ut_sdk_v1 #bjser
    #export UT_SDK_HOME=/opt/beanpod/ut_sdk_v6 #dechen GM02A
    #export UT_SDK_HOME=/opt/beanpod/sdk #boway V89
    export UT_SDK_HOME=/home/pub/tee/sdk/beanpod/ut_sdk_P9_huawei_JIM_ontim #zhongnuo JIM


    export TDS_SDK_ROOT=${UT_SDK_HOME}
    export GCC_HOME=${UT_SDK_HOME}/tools/arm-2011.03/bin
    echo "Setup BEANPOD build environmet!" | set_color yellow

elif [ $TEE == beanpod_old ]; then
    if [ -f /home/pub/tee/sdk/beanpod_old/setenv-can-input-para.sh ]; then
        source /home/pub/tee/sdk/beanpod_old/setenv-can-input-para.sh <<EOF
        0
EOF
    else
        echo "setenv-can-input-para.sh is inexistence, please check env!!"
        exit 1
    fi

    export TA_NAME=${TA_NAME:-"fp_server"}
    export UT_SDK_HOME=/home/pub/tee/sdk/beanpod_old #boway V89

    export TDS_SDK_ROOT=${UT_SDK_HOME}
    export GCC_HOME=${UT_SDK_HOME}/tools/arm-2011.03/bin
    echo "Setup BEANPOD_OLD build environmet!" | set_color yellow
#--------------------------------------------- TEE:SPREADTRUM --------------------------------------------------
elif [ $TEE == spreadtrum ]; then
    export PATH=/home/pub/tee/tool/arm-eabi-4.8/bin:$PATH
    export APP_ABI=armeabi

    export SPREADTRUM_SDK=/home/pub/tee/sdk/spreadtrum/trusty-sdk-sp9832-1.3.2
    #export SPREADTRUM_SDK=/home/pub/tee/sdk/spreadtrum/trusty-sdk-sp9850ka-1.3.0725 
    
    export TOS_BIN_PATH=${SPREADTRUM_SDK}
    export SPREADTRUM_SDK_TA=${SPREADTRUM_SDK}/sdk
    export SPREADTRUM_SDK_CA=${SPREADTRUM_SDK}/ca_lib

    echo "Setup SPREADTRUM build environmet!" | set_color yellow
elif [ $TEE == dummytee ]; then
    # ree platform export nothing
    echo "Setup dummytee build environmet!" | set_color yellow
else
    echo "Input tee platform name error!!" | set_color red
    exit 1
fi
