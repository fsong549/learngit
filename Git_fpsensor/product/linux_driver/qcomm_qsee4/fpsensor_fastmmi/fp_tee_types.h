#ifndef FP_TEE_TYPES_H_
#define FP_TEE_TYPES_H_

#include <stdbool.h>

//-----------------------------
#define VERSION_BUFFER_LEN 32
#define TA_VERSION "TA-V1.2.0 "
#define DRIVER_VERSION "DR-V1.0.1_1018"
#define ALGO_VERSION "AG-V1.0.1-20160616"

#define FINGER_MAX_COUNT (5)
#define TA_PATH_MAX     (512)
#define ALG_NAME_LEN (64)

typedef  int32_t (*taGetRspCunc)(char *pBuffer, int32_t iLen, void *pCmdProcessor);
extern int32_t tlcFunc(unsigned char *cmd, int32_t len, taGetRspCunc pRspFunc,
                       void *pProcessorInstance);
extern int32_t taOpen(void);
extern void taClose(void);
extern const char *get_cmd_str_from_cmd_id(int32_t iCmd);


typedef int32_t(*command_processor_func)(void *p_buffer_in, void *p_buffer_out, int32_t *p_rsp_len);
typedef struct {
    int32_t cmd_id;
    command_processor_func funcPtr;
    const char *p_cmd_desc;
} command_item_t;

typedef struct {
    int32_t cmd_id;
    int32_t table_idx;
} cmd_cache_map;

enum FP_ERROR {
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
    FP_ERROR_MAX,
};

typedef enum {
    /* Start the initialize the TA */
    TCI_FP_CMD_INIT                     = 0x01,
    TCI_FP_CMD_DEINIT                   = 0x02,
    TCI_FP_CMD_SET_ACTIVE_GROUP         = 0x03,
    TCI_FP_CMD_PRE_ENROLL               = 0x04,
    TCI_FP_CMD_REMOVE                   = 0x05,
    TCI_FP_CMD_STORETEMPLATE            = 0x06,
    TCI_FP_CMD_WAIT_FINGER_DOWN         = 0x07,
    TCI_FP_CMD_CHECK_FINGER_PRESENT     = 0x08,
    TCI_FP_CMD_CAPTURE_IMAGE            = 0x09,
    TCI_FP_CMD_START_ENROLL             = 0x0a,
    TCI_FP_CMD_ENROLL_IMG               = 0x0b,
    TCI_FP_CMD_FINISH_ENROLL            = 0x0c,
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
    TCI_FP_CMD_WAIT_FINGER_UP,
    TCI_FP_CMD_MAX,
} tci_fp_cmd_t;

//-------------------------------------------cmd msg

typedef struct fp_cmd_no_payload {
    int32_t cmd_id;
} fp_cmd_no_payload_t;


typedef struct fp_cmd_set_active_group {
    int32_t cmd_id;
    int32_t igid;
    uint8_t path[TA_PATH_MAX];
} fp_cmd_set_active_group_t;

typedef struct fp_cmd_byte_array {
    int32_t cmd_id;
    uint32_t file_len;
    uint32_t chunk_data_size;
    uint8_t template_data;
} fp_cmd_byte_array_t;

typedef struct fp_cmd_authorize_enrol {
    int32_t cmd_id;
    int32_t hat_size;
    int8_t hat;
} fp_cmd_authorize_enrol_t;

typedef struct fp_cmd_gid_fid {
    int32_t cmd_id;
    int32_t igid;
    uint32_t ifid;
} fp_cmd_gid_fid_t;

typedef struct fp_cmd_one_para {
    int32_t cmd_id;
    int32_t para;
} fp_cmd_one_para_t;

typedef struct fp_cmd_begin_auth {
    int32_t cmd_id;
    uint64_t challenge;
} fp_cmd_begin_auth_t;

typedef struct fp_cmd_authenticate {
    int32_t cmd_id;
    int64_t challengID;
    int32_t igid;
    int32_t forEnrollCheck;
} fp_cmd_authenticate_t;


typedef struct fp_cmd_rel_coords {
    int32_t cmd_id;
    int32_t start;
} fp_cmd_rel_coords_t;

typedef struct fp_cmd_set_property {
    int32_t cmd_id;
    uint32_t uTag;
    int32_t iValue;
} fp_cmd_set_property_t;

typedef struct fp_cmd_get_sub_rect {
    int32_t cmd_id;
    int32_t iIndex;
} fp_cmd_get_sub_rect_t;

typedef struct fp_cmd_get_template_ids {
    int32_t cmd_id;
    int32_t cntTemplate;
} fp_cmd_get_template_ids_t;

typedef struct fp_cmd_inject {
    int32_t cmd_id;
    int32_t dummy;
    int32_t data_len;
    uint8_t data[160 * 160];
} fp_cmd_inject_t;

typedef struct fp_Cmd {
    union {
        fp_cmd_no_payload_t         no_payload_cmd;
        fp_cmd_set_active_group_t   set_active_group_cmd;
        fp_cmd_gid_fid_t            gid_fid_cmd;
        fp_cmd_authenticate_t       authenticate_cmd;
        fp_cmd_rel_coords_t         rel_coords_cmd;
        fp_cmd_set_property_t       set_property_cmd;
        fp_cmd_get_sub_rect_t       get_sub_rect_cmd;
        fp_cmd_get_template_ids_t   get_template_ids_cmd;
        fp_cmd_one_para_t           one_para_cmd;
        fp_cmd_begin_auth_t         begin_auth_cmd;
        fp_cmd_authorize_enrol_t    authorize_enrol_cmd;
        fp_cmd_inject_t             inject_cmd;
        fp_cmd_byte_array_t         load_template_cmd;
    } c;
} fp_cmd_t;



//-------------------------------------------resp msg
typedef struct fp_rsp_no_payload {
    int32_t rsp_id;
} fp_rsp_no_payload_t;


typedef struct fp_rsp_capture_img {
    int32_t rsp_id;
    int32_t captureResult;
    int32_t imgWidth;
    int32_t imgHeight;
    int8_t   imgBufStub;
} fp_rsp_capture_img_t;



typedef struct fp_rsp_enroll_img {
    int32_t rsp_id;
    int32_t LastProgress;
    int32_t TotalEnrollCnt;
    int32_t CurEnrollCnt;
    int32_t EnrollFailReason;
    int32_t FillPart;
} fp_rsp_enroll_img_t;


typedef struct fp_rsp_pre_enroll {
    int32_t rsp_id;
    uint64_t changlleID;
} fp_rsp_pre_enroll_t;

typedef struct fp_rsp_get_database_id {
    int32_t rsp_id;
    uint64_t TplDataBaseID;
} fp_rsp_get_database_id_t;

typedef struct fp_rsp_get_last_finger_id {
    int32_t rsp_id;
    int32_t FingerID;
} fp_rsp_get_last_finger_id_t;

typedef struct fp_rsp_get_authen_ver {
    int32_t rsp_id;
    int32_t AuthenVer;
} fp_rsp_get_authen_ver_t;

typedef struct fp_rsp_authenticate {
    int32_t rsp_id;
    int32_t matchResult;
    uint32_t matchFid;
    uint32_t hat_size;
    uint8_t  hat;

} fp_rsp_authenticate_t;

typedef struct fp_rsp_end_authenticate {
    int32_t rsp_id;
    int32_t update_flag;
//lzk add for miui auth staticstal requirements 2016.9.23
    uint32_t match_fid;
    uint32_t match_tplt_len;
    int32_t match_score;
    int32_t match_result;
} fp_rsp_end_authenticate_t;

typedef struct fp_rsp_one_par {
    int32_t rsp_id;
    uint32_t par;
} fp_rsp_one_par_t;

typedef struct fp_rsp_byte_array {
    int32_t rsp_id;
    uint32_t remain_size;
    uint32_t array_len;
    uint8_t array[];
} fp_rsp_byte_array_t;

typedef struct fp_rsp_check_finger_present {
    int32_t rsp_id;
    int16_t FingerPresentSum;
} fp_rsp_check_finger_present_t;


typedef struct fp_rsp_get_enrol_fids {
    int32_t rsp_id;
    int32_t Fids[FINGER_MAX_COUNT];
    int32_t FidCnt;
} fp_rsp_get_enrol_fids_t;

typedef struct fp_rsp_rel_coords {
    int32_t rsp_id;
    int32_t GetRelCoordsRes;
    int32_t DeltaX;
    int32_t DeltaY;
} fp_rsp_rel_coords_t;
//-0---------------------------------------------
typedef struct fp_rsp_get_image_quality {
    int32_t rsp_id;
    int32_t Area;
    int32_t Quality;
    int32_t Condition;
} fp_rsp_get_image_quality_t;

typedef struct fp_rsp_get_template_ids {
    int32_t rsp_id;
    int32_t cntTemplate;
    int32_t Ids[1];
} fp_rsp_get_template_ids_t;

typedef struct fp_rsp_get_sub_rect_cnt {
    int32_t rsp_id;
    int32_t Count;
} fp_rsp_get_sub_rect_cnt_t;

typedef struct fp_rsp_get_sub_rect {
    int32_t rsp_id;
    int32_t Rect[12];
} fp_rsp_get_sub_rect_t;

typedef struct fp_rsp_get_image_dimention {
    int32_t rsp_id;
    int32_t Width;
    int32_t Height;
} fp_rsp_get_image_dimention_t;


typedef struct fp_rsp_read_reg {
    int32_t rsp_id;
    int32_t reg;
} fp_rsp_read_reg_t;




typedef struct fp_rsp_get_version {
    int32_t rsp_id;
    char    taVersion[VERSION_BUFFER_LEN];
    char    driverVersion[VERSION_BUFFER_LEN];
    char    algoVersion[VERSION_BUFFER_LEN];
} fp_rsp_get_version_t;


typedef struct fp_rsp_tee_info {
    int32_t rsp_id;
    uint32_t sensor_revision;
    uint32_t sensor_hw_id;
    int32_t feature_tee_storage;
    int32_t feature_sec_pay;
    int32_t feature_hw_auth;
    char alg_name[ALG_NAME_LEN];
} fp_rsp_tee_info_t;

typedef struct fp_rsp {
    union {
        fp_rsp_no_payload_t             no_payload_rsp;
        fp_rsp_capture_img_t            capture_img_rsp;
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
#ifdef FP_TEE_QSEE4
        char dummy[64 * 1024];
#endif
    } r;
} fp_rsp_t;

/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)

#endif
