LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wunused -DLOG_TAG='"fpCoreFactoryMode"'

LOCAL_SRC_FILES := main.cpp \
                   auto_adjust.cpp
            

LOCAL_MODULE := fp_para_adjustment
LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)


