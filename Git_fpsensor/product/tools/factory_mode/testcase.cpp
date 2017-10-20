
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>

#include <android/log.h>
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define FPTAG "checkboardtest "


typedef void (*finger_detect_callback)(int);


struct timeval timer_start(const char *pInfo)
{
    struct timeval start;
    if (pInfo)
    {
        LOGI(" timer start %s :", pInfo);
    }

    gettimeofday(&start, NULL);
    return start;
}

int timer_end(const char *fmt, struct timeval start)
{
    struct timeval time_end, time_delta;
    int time_elapsed = 0;
    gettimeofday(&time_end, NULL);
    timersub(&time_end, &start, &time_delta);
    time_elapsed = time_delta.tv_sec * 1000000 + time_delta.tv_usec;

    if (fmt)
    {
        LOGI(" %s the time delta is=%d ms", fmt , time_elapsed / 1000);
    }

    return time_elapsed / 1000;
}



bool is_64bit_system(void)
{
    long int_bits = (((long)((long *)0 + 1)) << 3);
    LOGI(FPTAG" is_64bit_system = %d", (int_bits == 64));
    return (int_bits == 64);
}
int32_t finger_detect_test_finished = 0;
void on_finger_detect(int result)
{
    LOGD(FPTAG"on_finger_detect, result=%d",result);
    finger_detect_test_finished = 1;
}

int wait_finger_detect_event(void)
{
    int wait_time_ms = 0;
    struct timeval tv_start = timer_start(NULL);
    do
    {
        if(finger_detect_test_finished)
            break;

        wait_time_ms = timer_end(NULL, tv_start);
        if(wait_time_ms > 30 * 1000) // 30s
        {
            LOGD(FPTAG"wait finger_down timeout");
            return -ETIMEDOUT;
        }
        usleep(100 * 1000);
    }while(1);
    return 0;
}


int checkFpSensor(void){
    LOGD(FPTAG"checkFpSesor()\n");
    int ret = -1;
    char *pFinaFpHalPath = (char *)"/system/lib64/hw/fpsensor_fingerprint.default.so";
    if(!is_64bit_system()){
        LOGD(FPTAG"32bit system\n");
        pFinaFpHalPath = (char *)"/system/lib/hw/fpsensor_fingerprint.default.so";
    }
    void* lib_handle_ = dlopen(pFinaFpHalPath, RTLD_NOW);
    if(lib_handle_ == NULL){
          LOGE(FPTAG" dlopen failed can't find hal so: %s, errno= %d\n",pFinaFpHalPath,errno);
          ret = -errno;
          return ret;
    }
    LOGD( FPTAG" dlopen for fpsensor hal success \n");

    int (*factory_init)(void) = (int (*)(void))dlsym(lib_handle_, "factory_init");
    int (*spi_test)(void) = (int (*)(void))dlsym(lib_handle_, "spi_test");
    int (*interrupt_test)(void) = (int (*)(void))dlsym(lib_handle_, "interrupt_test");
    int (*deadpixel_test)(void) = (int (*)(void))dlsym(lib_handle_, "deadpixel_test");
    int (*finger_detect)(finger_detect_callback) = (int (*)(finger_detect_callback))dlsym(lib_handle_, "finger_detect");
    int (*factory_exit)(void) = (int (*)(void))dlsym(lib_handle_, "factory_exit");


    if(factory_init == NULL || spi_test == NULL || interrupt_test == NULL || 
        deadpixel_test == NULL|| finger_detect == NULL || factory_exit == NULL){
            LOGE(FPTAG" dlopen can't get funcptr\n");
            LOGD(FPTAG" factory_init=%p\n",factory_init);
            LOGD(FPTAG" spi_test=%p\n",spi_test);
            LOGD(FPTAG" interrupt_test=%p\n",interrupt_test);
            LOGD(FPTAG" deadpixel_test=%p\n",deadpixel_test);
            LOGD(FPTAG" finger_detect=%p\n",finger_detect);
            LOGD(FPTAG" factory_exit=%p\n",factory_exit);

            ret = -EINVAL;
            goto out ;
    }

    if(factory_init() == 0){

        LOGD(FPTAG " factory_init ok\n");
        ret = spi_test();
        LOGD(FPTAG " spi_test result = %d\n",ret);
        ret = interrupt_test();
        LOGD(FPTAG " interrupt_test result = %d\n",ret);
        ret = deadpixel_test();
        LOGD(FPTAG " deadpixel_test result = %d\n",ret);
        LOGD(FPTAG " press sensor to finish the finger detect test...");
        finger_detect(on_finger_detect);

        ret = wait_finger_detect_event();
        LOGD(FPTAG " wait_finger_detect_event return:%d",ret);
        factory_exit();
    }
    else
    {
        LOGE(FPTAG " factory_init error\n");
    }
out:
    if (lib_handle_){
        dlclose(lib_handle_);
    }
    LOGD(FPTAG " factory_test exit ret = %d\n",ret);
    return ret;
}

extern "C"
{
    int testcase(void) {
        return checkFpSensor();
    }
}
