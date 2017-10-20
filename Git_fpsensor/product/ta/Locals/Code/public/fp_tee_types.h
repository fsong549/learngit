#ifndef FP_TEE_TYPES_H_
#define FP_TEE_TYPES_H_

#include "stdint.h"
#include <stdlib.h>
#include <stdbool.h>

//-----------------------------
#define VERSION_BUFFER_LEN 32
#define TA_VERSION "TA-V1.2.0 "
#define DRIVER_VERSION "DR-V1.0.1_1018"
// #define ALGO_VERSION "AG-V1.0.1-20160616"

#define FINGER_MAX_COUNT (5)
#define TA_PATH_MAX     (512)
#define ALG_NAME_LEN (64)
#define COMMIT_ID_LEN (64)

typedef  int32_t (*taGetRspCunc)(char *pBuffer, int32_t iLen, void *pCmdProcessor);
extern int32_t tlcFunc(unsigned char *cmd, int32_t len, taGetRspCunc pRspFunc,
                       void *pProcessorInstance);
extern int32_t taOpen(void);
extern void taClose(void);
extern const char *get_cmd_str_from_cmd_id(int32_t iCmd);
#ifdef FP_TEE_QSEE4
extern void fp_qsee_km_release_encapsulated_key(uint8_t *encapsulated_key);
extern int fp_qsee_km_get_encapsulated_key(uint8_t **encapsulated_key,
                                           uint32_t *size_encapsulated_key);
#endif

typedef int32_t(*command_processor_func)(void *p_buffer_in, void *p_buffer_out, int32_t *p_rsp_len);
typedef struct
{
    int32_t cmd_id;
    command_processor_func funcPtr;
    const char *p_cmd_desc;
} command_item_t;

typedef struct
{
    int32_t cmd_id;
    int32_t table_idx;
} cmd_cache_map;

typedef enum
{
    FP_LIB_OK,                           //0
    FP_LIB_FINGER_LOST,                  //6
    FP_LIB_ERROR_TOO_FAST,               //7
    FP_LIB_ERROR_TOO_SLOW,               //8
    FP_LIB_ERROR_GENERAL,                //9
    FP_LIB_ERROR_SENSOR,                 //10
    FP_LIB_ERROR_MEMORY,                 //11
    FP_LIB_ERROR_PARAMETER,              //12
    FP_LIB_FAIL_LOW_QUALITY,
    FP_LIB_FAIL_IDENTIFY_START,
    FP_LIB_FAIL_IDENTIFY_IMAGE,


    FP_ERROR_BASE = 500,
    FP_ERROR_INPUT,
    FP_ERROR_TIMEDOUT,
    FP_ERROR_ENOMEM,
    FP_ERROR_ALLOC,
    FP_ERROR_COMM,
    FP_ERROR_NOSPACE,
    FP_ERROR_IO,
    FP_ERROR_CANCELLED,
    FP_ERROR_NOENTITY,
    FP_ERROR_HARDWARE,
    FP_ERROR_CONFIG,
    FP_ERROR_UNKNOWN_CMD,
    FP_ERROR_ALGO_VER,

    //these enum val hasn't been used
    FP_TA_ERROR_CODE_BASE = 0xFFFF6000,
    FP_TA_ERROR_TA_NOT_INIT,
    FP_TA_ERROR_STATE,
    FP_TA_ERROR_LIB_INIT_FAIL,
    FP_TA_ERROR_INIT_ALG_PPLIB_FAIL,
    FP_TA_ERROR_ENROLL_EXCEED_MAX_FINGERPIRNTS,
    FP_TA_ERROR_ENROLL_NOT_COMPLETED,
    FP_TA_ERROR_ENROLL_GET_TEMPLATE_FAIL,
    FP_TA_ERROR_ENROLL_PACK_TEMPLATE_FAIL,
    FP_TA_ERROR_ENORLL_START_FAIL,
    FP_TA_ERROR_ENORLL_ADD_IMAGE,

} fp_lib_return_t;

typedef enum
{
    FP_LIB_ENROLL_SUCCESS,
    FP_LIB_ENROLL_HELP_SAME_AREA,
    FP_LIB_ENROLL_HELP_TOO_WET,
    FP_LIB_ENROLL_HELP_ALREADY_EXIST,
    // FP_LIB_ENROLL_TOO_MANY_ATTEMPTS, //no used
    // FP_LIB_ENROLL_TOO_MANY_FAILED_ATTEMPTS, //no used
    FP_LIB_ENROLL_FAIL_NONE,
    FP_LIB_ENROLL_FAIL_LOW_QUALITY,
    FP_LIB_ENROLL_FAIL_LOW_COVERAGE,
    FP_LIB_ENROLL_FAIL_LOW_QUALITY_AND_LOW_COVERAGE,
} fp_lib_enroll_result_t;

/*
 * Struct with data from an enroll attempt
 */
typedef struct
{
    /* Progress of the current enroll process in percent */
    uint32_t progress;
    /* Quality for the image*/
    uint32_t quality;
    /* Status of current enroll attempt */
    fp_lib_enroll_result_t result;
    /* Number of successful enroll attempts so far */
    uint32_t nr_successful;
    /* Number of total enroll require numbers */
    uint32_t nr_total;
    /* Number of failed enroll attempts so far */
    uint32_t nr_failed;
    /* Size of the enrolled template */
    // uint32_t enrolled_template_size;
    /* Size of the data part in the structure used for extended enroll */
    // uint32_t extended_enroll_size;
    /* Coverage of the image*/
    uint32_t coverage;
    /* Used to indicate that touches are too similar */
    // int8_t user_touches_too_immobile;
    // int8_t guide_direction;
} fp_lib_enroll_data_t;


typedef enum
{
    FP_LIB_IDENTIFY_NO_MATCH_NO_SKIN       = -2,
    FP_LIB_IDENTIFY_NO_MATCH               = 0,
    FP_LIB_IDENTIFY_MATCH_UPDATED_TEMPLATE = 1,
    FP_LIB_IDENTIFY_MATCH                  = 2,
} fp_lib_identify_result_t;


typedef enum
{
    MODE_FINGER_UP      = 0,
    MODE_FINGER_DOWN    = 1,
    MODE_NAV_DOWN       = 2,
} fp_wait_finger_mode_t;
/* Data from the identification attempt */
typedef struct
{
    /* Result of the identification attempt */
    fp_lib_identify_result_t result;
    /* Matching score */
    int32_t score;
    /* Index of the identification template */
    uint32_t index;
    /* Size of the update template if one exits */
    uint32_t updated_template_size;
    /*  Coverage */
    int coverage;
    /* Quality */
    int quality;
} fp_lib_identify_data_t;


typedef enum
{
    ALGO_VER_R5H    = 0,
    ALGO_VER_R5L    = 1,
    ALGO_VER_R6A    = 2,
    ALGO_VER_R7A    = 3,
    ALGO_VER_MAX,
} algo_enum_t;

typedef struct
{
    algo_enum_t ver;
    char *string;
} fp_algo_ver_t;

typedef enum
{
    /* Start the initialize the TA */
    TCI_FP_CMD_INIT                     = 0x01,
    TCI_FP_CMD_DEINIT                   = 0x02,
    TCI_FP_CMD_SET_ACTIVE_GROUP         = 0x03,
    TCI_FP_CMD_PRE_ENROLL               = 0x04,
    TCI_FP_CMD_REMOVE                   = 0x05,
    TCI_FP_CMD_STORETEMPLATE            = 0x06,
    TCI_FP_CMD_WAIT_FOR_FINGER          = 0x07,
    TCI_FP_CMD_CHECK_FINGER_PRESENT     = 0x08,
    TCI_FP_CMD_CAPTURE_IMAGE            = 0x09,
    TCI_FP_CMD_START_ENROLL             = 0x0a,
    TCI_FP_CMD_ENROLL_IMG               = 0x0b,
    TCI_FP_CMD_STOP,
    TCI_FP_CMD_BEGIN_AUTHENTICATE,
    TCI_FP_CMD_AUTHENTICATE,
    TCI_FP_CMD_END_AUTHENTICATE,
    TCI_FP_CMD_GET_ENROLLED_FIDS,
    TCI_FP_CMD_GET_REL_COORDS,

    TCI_FP_CMD_GET_IMAGE_QUALITY,
    TCI_FP_CMD_GET_TEMPLATE_IDS,
    TCI_FP_CMD_SET_PROPERTY,
    TCI_FP_CMD_GET_SUBRECT_COUNT,
    TCI_FP_CMD_GET_SUBRECT,
    TCI_FP_CMD_GET_IMGAGE_DIMENTION,
    TCI_FP_CMD_CHECKBOARD_TEST,
    TCI_FP_CMD_GET_VERSION,
    TCI_FP_CMD_AUTHORIZE_ENROL,
    TCI_FP_CMD_GET_TEMPLATE_DB_ID,
    TCI_FP_CMD_WAKEUP,

    TCI_FP_CMD_GET_LAST_IDENTIFY_ID,
    TCI_FP_CMD_GET_AUTHENTICATOR_VERSION,
    TCI_FP_CMD_READ_REG,
    TCI_FP_CMD_GET_TEE_INFO,
    TCI_FP_CMD_INJECT,
    TCI_FP_CMD_END_ENROLL,
    TCI_FP_CMD_GET_TEMPLATE_SIZE,
    TCI_FP_CMD_LOAD_TEMPLATE,
    TCI_FP_CMD_SET_AUTH_TOKEN_KEY,
    TCI_FP_CMD_STORE_SPLIT_BMP,
    TCI_FP_CMD_UPDATE_TEMPLATE,
    TCI_FP_CMD_GET_TEMPLATE_COUNT,
    TCI_FP_CMD_TA_CONFIG,
    TCI_FP_CMD_RETRIVE_IMAGE,
    TCI_FP_CMD_DEBUG_FINGER_DETECT,
    TCI_FP_CMD_DEEP_SLEEP,
    TCI_FP_CMD_DBG_FINGER_STATUS_VERFICAE,
    TCI_FP_CMD_SEND_BLOCK_TO_TA,
    TCI_FP_CMD_MAX,
} tci_fp_cmd_t;

//-------------------------------------------cmd msg

typedef struct fp_cmd_no_payload
{
    int32_t cmd_id;
} fp_cmd_no_payload_t;


typedef struct fp_cmd_set_active_group
{
    int32_t cmd_id;
    int32_t igid;
    uint8_t path[TA_PATH_MAX];
} fp_cmd_set_active_group_t;

typedef struct fp_cmd_byte_array
{
    int32_t cmd_id;
    uint32_t total_len;
    uint32_t chunk_data_size;
    uint8_t data;
} fp_cmd_byte_array_t;

typedef struct fp_cmd_authorize_enrol
{
    int32_t cmd_id;
    int32_t hat_size;
    int8_t hat;
} fp_cmd_authorize_enrol_t;

typedef struct fp_cmd_gid_fid
{
    int32_t cmd_id;
    int32_t igid;
    uint32_t ifid;
} fp_cmd_gid_fid_t;

typedef struct fp_cmd_one_para
{
    int32_t cmd_id;
    int32_t para;
} fp_cmd_one_para_t;

typedef struct fp_cmd_begin_auth
{
    int32_t cmd_id;
    // uint32_t indices_count;
    uint64_t challenge;
} fp_cmd_begin_auth_t;

// typedef struct fp_cmd_authenticate
// {
//     int32_t cmd_id;
//     int64_t challengID;
//     int32_t igid;
//     int32_t forEnrollCheck;
// } fp_cmd_authenticate_t;


typedef struct fp_cmd_rel_coords
{
    int32_t cmd_id;
    int32_t start;
} fp_cmd_rel_coords_t;

typedef struct fp_cmd_set_property
{
    int32_t cmd_id;
    uint32_t uTag;
    int32_t iValue;
} fp_cmd_set_property_t;

typedef struct fp_cmd_get_sub_rect
{
    int32_t cmd_id;
    int32_t iIndex;
} fp_cmd_get_sub_rect_t;

typedef struct fp_cmd_get_template_ids
{
    int32_t cmd_id;
    int32_t cntTemplate;
} fp_cmd_get_template_ids_t;

typedef struct fp_cmd_inject
{
    int32_t cmd_id;
    int32_t dummy;
    int32_t data_len;
#ifdef FP_TEE_SPREADTRUM
    uint8_t data[16 * 16];
#else
    uint8_t data[160 * 160];
#endif
} fp_cmd_inject_t;

typedef struct fp_cmd_array_para
{
    int32_t cmd_id;
    uint32_t size;
    uint8_t array[];
} fp_cmd_array_para_t;

typedef struct fp_cmd_ta_feature_config
{
    int32_t cmd_id;
    uint32_t enrol_same_finger_detect;
    uint32_t enrol_same_area_detect;
} fp_cmd_ta_feature_config_t;


typedef struct fp_Cmd
{
    union
    {
        fp_cmd_no_payload_t         no_payload_cmd;
        fp_cmd_set_active_group_t   set_active_group_cmd;
        fp_cmd_gid_fid_t            gid_fid_cmd;
        // fp_cmd_authenticate_t       authenticate_cmd;
        fp_cmd_rel_coords_t         rel_coords_cmd;
        fp_cmd_set_property_t       set_property_cmd;
        fp_cmd_get_sub_rect_t       get_sub_rect_cmd;
        fp_cmd_get_template_ids_t   get_template_ids_cmd;
        fp_cmd_one_para_t           one_para_cmd;
        fp_cmd_begin_auth_t         begin_auth_cmd;
        fp_cmd_authorize_enrol_t    authorize_enrol_cmd;
        fp_cmd_inject_t             inject_cmd;
        fp_cmd_byte_array_t         load_template_cmd;
        fp_cmd_inject_t             set_auth_token_key_cmd;
        fp_cmd_ta_feature_config_t  ta_config_cmd;
    } c;
} fp_cmd_t;



//-------------------------------------------resp msg
typedef struct fp_rsp_no_payload
{
    int32_t rsp_id;
} fp_rsp_no_payload_t;


// typedef struct fp_rsp_capture_img
// {
//     int32_t rsp_id;
//     int32_t captureResult;
//     int32_t imgWidth;
//     int32_t imgHeight;
//     int8_t   imgBufStub;
// } fp_rsp_capture_img_t;



typedef struct fp_rsp_enroll_img
{
    int32_t rsp_id;
    int32_t TotalEnrollCnt;
    fp_lib_enroll_data_t enroll_data;
} fp_rsp_enroll_img_t;


typedef struct fp_rsp_pre_enroll
{
    int32_t rsp_id;
    uint64_t changlleID;
} fp_rsp_pre_enroll_t;

typedef struct fp_rsp_get_database_id
{
    int32_t rsp_id;
    uint64_t TplDataBaseID;
} fp_rsp_get_database_id_t;

typedef struct fp_rsp_get_last_finger_id
{
    int32_t rsp_id;
    int32_t FingerID;
} fp_rsp_get_last_finger_id_t;

typedef struct fp_rsp_get_authen_ver
{
    int32_t rsp_id;
    int32_t AuthenVer;
} fp_rsp_get_authen_ver_t;

typedef struct fp_rsp_authenticate
{
    int32_t rsp_id;
    fp_lib_identify_data_t identify_data;
    uint32_t matchFid;
    uint32_t hat_size;
    uint8_t  hat;

} fp_rsp_authenticate_t;

// typedef struct fp_rsp_end_authenticate
// {
//     int32_t rsp_id;
//     int32_t update_flag;
// //lzk add for miui auth staticstal requirements 2016.9.23
//     uint32_t match_fid;
//     uint32_t match_tplt_len;
//     int32_t match_score;
//     int32_t match_result;
// } fp_rsp_end_authenticate_t;

typedef struct fp_rsp_one_par
{
    int32_t rsp_id;
    uint32_t par;
} fp_rsp_one_par_t;

typedef struct fp_rsp_byte_array
{
    int32_t rsp_id;
    uint32_t remain_size;
    uint32_t array_len;
    uint8_t array[];
} fp_rsp_byte_array_t;

typedef struct fp_rsp_check_finger_present
{
    int32_t rsp_id;
    int16_t FingerPresentSum;
} fp_rsp_check_finger_present_t;


typedef struct fp_rsp_get_enrol_fids
{
    int32_t rsp_id;
    int32_t Fids[FINGER_MAX_COUNT];
    int32_t FidCnt;
} fp_rsp_get_enrol_fids_t;

typedef struct fp_rsp_rel_coords
{
    int32_t rsp_id;
    int32_t GetRelCoordsRes;
    int32_t DeltaX;
    int32_t DeltaY;
} fp_rsp_rel_coords_t;
//-0---------------------------------------------
typedef struct fp_rsp_get_image_quality
{
    int32_t rsp_id;
    int32_t Area;
    int32_t Quality;
    int32_t Condition;
} fp_rsp_get_image_quality_t;

typedef struct fp_rsp_get_template_ids
{
    int32_t rsp_id;
    int32_t cntTemplate;
    int32_t Ids[1];
} fp_rsp_get_template_ids_t;

typedef struct fp_rsp_get_sub_rect_cnt
{
    int32_t rsp_id;
    int32_t Count;
} fp_rsp_get_sub_rect_cnt_t;

typedef struct fp_rsp_get_sub_rect
{
    int32_t rsp_id;
    int32_t Rect[12];
} fp_rsp_get_sub_rect_t;

typedef struct fp_rsp_get_image_dimention
{
    int32_t rsp_id;
    int32_t Width;
    int32_t Height;
    int32_t bit_width;
} fp_rsp_get_image_dimention_t;


typedef struct fp_rsp_read_reg
{
    int32_t rsp_id;
    int32_t reg;
} fp_rsp_read_reg_t;




typedef struct fp_rsp_get_version
{
    int32_t rsp_id;
    char    taVersion[VERSION_BUFFER_LEN];
    char    driverVersion[VERSION_BUFFER_LEN];
    // char    algoVersion[VERSION_BUFFER_LEN];
} fp_rsp_get_version_t;


typedef struct fp_rsp_tee_info
{
    int32_t rsp_id;
    uint32_t sensor_revision;
    uint32_t sensor_hw_id;
    int32_t feature_tee_storage;
    int32_t feature_hw_auth;
    char alg_name[ALG_NAME_LEN];
    char alg_ver[ALG_NAME_LEN];
    char commit_id[COMMIT_ID_LEN];
    uint32_t feature_enrol_same_finger_detect;
    uint32_t feature_enrol_same_area_detect;
} fp_rsp_tee_info_t;

typedef struct fp_rsp_array_msg
{
    int32_t rsp_id;
    uint32_t size;
    uint8_t array[];
} fp_rsp_array_msg_t;
typedef struct fp_rsp
{
    union
    {
        fp_rsp_no_payload_t             no_payload_rsp;
        // fp_rsp_capture_img_t            capture_img_rsp;
        fp_rsp_pre_enroll_t             pre_enroll_rsp;
        fp_rsp_authenticate_t           authenticate_rsp;
        fp_rsp_enroll_img_t             enroll_img_rsp;
        fp_rsp_check_finger_present_t   check_finger_present_rsp;
        fp_rsp_get_enrol_fids_t         get_enrol_fids_rsp;
        fp_rsp_rel_coords_t             rel_coords_rsp;
        fp_rsp_get_image_quality_t      get_image_quality_rsp;
        fp_rsp_get_template_ids_t       get_template_ids_rsp;
        fp_rsp_get_sub_rect_cnt_t       get_sub_rect_cnt_rsp;
        fp_rsp_get_sub_rect_t           GetSubRectRsp;
        fp_rsp_get_image_dimention_t    GetImageDimentionRsp;
        fp_rsp_get_version_t            GetVersionRsp;
        fp_rsp_get_database_id_t        GetDataBaseIDRsp;
        fp_rsp_get_last_finger_id_t     GetLastFingerIDRsp;
        fp_rsp_get_authen_ver_t         GetAuthenVerRsp;
        fp_rsp_read_reg_t               readReg;
        fp_rsp_tee_info_t               tee_info;
        fp_rsp_byte_array_t             store_template_rsp;
        fp_rsp_one_par_t                end_enroll_rsp;
        fp_rsp_one_par_t                get_tpl_size_rsp;
        fp_rsp_one_par_t                update_template;
#ifdef FP_TEE_QSEE4
        char dummy[32 * 1024];
#endif
    } r;
} fp_rsp_t;

/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)

#endif
