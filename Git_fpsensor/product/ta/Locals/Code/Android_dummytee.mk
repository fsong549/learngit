# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)
#  after yes CAN NOT HAVE a space or any other character
# LOGOFF := yes
LOGOFF := no
include $(CLEAR_VARS)

LOCAL_MODULE := libfp_ta_dummytee

LOCAL_SRC_FILES :=  fp_lib/${MODE}/libfp_ta_dummytee.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := fpsensor_module.default
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += \
                    $(LOCAL_PATH)/public \
                    $(LOCAL_PATH)/platform/dummytee/public \
                    $(LOCAL_PATH)/platform/dummytee 

LOCAL_SRC_FILES += \
                 $(LOCAL_PATH)/platform/dummytee/fp_ta_entry.c \
                 $(LOCAL_PATH)/platform/dummytee/fp_sec_pay.c \
                 $(LOCAL_PATH)/platform/dummytee/fp_internal_api.c \
                 $(LOCAL_PATH)/platform/dummytee/fp_dummytee_spi.c   \
                 $(LOCAL_PATH)/platform/dummytee/fp_dummytee_fs.c	\
                 $(LOCAL_PATH)/fp_ta_config.c

LOCAL_LDLIBS := -llog

LOCAL_STATIC_LIBRARIES := libfp_ta_dummytee 


LOCAL_CFLAGS += -DLOG_TAG='"fpCoreTA "' -Wall -DUSE_7110 -Wno-unused-function -Wno-unused-const-variable \
				 -DGIT_BRANCH_FROM_BUILD='"$(GIT_BRANCH)"' -DCOMMIT_ID_FROM_BUILD='"$(COMMIT_ID)"'

LOCAL_CFLAGS     += -DTARGET_MODE='"$(MODE)"' -DDUMMYTEE_ -std=c99
LOCAL_CFLAGS += -DFP_TEE_DUMMYTEE=1

ifeq ($(MODE), Debug)
LOCAL_CFLAGS += -DFP_LOG_LEVEL=3
else
LOCAL_CFLAGS += -DFP_LOG_LEVEL=1
endif
ifdef NDK_ROOT
LOCAL_CFLAGS += -DNDK_ROOT
endif

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
