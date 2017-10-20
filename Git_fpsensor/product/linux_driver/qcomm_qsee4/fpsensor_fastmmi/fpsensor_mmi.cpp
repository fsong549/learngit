/*
 * Copyright (c) 2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include "QSEEComAPI.h"
#include "fp_tee_types.h"
#include "fpsensor_gpio.h"

#define FPTAG               "fpsensor_mmi.cpp"
#define FPSENSOR_TA_PATH    "/etc/firmware"
#define FPSENSOR_TA_NAME    "fngap64"
#define FPSENSOR_NOPAYLOAD_CMD_LEN      (4)
#define FPSENSOR_TA_MAX_SHARE_BUFF_SIZE (512 * 1024)

typedef struct {
    int32_t cmdLen;
	uint32_t data1; 
	uint32_t data2;
    fp_cmd_t fpCmd;
}__attribute__ ((aligned (64))) fpsensor_qsee_msg_header;

typedef struct {
    int32_t rspCode;
    int32_t rspLen;
    fp_rsp_t fpRsp;
}__attribute__ ((aligned (64))) fpsensor_qsee_msg_response;

fpsensor_qsee_msg_header *qseeCmd = NULL;
fpsensor_qsee_msg_response *qseeRsp = NULL;
struct QSEECom_handle *qsee_sessionHandle = NULL;
class fpGpioHal *pGpioHal = NULL; 

static int32_t fpsensor_send_mmi_cmd(int cmd_id)
{
    int32_t status = 0;
    int32_t cmd_len = 0;
    int32_t rsp_len = 0;
    fp_cmd_no_payload_t cmd;

    if (!qsee_sessionHandle) {
        LOGE(FPTAG"CAN NOT send Qsee CMD, TA is not loaded!!!\n");
        return FAILED;
    }

    qseeCmd = (fpsensor_qsee_msg_header *)qsee_sessionHandle->ion_sbuffer;
    
    cmd_len = sizeof(fpsensor_qsee_msg_header);
    if (cmd_len & QSEECOM_ALIGN_MASK)
        cmd_len = QSEECOM_ALIGN(cmd_len);
    // construct qseeCmd
    cmd.cmd_id = cmd_id;
    memcpy(&qseeCmd->fpCmd, &cmd, sizeof(cmd));
    qseeCmd->cmdLen = FPSENSOR_NOPAYLOAD_CMD_LEN; // INIT and CHECKBOARD are both NoPayload cmd

    // construct qseeRsp
    qseeRsp = (fpsensor_qsee_msg_response *)(qsee_sessionHandle->ion_sbuffer + cmd_len);
    rsp_len = sizeof(fpsensor_qsee_msg_response);
    if (rsp_len & QSEECOM_ALIGN_MASK)
        rsp_len = QSEECOM_ALIGN(rsp_len);
    
    // send cmd
    //LOGD(FPTAG"%s CMD addr:0x%08X, RSP addr: 0x%08X\n", __func__, qseeCmd, qseeRsp);
    LOGD(FPTAG"%s send cmd id:%d, length:%d, %s-%s\n", __func__, cmd_id, cmd_len, __TIME__, __DATE__);
    status = QSEECom_send_cmd(qsee_sessionHandle, qseeCmd, cmd_len, qseeRsp, rsp_len);
    if (status) {
        LOGE(FPTAG"%s send_cmd failed %i", __func__, status);
        return FAILED;
    }

    LOGD(FPTAG"%s recv rsp_code:%d, rsp_len:%d", __func__, qseeRsp->rspCode, qseeRsp->rspLen);
    return SUCCESS;
}

static const char str_send_selftest_cmd_err[] = "Send qsee SELFTEST CMD fail";
static const char str_selftest_err[] = "fpsensor SELFTEST fail";
static const char str_selftest_pass[] = "fpsensor SELFTEST pass";
static const char str_send_checkboard_cmd_err[] = "Send qsee CHECKBOARD CMD fail";
static const char str_checkboard_err[] = "fpsensor CHECKBOARD fail";
static const char str_checkboard_pass[] = "fpsensor CHECKBOARD pass";

static void *fpsensor_run_test(void *mod)
{
    int32_t status = 0;
    mmi_module_t *module = NULL;

    if(mod == NULL) {
        LOGE(FPTAG"%s NULL for cb function ", __func__);
        return NULL;
    }

    module = (mmi_module_t *)mod;
    signal(SIGUSR1, signal_handler);
    
    /* reset chip first */
    pGpioHal->ChipGpioReset();
    /* send init == selftest */
    status = fpsensor_send_mmi_cmd(TCI_FP_CMD_INIT);
    if (status) {
        LOGE(FPTAG"%s send selftest cmd error: %d", __func__, status);
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_send_selftest_cmd_err,
                                            sizeof(str_send_selftest_cmd_err), PRINT);
        return NULL;
    }
    /* get selftest response */
    if (qseeRsp->rspCode != 0) {
        LOGE(FPTAG"%s selftest error: %d", __func__, qseeRsp->rspCode);
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_selftest_err, sizeof(str_selftest_err), PRINT);
        return NULL;
    } else {
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_selftest_pass, sizeof(str_selftest_pass), PRINT);
    }

    /* reset chip first */
    pGpioHal->ChipGpioReset();
    /* send checkboard cmd */
    status = fpsensor_send_mmi_cmd(TCI_FP_CMD_CHECKBOARD_TEST);
    if (status) {
        LOGE(FPTAG"%s send checkboard cmd error: %d", __func__, status);
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_send_checkboard_cmd_err,
                                            sizeof(str_send_checkboard_cmd_err), PRINT);
        fpsensor_send_mmi_cmd(TCI_FP_CMD_DEINIT);
        return NULL;
    }
    /* get checkboard response */
    if (qseeRsp->rspCode != 0) {
        LOGE(FPTAG"%s checkboard error: %d", __func__, qseeRsp->rspCode);
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_checkboard_err, sizeof(str_checkboard_err), PRINT);
        fpsensor_send_mmi_cmd(TCI_FP_CMD_DEINIT);
        return NULL;
    } else {
        module->cb_print("FPSENSOR", SUBCMD_MMI, str_checkboard_pass, sizeof(str_checkboard_pass), PRINT);
        fpsensor_send_mmi_cmd(TCI_FP_CMD_DEINIT);
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

static int32_t fpsensor_ta_open(void)
{
    int32_t status = 0;

	LOGD(FPTAG"%s open TA %s/%s\n", __func__, FPSENSOR_TA_PATH, FPSENSOR_TA_NAME);
    status = QSEECom_start_app(&qsee_sessionHandle,
                                FPSENSOR_TA_PATH,
                                FPSENSOR_TA_NAME,
                                FPSENSOR_TA_MAX_SHARE_BUFF_SIZE);
    if (status) {
		LOGE(FPTAG"%s start_app failed: %i", __func__, status);
		return FAILED;
	}

	LOGD(FPTAG"%s open qsee TA OK! %s-%s", __func__, __DATE__, __TIME__);
	return SUCCESS;
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
    
    ret = fpsensor_ta_open();
    if (ret) {
        LOGD(FPTAG"%s: TA open failed!!!\n", __func__);
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

static int32_t fpsensor_ta_close(void)
{
    int32_t status = 0;
    
    if (qsee_sessionHandle == NULL) {
        LOGE(FPTAG " %s: TA already closed!!!\n", __func__);
        return SUCCESS;
    }

    status = QSEECom_shutdown_app(&qsee_sessionHandle);
    if (status) {
        LOGE(FPTAG"%s shutdown_app failed: %d", __func__, status);
        return FAILED;
    }
    
    qsee_sessionHandle = NULL;
    return SUCCESS;
}

static int32_t fpsensor_module_stop(const mmi_module_t *module)
{
    int32_t status = SUCCESS;

    LOGD(FPTAG"%s start.", __func__);

    // TA opened in fpsensor_module_run(), so close it in fpsensor_module_stop()
    fpsensor_ta_close();

    if(module == NULL) {
        LOGE("%s NULL point received ", __func__);
        return FAILED;
    }
    pthread_kill(module->run_pid, SIGUSR1);

    return SUCCESS;
}

static int32_t fpsensor_module_init(const mmi_module_t * module, unordered_map < string, string > &params)
{
    LOGD(FPTAG"%s start ", __func__);

    if(module == NULL) {
        LOGE("%s NULL point received ", __func__);
        return FAILED;
    }

    pGpioHal = new fpGpioHal();
    pGpioHal->IOCtrlInit();    
    return SUCCESS;
}

static int32_t fpsensor_module_deinit(const mmi_module_t *module)
{
    LOGD(FPTAG"%s start.", __func__);

    if(module == NULL) {
        LOGE(FPTAG"%s NULL point received ", __func__);
        return FAILED;
    }

    delete pGpioHal;
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
