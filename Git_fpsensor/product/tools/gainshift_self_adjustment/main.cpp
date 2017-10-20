
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>
#include <android/log.h>
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


extern "C"  int auto_adjust(void);

int main(void) {
    int ret = auto_adjust();
    LOGD(" exit from main,ret = %d\n",ret);
    return ret;
}
