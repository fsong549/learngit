#ifndef FP_LOG_H
#define FP_LOG_H

// #define DEBUG_ENABLE 0

#include <stddef.h>

#ifndef LOG_TAG
#define LOG_TAG "fpCoreJni"
#endif

#include <android/log.h>
#if (DEBUG_ENABLE)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define  LOGD(...)
#endif
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)



#endif

