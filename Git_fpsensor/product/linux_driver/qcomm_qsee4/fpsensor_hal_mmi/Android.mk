LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -Wall

LOCAL_SRC_FILES += fpsensor_mmi.cpp

LOCAL_MODULE := mmi_fingerprint

LOCAL_C_INCLUDES := $(QC_PROP_ROOT)/fastmmi/libmmi \
                    external/connectivity/stlport/stlport

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libminui \
        libmmi 

include $(BUILD_SHARED_LIBRARY)

