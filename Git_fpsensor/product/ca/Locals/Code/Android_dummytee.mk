# =============================================================================
#
# =============================================================================
LOCAL_PATH	:= $(call my-dir)


#second way----------------------------------prebuild libfp_daemon_impl_ca.a--------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libfp_daemon_impl_ca
LOCAL_SRC_FILES := fp_lib/$(MODE)/libfp_daemon_impl_ca.a
include $(PREBUILT_STATIC_LIBRARY)



#--------------------------------------------fpsensor_fingerprint.default.so---------------------------------------------------------

include $(CLEAR_VARS)


# Module name
LOCAL_MODULE    := fpsensor_fingerprint.default

LOCAL_CFLAGS := 

# Add your folders with header files here
LOCAL_C_INCLUDES += $(LOCAL_PATH) \
                    $(PATH_TA_OUT)/Public \


# Add your source files here (relative paths)
LOCAL_SRC_FILES := fp_config_external.cpp \
                                 ta_entry/fp_tee_${TEE}.cpp

LOCAL_STATIC_LIBRARIES := fp_daemon_impl_ca


# # Need the MobiCore client library
# LOCAL_SHARED_LIBRARIES := libMcClient


#lzk add
LOCAL_CFLAGS += -Wall -std=c++11 -fexceptions -DLOG_TAG='"fpCoreCA"'
LOCAL_LDLIBS := -llog -ljnigraphics
#lzk add end

ifeq ($(MODE), Debug)
LOCAL_CFLAGS += -DDEBUG_ENABLE=1
else
LOCAL_CFLAGS += -DDEBUG_ENABLE=0
endif

include $(BUILD_SHARED_LIBRARY)



