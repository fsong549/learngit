LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wunused -DLOG_TAG='"fpCoreFactoryMode"'

LOCAL_SRC_FILES := factory_mode.cpp \
                   testcase.cpp
            

LOCAL_MODULE := factory_mode
LOCAL_LDLIBS := -llog

include $(BUILD_EXECUTABLE)


