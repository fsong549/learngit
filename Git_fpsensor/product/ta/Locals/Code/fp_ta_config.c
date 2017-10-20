#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "fpsensor_config.h"
#include <string.h>
#include "fp_internal_api.h"

#define LOGTAG "fp_ta_config.c "

fp_config_t *fp_config = NULL;



#define ALGO_NEO_SQ_XS "neosq_xs"
#define ALGO_NEO_SQ_XS_SPEED "neosq_xs_speed"
#define ALGO_NEO_SQ_XXS "neosq_xxs"
#define ALGO_NEO_SQ_XXS_SPEED "neosq_xxs_speed"
#define ALGO_NEO_RE_S "neo_rectangular_s"

#define ALGO_CARDO_SQ_S "cardo_square_s"
#define ALGO_CARDO_RE_M "cardo_rectangular_m"
#define ALGO_CARDO_SQ_XS "cardo_square_xs"
#define ALGO_CARDO_SQ_XXS "cardo_square_xxs"
#define ALGO_CARDO_RE_S "cardo_rectangular_s"


int32_t set_config_for_723x()
{
    /*algorithm_config*/
    char algo[] = ALGO_CARDO_RE_S;
    // for 723x algorithm should be cardo_rectangular_s, and secure_level should setup to at least 2M,
    // speed mode is not supported,
    // preprocessor should be set to 0 when applying the denoise method on glass-covered sensor;
    SET_MEMBER_COPY(fp_config->algorithm, alg_name, algo, sizeof(algo));
    SET_MEMBER(fp_config->algorithm, mt_size                        , 30);
    SET_MEMBER(fp_config->algorithm, mt_size_bytes                  , 0);
    SET_MEMBER(fp_config->algorithm, enr_samples                    , 12);
    SET_MEMBER(fp_config->algorithm, secure_level                   , FAR_20M);
    SET_MEMBER(fp_config->algorithm, enrollment_filter_enable       , 0);
    SET_MEMBER(fp_config->algorithm, enrollment_qualtiy_threshold   , 40);
    SET_MEMBER(fp_config->algorithm, enrollment_area_threshold      , 0);
    SET_MEMBER(fp_config->algorithm, enrollment_condition_threshold , 10);
    SET_MEMBER(fp_config->algorithm, verification_filter_enable  ,
               /*FINGER_SKIN_CLASS_PB | FINGER_SKIN_CLASS_GLCM*/ 0);
    SET_MEMBER(fp_config->algorithm, verification_qualtiy_threshold , 40);
    SET_MEMBER(fp_config->algorithm, verification_area_threshold    , 18);
    SET_MEMBER(fp_config->algorithm, enable_preprocessor            ,
               0); // 1:pb_preprocess 2:local_preprocess
    SET_MEMBER(fp_config->algorithm, lock_templates_after_enroll            ,
               1); //1:prevent enrolled templates from being removed by further update operations

    /*autogain_config*/
    SET_MEMBER(fp_config->autogain, autogain_timeout_ms, 600);
    SET_MEMBER(fp_config->autogain, init_gain          , 8);
    SET_MEMBER(fp_config->autogain, init_shift         , 7);
    SET_MEMBER(fp_config->autogain, init_pixel_control , 4);
    SET_MEMBER(fp_config->autogain, init_cbase         , 511);
    SET_MEMBER(fp_config->autogain, enable_post_check  , 0);
    SET_MEMBER(fp_config->autogain, accumulate_times   , 16);
    SET_MEMBER(fp_config->autogain, fifo_16bit         , 1);

    fp_config->mark_filter_enable = ENABLE;
    return 0;
}

int32_t set_config_for_7222()
{
    /*algorithm_config*/
    // for 7222 algorithm must be cardo_square_xxs, and secure_level should setup to at least 1000M,
    // speed mode is not supported;
    char algo[] = ALGO_CARDO_SQ_XXS;
    SET_MEMBER_COPY(fp_config->algorithm, alg_name, algo, sizeof(algo));
    SET_MEMBER(fp_config->algorithm, mt_size                        , 30);
    SET_MEMBER(fp_config->algorithm, mt_size_bytes                  , 0);
    SET_MEMBER(fp_config->algorithm, enr_samples                    , 12);
    SET_MEMBER(fp_config->algorithm, secure_level                   , FAR_1000M);
    SET_MEMBER(fp_config->algorithm, enrollment_filter_enable       , 0);
    SET_MEMBER(fp_config->algorithm, enrollment_qualtiy_threshold   , 40);
    SET_MEMBER(fp_config->algorithm, enrollment_area_threshold      , 0);
    SET_MEMBER(fp_config->algorithm, enrollment_condition_threshold , 10);

    SET_MEMBER(fp_config->algorithm, enable_preprocessor            ,
               2); // 1:pb_preprocess 2:local_preprocess
    SET_MEMBER(fp_config->algorithm, lock_templates_after_enroll    ,
               1); //1:prevent enrolled templates from being removed by further update operations

    /*setup wrong trigger threshold*/
    SET_MEMBER(fp_config->algorithm, verification_filter_enable      ,
               FINGER_SKIN_CLASS_PB | FINGER_SKIN_CLASS_GLCM);
    SET_MEMBER(fp_config->algorithm, verification_qualtiy_threshold , 20);
    SET_MEMBER(fp_config->algorithm, verification_area_threshold , 6);
    SET_MEMBER(fp_config->algorithm, verification_condition_threshold , 10);
    SET_MEMBER(fp_config->algorithm, verification_skin_threshold , 500);
    SET_MEMBER(fp_config->algorithm, verification_artificial_threshold , 3000);

    /*autogain_config*/
    SET_MEMBER(fp_config->autogain, autogain_timeout_ms, 200);
    SET_MEMBER(fp_config->autogain, init_gain          , 5);
    SET_MEMBER(fp_config->autogain, init_shift         , 13);
    SET_MEMBER(fp_config->autogain, init_pixel_control , 7);
    SET_MEMBER(fp_config->autogain, init_cbase         , 0);
    SET_MEMBER(fp_config->autogain, enable_post_check  , 0);
    SET_MEMBER(fp_config->autogain, accumulate_times   , 2);
    SET_MEMBER(fp_config->autogain, fifo_16bit         , 0);
    return 0;
}

int32_t set_config_for_715x()
{
    /*algorithm_config*/

    // 715x coating, pb_preprocess is recommanded
    // for 715x algorithm (with local_preprocess) can be :
    // neosq_xs, and secure_level: 5M
    // neosq_xs_speed, and secure_level: 10M
    // cardo_square_xs, secure_level: 2M (better performance, better waterproof, more template is needed);
    char algo[] = ALGO_NEO_SQ_XS;
    SET_MEMBER_COPY(fp_config->algorithm, alg_name, algo, sizeof(algo));
    SET_MEMBER(fp_config->algorithm, mt_size                        , 30);
    SET_MEMBER(fp_config->algorithm, mt_size_bytes                  , 0);
    SET_MEMBER(fp_config->algorithm, enr_samples                    , 12);
    SET_MEMBER(fp_config->algorithm, secure_level                   , FAR_5M);
    SET_MEMBER(fp_config->algorithm, enrollment_filter_enable       , 1);
    SET_MEMBER(fp_config->algorithm, enrollment_qualtiy_threshold   , 40);
    SET_MEMBER(fp_config->algorithm, enrollment_area_threshold      , 12);
    SET_MEMBER(fp_config->algorithm, enrollment_condition_threshold , 10);

    SET_MEMBER(fp_config->algorithm, enable_preprocessor            ,
               2); // 0:none 1:pb_preprocess 2:local_preprocess
    SET_MEMBER(fp_config->algorithm, lock_templates_after_enroll            ,
               1); //1:prevent enrolled templates from being removed by further update operations

    /*setup wrong trigger threshold*/
    SET_MEMBER(fp_config->algorithm, verification_filter_enable      ,
               FINGER_SKIN_CLASS_PB | FINGER_SKIN_CLASS_GLCM);
    SET_MEMBER(fp_config->algorithm, verification_qualtiy_threshold , 20);
    SET_MEMBER(fp_config->algorithm, verification_area_threshold , 21);
    SET_MEMBER(fp_config->algorithm, verification_condition_threshold , 10);
    SET_MEMBER(fp_config->algorithm, verification_skin_threshold , 2600);
    SET_MEMBER(fp_config->algorithm, verification_artificial_threshold , 5800);

    /*autogain_config*/
    SET_MEMBER(fp_config->autogain, autogain_timeout_ms, 200);
    SET_MEMBER(fp_config->autogain, init_gain          , 7);
    SET_MEMBER(fp_config->autogain, init_shift         , 22);
    SET_MEMBER(fp_config->autogain, init_pixel_control , 0x00);
    SET_MEMBER(fp_config->autogain, init_cbase         , 0);
    SET_MEMBER(fp_config->autogain, enable_post_check  , 0);
    SET_MEMBER(fp_config->autogain, accumulate_times   , 2);
    SET_MEMBER(fp_config->autogain, fifo_16bit         , 0);

    return 0;
}

int32_t fp_ta_config_init(uint32_t hw_id)
{
    int32_t status = 0;

    if (NULL != fp_config)
    {
        LOGE(LOGTAG"ERROR: fp_config is not NULL!!");
        return -FP_ERROR_CONFIG;
    }

    fp_config = fp_malloc(sizeof(fp_config_t));
    if (NULL == fp_config)
    {
        return -FP_ERROR_ALLOC;
    }
    //--------feature--------------------------(value only 1 or 0)
    fp_config->feature.enrol_same_finger_detect     = 1;
    fp_config->feature.enrol_same_area_detect       = 1;
    //--------wait_finger_adc----------------
    /*capture wait finger down*/
    fp_config->wait_finger_adc[0].gain              = 5;
    fp_config->wait_finger_adc[0].shift             = 12;
    fp_config->wait_finger_adc[0].pxl_ctrl          = 0;
    fp_config->wait_finger_adc[0].threadhold        = 0xe0;
    fp_config->wait_finger_adc[0].sleep_dect        = 1;
    fp_config->wait_finger_adc[0].adjust            = NOT_ADJUST;
    fp_config->wait_finger_adc[0].esd_reset         = ENABLE;

    /*gesture wait finger down*/
    fp_config->wait_finger_adc[1].gain              = 5;
    fp_config->wait_finger_adc[1].shift             = 9;
    fp_config->wait_finger_adc[1].pxl_ctrl          = 0;
    fp_config->wait_finger_adc[1].threadhold        = 0x60;
    fp_config->wait_finger_adc[1].sleep_dect        = 1;
    fp_config->wait_finger_adc[1].adjust            = NOT_ADJUST;
    fp_config->wait_finger_adc[1].esd_reset         = ENABLE;

    /*gesture_config*/
    fp_config->gesture.enable_direction             = NAV_ENABLE;

    /*HW cac enabel*/
    fp_config->hw_cac_enable                        = DISABLE;  //enable just for 7253

    /*checkboard */
    fp_config->max_deviation                        = 40;
    fp_config->max_dead_pixels                      = 11;
    fp_config->max_dead_pixels_in_key_zone          = 2;    //value~[2,8]


    /*sensor config*/
    switch (hw_id)
    {
        case 0x7153:
            status = set_config_for_715x();
            break;
        case 0x7230:
            status = set_config_for_723x();
            break;
        case 0x7222:
            status = set_config_for_7222();
            break;
        default:
            LOGE(LOGTAG"Unknown hw id: 0x%x, goto default: set_config_for_715x()" , hw_id);
            status = set_config_for_715x();
            break;
    }

    return status;
}


int32_t fp_ta_config_deinit(void)
{
    fp_free(fp_config);
    fp_config = NULL;

    return 0;
}

//---------------------here just for algo compile symbol-------------
#include <stdarg.h>
void logd_extra(const char *fmt, ...)
{

    va_list args;
    va_start(args, fmt);
    char buf[512] = {0};
#ifndef FP_TEE_TBASE
    vsnprintf(buf, 512, fmt, args);
#endif

#if defined(FP_TEE_TBASE)
    tlApiLogPrintf("[fpCoreAlgo]");
    tlApiLogvPrintf(fmt, args);
    tlApiLogPrintf("\n");
#elif defined(FP_TEE_DUMMYTEE)
    __android_log_write(ANDROID_LOG_DEBUG, "[fpCoreAlgo]", buf);
#elif defined(FP_TEE_QSEE4)
    qsee_log(QSEE_LOG_MSG_ERROR, buf);
#else
    LOGD_ALGO("[fpCoreAlgo]");
    LOGD_ALGO(buf);
    LOGD_ALGO("\n");
#endif
    va_end(args);

}
//-----------------------------------------------------------------------

#ifdef DUMMYTEE_
extern int32_t dummy_link();
void config_dummy_link()
{
    dummy_link();
}
#endif
