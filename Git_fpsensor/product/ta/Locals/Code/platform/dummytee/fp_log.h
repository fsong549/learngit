#ifndef FP_LOG_H
#define FP_LOG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef NDK_ROOT
#include <android/log.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_TAG_D
#define LOG_TAG_D "fpCoreTA [DEBUG]"
#define LOG_TAG_I "fpCoreTA [INFO]"
#define LOG_TAG_E "fpCoreTA [ERROR]"
#endif
/*************************************************************************
*   Begin to define macros for printing log                             *
*************************************************************************/
#define FP_LOG_DEBUG_LEVEL   3
#define FP_LOG_INFO_LEVEL    2
#define FP_LOG_ERROR_LEVEL   1

/* debug */
#if( FP_LOG_LEVEL >= FP_LOG_DEBUG_LEVEL )
#define LOGD(...) \
    __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG_D,__VA_ARGS__)
#else
#define LOGD(...)
#endif

/* info */
#if( FP_LOG_LEVEL >= FP_LOG_INFO_LEVEL )
#define LOGI(...)  \
    __android_log_print(ANDROID_LOG_INFO,LOG_TAG_I,__VA_ARGS__)
#else
#define LOGI(...)
#endif

/* error */
#if( FP_LOG_LEVEL >= FP_LOG_ERROR_LEVEL )
#define LOGE(...)  \
    __android_log_print(ANDROID_LOG_ERROR,LOG_TAG_E,__VA_ARGS__)
#else
#define LOGE(...)
#endif

#ifdef __cplusplus
}
#endif

#endif

