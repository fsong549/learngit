#ifndef FP_COMMON_EXTERNAL_H
#define FP_COMMON_EXTERNAL_H


#define FP_CONFIG_REPORT_DETAIL_MSG                     "fp_config_report_detail_msg"
#define FP_CONFIG_FEATURE_NAVIGATOR                     "fp_config_feature_navigator"
#define FP_CONFIG_FEATURE_AUTH_SUCCESS_CONTINUOUS       "fp_config_feature_auth_success_continuous"
#define FP_CONFIG_RECORD_STATISTICAL                    "fp_config_record_statistical"
#define FP_CONFIG_STORE_CAPTURED_IMG                    "fp_config_store_captured_img"
#define FP_CONFIG_MAX_ENROLL_FINGER_SLOT_NUMBER         "fp_config_max_enroll_finger_slot_number"
#define FP_CONFIG_FEATURE_AUTH_FIRST_ENTER_NO_WAIT_UP   "fp_config_feature_auth_first_enter_nowait_up"
#define FP_CONFIG_FEATURE_AUTH_UNMATCH_RETRY            "fp_config_feature_auth_unmatch_retry"
#define FP_CONFIG_FEATURE_AUTH_UNMATCH_RETRY_TIMES      "fp_config_feature_auth_unmatch_retry_times"
#define FP_CONFIG_FEATURE_AUTH_HAL_CONTROL_UPDATE_TPLT  "fp_config_feature_auth_hal_control_update_tplt"
#define FP_CONFIG_FEATURE_AUTH_HAL_CONTROL_STORE_TIME   "fp_config_feature_auth_hal_control_store_time"
#define FP_CONFIG_FEATURE_AUTH_RETRY_ADDITIONAL_2S      "fp_config_feature_auth_retry_additional_2s"
#define FP_CONFIG_FEATURE_ENHANCEMENT_BMP_BEFORE_SAVE   "fp_config_feature_enhancement_bmp_before_save"
#define FP_CONFIG_FEATURE_ENABLE_EXT_SVC2               "fp_config_feature_enable_ext_svc2"
#define FP_CONFIG_FEATURE_INTERRUPT_STATUS_REG          "fp_config_feature_interrupt_status_reg"
#define FP_CONFIG_VALUE_ENROLL_IMG_QUALITY_BAD_RPT_CODE "fp_config_value_enroll_img_quality_bad_rpt_code"
#define FP_CONFIG_FEATURE_ENABLE_SYNC_XML               "fp_config_feature_enable_sync_xml"
#define FP_CONFIG_FEATURE_DISABLE_AUTHORIZE_ENROLL      "fp_config_feature_disable_authorize_enroll"
#define FP_CONFIG_VALUE_FAE_VERSION_SUB                 "fp_config_value_fae_version_sub"
#define FP_CONFIG_VALUE_FAE_CUSTOMER_ID                 "fp_config_value_fae_customer_id"
#define FP_CONFIG_VALUE_FAE_PRODUCT_ID                  "fp_config_value_fae_product_id"


#define NAV_ENABLE 1
#define NAV_DISABLE 0
#define EANBLE 1
#define DISABLE 0

#ifndef KEY_DIRECTION
#include "linux-event-codes.h"
#endif

typedef struct gesture_config
{
    uint32_t enable_direction;
    uint32_t enable_long_press;
    uint32_t enable_double_click;
    uint32_t long_press_time_threshold;
    uint32_t double_click_time_threshold;
    uint32_t enable_touch;
    uint32_t up_threshold;
    uint32_t down_threshold;
    uint32_t left_threshold;
    uint32_t right_threshold;
    uint32_t two_dimensional_nav;
    uint32_t report_touch_by_timer;
} gesture_config_t;
extern  gesture_config_t gesture_config ;
extern  const int32_t gKeys[7];


typedef struct fp_config_item
{
    const char *item_desc;
    int32_t config_value;
} fp_config_item_t;


typedef struct fp_event_callback
{
    int32_t version;
    int32_t size;
    int32_t dummy[2];
    void *priv_user_data;

    void (*on_fp_init)(void *user_data);

    //report_type: 0 acquire, continue enroll; != 0 means error, exit enroll progress
    int32_t (*on_enroll_duplicate)(int32_t *report_code, int32_t *report_type);
    //report_code: 0 not report to user, otherwise report code to UI
    int32_t (*on_enroll_finger_same_area)(int32_t *report_code);
    void (*on_fp_deinit)(void);
    const char *(*on_generate_img_file_name)(uint8_t *img_suffix);
    const char *(*get_fp_debug_base_dir)(void);
    void (*generate_debug_base_dir)(void);
    const char *(*get_l_hal_dir)(void);
    void (*reserved_function[15])(void);
} fp_event_callback_t;

extern fp_event_callback_t customer_callback;
extern char *get_current_timestamp(char *buf, int len);


#endif
