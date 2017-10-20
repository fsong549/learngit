LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall

LOCAL_SRC_FILES += fpsensor_mmi.cpp \
				   fpsensor_gpio.cpp

LOCAL_MODULE := mmi_fingerprint

LOCAL_CFLAGS += -DFP_TEE_QSEE4=1

LOCAL_C_INCLUDES := $(QC_PROP_ROOT)/fastmmi/libmmi \
                    external/connectivity/stlport/stlport

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
ifeq ($(TARGET_COMPILE_WITH_MSM_KERNEL),true)
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
endif


LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libminui \
        libmmi \
        libQSEEComAPI 

include $(BUILD_SHARED_LIBRARY)

