
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "fp_log.h"
#include <dlfcn.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include "fp_ta_entry.h"
#include "tci.h"
extern void *fp_malloc(size_t size);
extern void fp_free(void *buf);

#define FPTAG " fp_tee_ree_env.cpp "


tciMessage_t *tci;

uint8_t *pTAData = NULL;
uint32_t nTASize;
void *lib_handle_;
void (*tlMain)( addr_t ,  uint32_t );
int (*driver_open_ta)();
void (*driver_close_ta)();
// bool is_64bit_system(void)
// {
//     long int_bits = (((long)((long *)0 + 1)) << 3);
//     LOGI(FPTAG" is_64bit_system = %d", (int_bits == 64));
//     return (int_bits == 64);
// }
extern char *customer_get_dummytee_low_level_hal_name(void);
// -------------------------------------------------------------
int32_t tlcOpen(uint8_t *pTAData,  uint32_t nTASize)
{
    int32_t mcRet = 0;

    tci = (tciMessage_t *)fp_malloc(sizeof(tciMessage_t));
    if (tci == NULL)
    {
        LOGE(FPTAG"Allocation of TCI failed");
        return -12;
    }

    memset(tci, 0, sizeof(tciMessage_t));

    char *pFinaFpHalPath = customer_get_dummytee_low_level_hal_name(); //config dummytee ta path in fp_config_external.cpp
    if (pFinaFpHalPath == NULL)
    {
        LOGE(FPTAG" get dummytee low level hal name error\n");
        mcRet = -ENOENT;
        return mcRet;
    }

    lib_handle_ = dlopen(pFinaFpHalPath, RTLD_NOW);
    if (lib_handle_ == NULL)
    {
        LOGE(FPTAG" dlopen failed can't find hal so:%s\n", pFinaFpHalPath);
        mcRet = -ENOENT;
        return mcRet;
    }
    tlMain = (void ( *)(addr_t ,  uint32_t))dlsym(lib_handle_, "tlMain");
    if (tlMain == NULL)
    {
        LOGE(FPTAG" dlopen failed can't find dlsym tlMain:%s\n", pFinaFpHalPath);
        mcRet = -ENOENT;
        return mcRet;
    }
    driver_open_ta = (int ( *)())dlsym(lib_handle_, "driver_open_ta");
    if (tlMain == NULL)
    {
        LOGE(FPTAG" dlopen failed can't find dlsym driver_open_ta:%s\n", pFinaFpHalPath);
        mcRet = -ENOENT;
        return mcRet;
    }
    driver_close_ta = (void ( *)())dlsym(lib_handle_, "driver_close_ta");
    if (tlMain == NULL)
    {
        LOGE(FPTAG" dlopen failed can't find dlsym driver_close_ta:%s\n", pFinaFpHalPath);
        mcRet = -ENOENT;
        return mcRet;
    }
    driver_open_ta();
    return mcRet;
}


// -------------------------------------------------------------
void tlcClose(void)
{
    LOGD(FPTAG"Closing the session.");

    driver_close_ta();
    fp_free(tci);
    tci = NULL;
}

// -------------------------------------------------------------
uint32_t gSn = 0;

int32_t tlcFunc(unsigned char *pCmd, int len, taGetRspCunc pRspFunc, void *pProcessorInstance)
{
    if (NULL == tci)
    {
        LOGE(FPTAG"TCI has not been set up properly - exiting.");
        return -FP_ERROR_ALLOC;
    }

    do
    {
        // Prepare command message in TCI.
        gSn = (gSn++ > 100000) ? 0 : gSn;
        tci->msg_header.cmdFp.header.commandId = FP_OPERATION_ID;
        tci->msg_header.cmdFp.len = len;
        tci->msg_header.cmdFp.sn = gSn;

        // Copy data to TCI buffer.
        memcpy((char *) tci->data, pCmd, len);

        LOGD(FPTAG"cmd sn=%d,cmd data: %x, %x, %x, %x\n", tci->msg_header.cmdFp.sn, tci->data[0],
             tci->data[1],
             tci->data[2], tci->data[3] );
        tlMain(tci, sizeof(tciMessage_t));

        // Read result from TCI buffer.
        if (pRspFunc)
        {
            pRspFunc((char *) tci->data, tci->msg_header.rspFp.len, pProcessorInstance);
        }
    }
    while (0);

    return tci->msg_header.response.returnCode;
}

int32_t taOpen()
{
    LOGD(FPTAG"taOpen invoked");
    int32_t ret;
    LOGD(FPTAG"Copyright CHIPONE");
    ret = tlcOpen(pTAData, nTASize);
    return 0;
}

void taClose()
{
    LOGD(FPTAG"taClose invoked");
    tlcClose();
    if (pTAData)
    {
        fp_free(pTAData);
        pTAData = NULL;
    }
}

