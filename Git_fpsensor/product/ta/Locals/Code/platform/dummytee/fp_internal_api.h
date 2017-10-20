#ifndef FP_INTERNAL_API_H
#define FP_INTERNAL_API_H

#include "fp_log.h"
#include <stdint.h>
#include <stdbool.h>
#include "fp_dummytee_fs.h"

extern void *fp_malloc(uint32_t size);
extern void fp_free(void *buffer);

extern void fp_delay_ms (uint32_t duration_ms);
extern void fp_get_timestamp (uint64_t *pTimestamp);
extern uint64_t tatimerStart(const char *pInfo);
extern int tatimerEnd(uint64_t startTime, const char *pInfo);
extern uint64_t fp_get_uptime(void);

extern void printMallocCnt(void);
extern int getMallocCnt(void);
extern int getPbMallocCnt(void);
extern int32_t dummy_link(void);

extern int32_t fp_secure_random(uint8_t *data, int32_t length);
extern int fp_hmac_sha256(const uint8_t *data, uint32_t size_data,
                          const uint8_t *key, uint32_t size_key, uint8_t *hmac);
#endif

