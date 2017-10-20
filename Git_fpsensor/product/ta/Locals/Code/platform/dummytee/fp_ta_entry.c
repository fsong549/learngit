#include "fp_internal_api.h"
#include "fp_ta_entry.h"
#include "hw_auth_token.h"
#include <hardware.h>

#define LOGTAG "fp_ta_entry.c "

extern int process_cmd_fingerprint(tciMessage_t *message);
extern int _process_cmd_fingerprint(void *pCmd, void *pRsp, int *pLen);

// SWd buffer where crypto operation happens
uint8_t swdData[MAX_DATA_LEN];

/**
 * TA entry.
 */
void tlMain( addr_t tciBuffer,  uint32_t tciBufferLen)
{
    tciReturnCode_t ret;
    LOGD(LOGTAG"TA Tlfingerprint: Starting.\n");

    LOGD(LOGTAG"TA Tlfingerprint: REE info.\n");

    // Check if the size of the given TCI is sufficient.
    if ((NULL == tciBuffer) || (sizeof(tciMessage_t) > tciBufferLen))
    {
        LOGE(LOGTAG"REE TA Tlfingerprint: Error, invalid TCI size.\n");
        LOGE(LOGTAG"TCI buffer: %x.\n", tciBuffer);
        LOGE(LOGTAG"TCI buffer length: %d.\n", tciBufferLen);
        LOGE(LOGTAG"sizeof(tciMessage_t): %d.\n", sizeof(tciMessage_t));
    }

    tciMessage_t *message = (tciMessage_t *) tciBuffer;

    // Copy commandId in SWd.
    tciCommandId_t commandId = message->msg_header.command.commandId;

    LOGD(LOGTAG"REE TA cmd is %s, cmd sn=%d, commandId=%x, fp_cmdID=0x%x, mallocCnt = %d ,pbmalloccnt = %d\n",
         get_cmd_str_from_cmd_id(*(int32_t *)(&message->data)), message->msg_header.cmdFp.sn,
         commandId, *(int32_t *)(&message->data), getMallocCnt(), getPbMallocCnt());
    // dbgBlob(LOGTAG" Before process TCI buffer:", tciBuffer, 32);

    // Process command message.
    switch (commandId)
    {
            //-----------------------------------------------
        case FP_OPERATION_ID:
            ret = process_cmd_fingerprint(message);
            break;
        default:
            // Unknown commandId.
            LOGE(LOGTAG"TA Tlfingerprint: Error, unknow commandId=%x.\n");
            ret = RET_ERR_UNKNOWN_CMD;
            break;
    } // end switch (commandId).

    // Set up response header -> mask response ID and set return code.
    message->msg_header.response.responseId = RSP_ID(commandId);
    message->msg_header.response.returnCode = ret;
    LOGD(LOGTAG"TA Tlfingerprint: returning %d.\n", ret);

}

/**
 * Process a fingerprint command message.
 * The command data will be checked for in.
 *
 * @return 0 if operation has been successfully completed.
 */

int process_cmd_fingerprint(tciMessage_t *message)
{
    // Check parameters.
    fp_cmd_t *pCmd = NULL;
    fp_rsp_t *pRsp = NULL;
    uint32_t len = message->msg_header.cmdFp.len;
    char *data = (char *)message->data;
    int iRet = 0;

    if (len > MAX_DATA_LEN)
    {
        LOGE(LOGTAG"TA Tlfingerprint: Error, invalid cipher data length (> %d).\n", MAX_DATA_LEN);
        return -FP_ERROR_NOSPACE;
    }

    // Copy plain data to SWd.
    memcpy(&swdData, data, len);

    pCmd = (fp_cmd_t *)swdData;
    pRsp = (fp_rsp_t *)data;

    iRet = _process_cmd_fingerprint((void *)pCmd, (void *)pRsp, &message->msg_header.rspFp.len);
    return iRet;
}

int32_t dummy_link()
{
    addr_t tciBuffer = 0;
    uint32_t tciBufferLen = 0;

    tlMain(tciBuffer, tciBufferLen);
    return 0;
}

