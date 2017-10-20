#include "fp_internal_api.h"
#include "fp_log.h"
#include "fp_tee_types.h"
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#define LOGTAG " fp_internal_api.c "

static int iMallocCnt = 0;
static int iPbMallocCnt = 0;

void *fp_malloc(uint32_t size)
{
    if (size > 0)
    {
        iMallocCnt++;
    }

    return malloc(size);
}

void fp_free(void *buffer)
{
    if (buffer != NULL)
    {
        iMallocCnt--;
        free(buffer);
    }
}

void printMallocCnt(void)
{
    LOGD(LOGTAG"MallocCnt: %d ", iMallocCnt);
}

int getMallocCnt(void)
{
    return iMallocCnt;
}

int getPbMallocCnt(void)
{
    return iPbMallocCnt;
}

void fp_get_timestamp (uint64_t *pTimestamp)
{
    struct timeval ts;
    gettimeofday(&ts, NULL);
    *pTimestamp = ts.tv_usec + ts.tv_sec * 1000 * 1000;
}
uint64_t fp_get_uptime(void)
{
    uint64_t timestamp;
    fp_get_timestamp(&timestamp);

    return timestamp / 1000; //convert us to ms.
}

//1 lzk TBD the rand is not available in app_platform  api_21
static unsigned int g_next_random = 1;

struct timeval ta_timer_start(const char *pInfo)
{
    struct timeval start;
    if (pInfo)
    {
        LOGI(" timer start %s :", pInfo);
    }

    gettimeofday(&start, NULL);
    return start;
}

static void saRand(void)
{
    struct timeval cur_time = ta_timer_start(NULL);
    g_next_random = cur_time.tv_sec * 1000000 + cur_time.tv_usec;
}

static int32_t aRand()
{
    unsigned int next = g_next_random;
    int result;

    next *= 1103515245;
    next += 12345;
    result = (unsigned int) (next / 65536) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    g_next_random = next;

    return result;
}

int32_t fp_secure_random(uint8_t *buf, int32_t size)
{
    int32_t random_value = 0;
    saRand();
    for (int i = 0; i < size; i++)
    {
        random_value = aRand();
        buf[i] = random_value % 0xff;
    }
    return 0;
}

void fp_delay_ms (uint32_t duration_ms)
{
    usleep(duration_ms * 1000);
}

int fp_hmac_sha256(const uint8_t *data, uint32_t size_data,
                   const uint8_t *key, uint32_t size_key,
                   uint8_t *hmac)
{
    int status = 0;
    LOGD("[%s] enter \n", __func__ );

    return status;
}

/******************************************************************************/
uint32_t fp_get_wrapped_size(uint32_t data_size)
{
    return data_size;
}

/******************************************************************************/
int32_t fp_wrap_crypto(uint8_t *data,
                       uint32_t data_size,
                       uint8_t *enc_data,
                       uint32_t *enc_data_size)
{

    LOGD("%s: begin", __func__);
    memcpy(enc_data, data, data_size);//dummy
    LOGD("%s: end", __func__);

    return 0;
}

/******************************************************************************/
int32_t fp_unwrap_crypto(uint8_t *enc_data,
                         uint32_t enc_data_size,
                         uint8_t  **data,
                         uint32_t *data_size)
{
    LOGD("%s: begin\n", __func__);

    *data = enc_data;//dummy
    LOGD("%s: end\n", __func__);
    return 0;
}

