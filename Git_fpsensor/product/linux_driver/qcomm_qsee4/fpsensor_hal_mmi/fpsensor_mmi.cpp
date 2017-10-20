#include "mmi_module.h"
#include <android/log.h>

#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define FPTAG "fpCore_mmi"

void *lib_handle_ = NULL;
void (*initFunc)(void) =  NULL;
int64_t (*fpOpenFunc)(void) = NULL;
int (*fpCloseFunc)(void) =  NULL;
int32_t (*fpSetExtensionStatufFunc)(int32_t) = NULL;
int32_t (*fpSensorTest)(int32_t,int32_t) = NULL;

static bool is_64bit_system(void)
{
    long int_bits = (((long)((long *)0 + 1)) << 3);
    LOGI(FPTAG" is_64bit_system = %d", (int_bits == 64));
    return (int_bits == 64);
}

static int fpsensor_hal_init(void)
{
    int64_t ret = 0;
    char *pFinaFpHalPath = (char *)"/system/lib/hw/fpsensor_fingerprint.default.so";

    if(is_64bit_system())
        pFinaFpHalPath = (char *)"/system/lib64/hw/fpsensor_fingerprint.default.so";

    lib_handle_ = dlopen(pFinaFpHalPath, RTLD_NOW);
    if(lib_handle_ == NULL) {
          LOGE(FPTAG" dlopen failed can't find hal so:%s\n",pFinaFpHalPath);
          return -ENOENT;
    }
    LOGD( FPTAG" dlopen for fpsensor hal success \n");
    // find and get callback functions
    initFunc =  (void (*)(void))dlsym(lib_handle_, "fp_init");
    fpOpenFunc =  (int64_t (*)())dlsym(lib_handle_, "fp_openHal");
    fpCloseFunc =  (int (*)())dlsym(lib_handle_, "fp_closeHal");
    fpSetExtensionStatufFunc = (int32_t (*)(int32_t))dlsym(lib_handle_, "fp_set_extension_status");
    // check callback functions 
    if(fpSetExtensionStatufFunc == NULL
       || initFunc == NULL
       || fpOpenFunc == NULL
       || fpCloseFunc == NULL
       || fpSensorTest == NULL) {
            LOGE(FPTAG" dlopen can't get funcptr\n");
            LOGD(FPTAG" fpSetExtensionStatufFunc=%p\n", fpSetExtensionStatufFunc);
            LOGD(FPTAG" initFunc=%p\n", initFunc);
            LOGD(FPTAG" fpOpenFunc=%p\n", fpOpenFunc);
            LOGD(FPTAG" fpCloseFunc=%p\n", fpCloseFunc);
            LOGD(FPTAG" fpSensorTest=%p\n", fpSensorTest);
            dlclose(lib_handle_);
            lib_handle_ = NULL;
            return -EINVAL;
    }

    LOGD(FPTAG" fpsensor hal initializing...\n");
    fpSetExtensionStatufFunc(0);
    initFunc();
    ret = fpOpenFunc();
    if (ret) {
        LOGD(FPTAG "Find fpSensor, HAL id:%lld\n", ret);
        return 0;
    } else {
        LOGD(FPTAG " Can NOT Find fpSensor !!!!!!!!!\n");
        // release all resource here
        fpCloseFunc();
        dlclose(lib_handle_);
        lib_handle_ = NULL;
        fpSetExtensionStatufFunc = NULL;
        initFunc = NULL;
        fpOpenFunc = NULL;
        fpCloseFunc = NULL;
        fpSensorTest = NULL;
        return -ENOENT;
    }
}

static int32_t fpsensor_hal_deinit(void)
{
    if (lib_handle_ == NULL) {
        LOGE(FPTAG"%s: lib_handle_ is NULL, init failed?\n", __func__);
        return -ENOENT;
    }

    if (fpCloseFunc)
        fpCloseFunc();
    dlclose(lib_handle_);
    lib_handle_ = NULL;
    fpSetExtensionStatufFunc = NULL;
    initFunc = NULL;
    fpOpenFunc = NULL;
    fpCloseFunc = NULL;
    fpSensorTest = NULL;

    return 0;
}

static const char str_selftest_fail[] = "fpsensor SELFTEST fail";
static const char str_selftest_pass[] = "fpsensor SELFTEST pass";
static const char str_checkboard_fail[] = "fpsensor CHECKBOARD fail";
static const char str_checkboard_pass[] = "fpsensor CHECKBOARD pass";
static void *fpsensor_run_test(void *mod)
{
    int32_t status = 0;
    mmi_module_t *module = NULL;

    if(mod == NULL) {
        LOGE(FPTAG"%s NULL for cb function ", __func__);
        return NULL;
    }

    if (!lib_handle_ || !fpSensorTest) {
        LOGE(FPTAG"%s NULL for hal function ", __func__);
        return NULL;
    }

    module = (mmi_module_t *)mod;
    signal(SIGUSR1, signal_handler);


    /* selftest */
    status = fpSensorTest(111,0);
    if (status) {
        LOGE(FPTAG"%s selftest error: %d", __func__, status);
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_selftest_fail, sizeof(str_selftest_fail), PRINT_DATA);
        return NULL;
    } else {
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_selftest_pass, sizeof(str_selftest_pass), PRINT_DATA);
    }

    /* checkboard */
    status = fpSensorTest(110,0);
    if (status) {
        LOGE(FPTAG"%s checkboard error: %d", __func__, status);
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_checkboard_fail, sizeof(str_checkboard_fail), PRINT_DATA);
        return NULL;
    } else {
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_checkboard_pass, sizeof(str_checkboard_pass), PRINT_DATA);
    }

    return NULL;
}

static int32_t fpsensor_module_run_mmi(const mmi_module_t *module, unordered_map < string, string > &params)
{
    int ret = SUCCESS;
    
    LOGD(FPTAG"%s start ", __func__);

    ret = pthread_create((pthread_t *)&module->run_pid, NULL, fpsensor_run_test, (void *)module);
    if(ret < 0) {
        LOGE(FPTAG"%s:Can't create pthread: %s\n", __func__, strerror(errno));
        return FAILED;
    } else {
        pthread_join(module->run_pid, NULL);
    }

    return ret;
}

/**
* Before call Run function, caller should call module_init first to initialize the module.
* the "cmd" passd in MUST be defined in cmd_list ,mmi_agent will validate the cmd before run.
*
*/
static int32_t fpsensor_module_run(const mmi_module_t * module, const char *cmd, unordered_map < string, string > &params)
{
    int ret = FAILED;

    if(!module || !cmd) {
        LOGE(FPTAG"%s NULL point received ", __func__);
        return FAILED;
    }
    
    LOGD(FPTAG"%s start.command : %s", __func__, cmd);
    if(!strcmp(cmd, SUBCMD_MMI)) {
        ret = fpsensor_module_run_mmi(module, params);
    } else {
        LOGE(FPTAG"%s Invalid command: %s received ", __func__, cmd);
        ret = FAILED;
    }

   /** Default RUN mmi*/
    return ret;
}

static int32_t fpsensor_module_stop(const mmi_module_t *module)
{
    int32_t status = SUCCESS;

    LOGD(FPTAG"%s start.", __func__);

    if(module == NULL) {
        LOGE("%s NULL point received ", __func__);
        return FAILED;
    }
    pthread_kill(module->run_pid, SIGUSR1);

    return SUCCESS;
}

static int32_t fpsensor_module_init(const mmi_module_t * module, unordered_map < string, string > &params)
{
    int32_t ret = 0;

    LOGD(FPTAG"%s start ", __func__);

    if(module == NULL) {
        LOGE("%s NULL point received ", __func__);
        return FAILED;
    }

    ret = fpsensor_hal_init();
    if (ret) {
        LOGE(FPTAG" %s hal init failed\n", __func__);
        return FAILED;
    }

    LOGD(FPTAG" %s ok, %s-%s\n", __func__, __DATE__, __TIME__);
    return SUCCESS;
}

static int32_t fpsensor_module_deinit(const mmi_module_t *module)
{
    int32_t ret = 0;

    LOGD(FPTAG"%s start.", __func__);

    if(module == NULL) {
        LOGE(FPTAG"%s NULL point received ", __func__);
        return FAILED;
    }

    ret = fpsensor_hal_deinit();
    if (ret)
        return FAILED;

    return SUCCESS;
}

/**
* Methods must be implemented by module.
*/
static struct mmi_module_methods_t module_methods = {
    .module_init    = fpsensor_module_init,
    .module_deinit  = fpsensor_module_deinit,
    .module_run     = fpsensor_module_run,
    .module_stop    = fpsensor_module_stop,
};

/**
* Every mmi module must have a data structure named MMI_MODULE_INFO_SYM
* and the fields of this data structure must be initialize in strictly sequence as definition,
* please don't change the sequence as g++ not supported in CPP file.
*/
mmi_module_t MMI_MODULE_INFO_SYM = {
    .version_major              = 1,
    .version_minor              = 0,
    .name                       = "Fingerprint",
    .author                     = "Qualcomm Technologies, Inc.",
    .methods                    = &module_methods,
    .module_handle              = NULL,
    .supported_cmd_list         = NULL,
    .supported_cmd_list_size    = 0,
    .cb_print                   = NULL, /**it is initialized by mmi agent*/
    .run_pid                    = -1,
};
