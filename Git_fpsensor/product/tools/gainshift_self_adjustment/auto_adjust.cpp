
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#include <android/log.h>
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define FPTAG "auto_adjust "

#define RESULT_BUFER_SIZE 24

bool is_64bit_system(void)
{
    long int_bits = (((long)((long *)0 + 1)) << 3);
    LOGI(FPTAG" is_64bit_system = %d", (int_bits == 64));
    return (int_bits == 64);
}


void *lib_handle_ = NULL;
char test_result[RESULT_BUFER_SIZE];
int (*factory_init)(void) = NULL;
int (*factory_exit)(void) = NULL;
int (*push_gain_shift_pixctl)(int, int, int, char*,int) = NULL;
int (*capture_image_test_sync)(int) = NULL;
int cur_pixctl = 0;
int running = 1;

int display_mode = 0;

void deal_input_error(void)
{
    char dummy[128];
    scanf("%s",dummy);
    fflush(stdin);
}

void process_display_mode(void)
{
retry:
    printf("  input new mode[0,1]: ");
    int new_mode;
    if(scanf("%d",&new_mode) != 1)
    {
        deal_input_error();
        goto retry;
    }

    if(new_mode == 0)
    {
        display_mode = 0;
    }
    else
    {
        display_mode = 1;
    }

    return;
}

void parse_result(char *buff, int buf_size)
{
    if(display_mode == 0)
    {
        //just display half result
        for(int i = 0; i < buf_size / 2; i++)
        {
            printf(" 0x%02x",buff[i]);
        }
    }
    else
    {
        for(int i = 0; i < buf_size / 2; i++)
        {
            int value = buff[i * 2] + (buff[i * 2 + 1] << 8);
            printf(" 0x%04x", value);
        }
    }
    printf("\n");
    return;
}

void do_test(int gain, int shift, int pixctl)
{
    memset(test_result,0,sizeof(test_result));
    push_gain_shift_pixctl(gain,shift,pixctl,test_result,sizeof(test_result));
    printf("   gain:%2d,shift:%2d,pixctl:%2d-->",gain,shift,pixctl);
    parse_result(test_result,sizeof(test_result));
}

void process_push_gain_shift_pixctl(void)
{
    int gain = 0;
    int shift = 0;
    int pixctl = 0;
    printf("  input gain,shift,pixctl:");
    int ret = scanf("%d%d%d",&gain,&shift,&pixctl);
    if(ret == 3)
    {
        cur_pixctl = pixctl;
        do_test(gain,shift,pixctl);
    }
    else
    {
        deal_input_error();
    }
    return;
}

void process_loop_push_gain_shift(void)
{
    int GAIN_MAX = 16;
    int SHIFT_MAX = 32;

    for(int gain = 0; gain < GAIN_MAX; gain++)
    {
        for(int shift = 0; shift < SHIFT_MAX && running; shift++)
        {
            do_test(gain,shift,cur_pixctl);
        }
    }
    return;
}


void process_capture_image_test(void)
{
    static int sn = 0;
    if(capture_image_test_sync != NULL)
    {
        printf("  start capture image test, wait press sensor . . .\n");
        fsync(1);
        int32_t result = capture_image_test_sync(sn);
        printf("  capture image test finished, result =%d, check /data/fp_capture_test_%d.bmp \n",result,sn++);
    }
    return;
}

void Test(void)
{
retry:
    printf("\n-------------para auto adjust------------\n\n");

    printf("1. set result display mode [0:Byte,1:DByte]     ->Cur Mode:%d\n",display_mode);
    printf("2. push gain shift pixctl value\n");
    printf("3. loop push gain shift value\n");
    printf("4. capture image test\n");
    printf("5. quit\n\n");
    int choice = 0;
    printf(" input your choice[1~5]: ");
    if( scanf("%d",&choice) != 1)
    {
        deal_input_error();
        goto retry;
    }

    switch(choice)
    {
        case 1:
            process_display_mode();
            break;
        case 2:
            process_push_gain_shift_pixctl();
            break;
        case 3:
            process_loop_push_gain_shift();
            break;
        case 4:
            process_capture_image_test();
            break;
        case 5:
            return;
        default:
            printf(" invalid input,retry again\n");
            break;
    }

    goto retry;
}

int InitLib(void)
{
    LOGD(FPTAG"InitLib()\n");
    int ret = -1;
    char *pFinaFpHalPath = (char *)"/system/lib64/hw/fpsensor_fingerprint.default.so";
    if(!is_64bit_system()){
        LOGD(FPTAG"32bit system\n");
        pFinaFpHalPath = (char *)"/system/lib/hw/fpsensor_fingerprint.default.so";
    }
    lib_handle_ = dlopen(pFinaFpHalPath, RTLD_NOW);
    if(lib_handle_ == NULL){
          LOGE(FPTAG" dlopen failed can't find hal so: %s, errno= %d\n",pFinaFpHalPath,errno);
          ret = -errno;
          return ret;
    }
    LOGD( FPTAG" dlopen for fpsensor hal success \n");

    factory_init = (int (*)(void))dlsym(lib_handle_, "factory_init");
    factory_exit = (int (*)(void))dlsym(lib_handle_, "factory_exit");
    push_gain_shift_pixctl = (int (*)(int,int,int,char*,int))dlsym(lib_handle_, "push_gain_shift_pixctl");
    capture_image_test_sync = (int(*)(int))dlsym(lib_handle_, "capture_image_test_sync");

    if(factory_init == NULL || factory_exit == NULL || 
        push_gain_shift_pixctl == NULL ||
        capture_image_test_sync == NULL ){
            LOGE(FPTAG" dlopen can't get funcptr\n");
            LOGD(FPTAG" factory_init=%p\n",factory_init);
            LOGD(FPTAG" factory_exit=%p\n",factory_exit);
            LOGD(FPTAG" push_gain_shift_pixctl=%p\n",push_gain_shift_pixctl);
            LOGD(FPTAG" capture_image_test_sync=%p\n",capture_image_test_sync);

            ret = -EINVAL;
            goto out;
    }

    if(factory_init() == 0){
        LOGD(FPTAG " factory_init ok\n");
        ret = 0;
    }
    else
    {
        LOGE(FPTAG " factory_init error\n");
    }
out:
    return ret;
}

void DeInitLib(void)
{
    if(factory_exit)
    {
        factory_exit();
    }
    if (lib_handle_){
        dlclose(lib_handle_);
        lib_handle_ = NULL;
    }
    LOGD(FPTAG " DeInitLib ok\n");
}

void signal_func(int sign_no)
{
    if(sign_no == SIGINT){;
        running = 0;
        LOGD("SIGINT(ctrl+c) received,exit app\n");
        DeInitLib();
        exit(0);
    }
}


extern "C"
{
    int auto_adjust(void) {
        signal(SIGINT, signal_func);
        if(InitLib() == 0)
        {
            Test();
        }
        DeInitLib();

        return 0;
    }
}
