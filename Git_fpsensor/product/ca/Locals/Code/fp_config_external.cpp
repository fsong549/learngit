#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "fp_log.h"
#include "fp_common_external.h"
#include "fp_tee_types.h"

#define FPTAG "fp_config_external.cpp "

//for gesture
const int32_t gKeys[] =
{
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_ENTER, //touch
    KEY_ESC,  //long press
    KEY_SELECT//double click
};

gesture_config_t gesture_config =
{
    .enable_direction = NAV_DISABLE,
    .enable_long_press = NAV_DISABLE,
    .enable_double_click = NAV_DISABLE,
    .long_press_time_threshold = 600,
    .double_click_time_threshold = 500,
    .enable_touch = NAV_ENABLE ,
    .up_threshold    = 2,
    .down_threshold  = 2,
    .left_threshold  = 2,
    .right_threshold = 2,
    .two_dimensional_nav = NAV_ENABLE,
    .report_touch_by_timer = NAV_ENABLE
};

//the type of the following value should be int32_t

#define FAE_VERSION_SUB  0
#define FAE_CUSTOMER_ID  0
#define FAE_PRODUCT_ID   0

extern "C" {

    fp_config_item_t fp_system_config_table[] =
    {
        {"default_config_item",                                         0},        //1lzk do not modify this line
//----------------------------fp_config----------------------------------------------------
        {FP_CONFIG_REPORT_DETAIL_MSG,                                   0},         //(0,1) do not use this feature in 6.0 platform, enable on 5.X
        {FP_CONFIG_FEATURE_NAVIGATOR,                                   1},         //(0,1) according to user requirements
        //caution: modify definition of FINGER_MAX_COUNT in TA to change the enroll finger number ,do not modify this line
        {FP_CONFIG_MAX_ENROLL_FINGER_SLOT_NUMBER,        FINGER_MAX_COUNT},         //[5~20] how many finger can be enrolled in the database
        {FP_CONFIG_FEATURE_AUTH_FIRST_ENTER_NO_WAIT_UP,                 1},         //(0,1) on first time enter authenticate funcation, do not wait finger up
        {FP_CONFIG_FEATURE_AUTH_UNMATCH_RETRY,                          1},         //(0,1) when authentication unmatch, do not wait finger up and retry
        {FP_CONFIG_FEATURE_AUTH_UNMATCH_RETRY_TIMES,                    2},         //[0~3] when authenticate failed, retry how many times, configurable when FP_CONFIG_FEATURE_AUTH_UNMATCH_RETRY = 1
        {FP_CONFIG_FEATURE_AUTH_RETRY_ADDITIONAL_2S,                    1},         //(0,1) after auth failed and report unmatch msg, allow to retry additional 2s, configurable when FP_CONFIG_FEATURE_AUTH_UNMATCH_RETRY = 1

        {FP_CONFIG_FEATURE_AUTH_HAL_CONTROL_UPDATE_TPLT,                0},         //(0,1) enable or disable hal control update templates when authentication matched
        {FP_CONFIG_FEATURE_AUTH_HAL_CONTROL_STORE_TIME,                 0},         //10 means 10s   (0s~24*60*60s  second) control the time interval of store_temp
        {FP_CONFIG_FEATURE_INTERRUPT_STATUS_REG,                        1},         //(0,1) enable or disable, enable it when get int level can't work normal, instead of it, confirming last interrupt status by reading reg 0x18.
        {FP_CONFIG_FEATURE_DISABLE_AUTHORIZE_ENROLL,                    0},         //(0,1) enable or disable the authorize_enroll process, when disabled(set to 1), the enroll has no time_out, and do not check the input "challenge"
//---------------------------project-------------------------------------------------------
        {FP_CONFIG_FEATURE_AUTH_SUCCESS_CONTINUOUS,                     0},         //(0,1) for ZTE yude this feature enable continuous capture even authentication successes
//---------------------------dev_test------------------------------------------------------
        {FP_CONFIG_RECORD_STATISTICAL,                                  0},         //(0,1) control whether to enable record authentication statistical info
        {FP_CONFIG_STORE_CAPTURED_IMG,                                  0},         //(0,1) test purpose control whether to store the captured image , the default name is /sdcard/fpimg.bmp
        {FP_CONFIG_FEATURE_ENHANCEMENT_BMP_BEFORE_SAVE,                 0},         //(0,1) enable or disable enhancement bmp before save to file, contact "tczhai" for more info
        {FP_CONFIG_FEATURE_ENABLE_SYNC_XML,                             0},         //(0,1) enable or disable, when enabled, can sync templates in db and xml in framework. need selinux permission
        {FP_CONFIG_FEATURE_ENABLE_EXT_SVC2,                             1,         //(0,1) enable or disable ext_svc2 service
        {FP_CONFIG_VALUE_ENROLL_IMG_QUALITY_BAD_RPT_CODE,               2},         //(!= 0) when enroll image quality too bad, report which code to UI, should not set to 0
//---------------------------fae-----------------------------------------------------------
        {FP_CONFIG_VALUE_FAE_VERSION_SUB,                 FAE_VERSION_SUB},         //sub  version code for FAE
        {FP_CONFIG_VALUE_FAE_CUSTOMER_ID,                 FAE_CUSTOMER_ID},         //customer id for FAE
        {FP_CONFIG_VALUE_FAE_PRODUCT_ID,                   FAE_PRODUCT_ID},         //product id for FAE

        {NULL,                                                          0},         //do not remove this line, and make it be the last line of this table
    };
    extern void rename_hal_module_id(const char *hal_new_name);
}

static char fp_debug_base_dir[PATH_MAX];

void customer_generate_base_dir(void)
{
    char startup_time[22];
    memset(fp_debug_base_dir, 0, sizeof(fp_debug_base_dir));
    snprintf(fp_debug_base_dir, sizeof(fp_debug_base_dir), "/sdcard/fp_data/fp_debug_%s",
             get_current_timestamp(startup_time, sizeof(startup_time)));
    LOGD(FPTAG"customer_generate_base_dir invoked,fp_debug_base_dir->%s", fp_debug_base_dir);
}

void customer_on_fp_init(void *user_data)
{
    customer_callback.size = sizeof(fp_event_callback_t);
    customer_callback.priv_user_data = user_data;
    customer_generate_base_dir();
//for FAE additional info print
    LOGI(FPTAG"FAE additional info output:");
    LOGI(FPTAG"CA FAE BUILD INFO   sub  version:0x%x", FAE_VERSION_SUB);
    LOGI(FPTAG"CA FAE BUILD INFO   customer id :0x%x", FAE_CUSTOMER_ID);
    LOGI(FPTAG"CA FAE BUILD INFO   product  id :0x%x", FAE_PRODUCT_ID);

    return;
}

//report_code: the msg code report to UI
//report_type: 0, acquire msg,continue enroll; !=0, error msg, exit from enroll
int32_t customer_on_enroll_duplicate(int32_t *report_code, int32_t *report_type)
{
    if (!report_code || !report_type)
    {
        return -EINVAL;
    }

    *report_code = 0;
    *report_type = 0;
    return 0;
}

//caution:feature depond on algorithm configuration, even the report_code set to no zero
int32_t customer_on_enroll_finger_same_area(int32_t *report_code)
{
    if (!report_code)
    {
        return -EINVAL;
    }

    *report_code = 0;
    return 0;
}

const char *customer_get_dummytee_low_level_hal_name(void)
{
    extern bool is_64bit_system(void);
    if (is_64bit_system())
    {
        return "/system/vendor/lib64/hw/fpsensor_module.default.so";
    }
    else
    {
        return "/system/vendor/lib/hw/fpsensor_module.default.so";
    }
    return NULL;
}

//do not change this part-----------------------------------------------------------------
const char *customer_get_fp_debug_base_dir(void)
{
    return fp_debug_base_dir;
}

fp_event_callback_t customer_callback =
{
    .version = 0,
    .size = 0,
    {0, 0},
    .priv_user_data = 0,
    .on_fp_init = customer_on_fp_init,

    .on_enroll_duplicate = customer_on_enroll_duplicate,
    .on_enroll_finger_same_area = customer_on_enroll_finger_same_area,
    .on_fp_deinit = NULL,
    .on_generate_img_file_name = NULL,
    .get_fp_debug_base_dir = customer_get_fp_debug_base_dir,
    .generate_debug_base_dir = customer_generate_base_dir,
    .get_l_hal_dir = NULL,
    {0},
};

#define FINGERPRINT_HAL_MODULE_ID  "fingerprint"
static __attribute__((constructor(101))) void init_function_for_fae(void)
{
    LOGD(FPTAG"force set hal module id to %s", FINGERPRINT_HAL_MODULE_ID);
    rename_hal_module_id(FINGERPRINT_HAL_MODULE_ID);
}
//do not change this part-----------------------------------------------------------------
