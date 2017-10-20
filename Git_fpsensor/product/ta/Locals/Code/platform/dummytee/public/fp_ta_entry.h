
#ifndef TL_FINGERPRINT_H_
#define TL_FINGERPRINT_H_

#include "tci.h"
#include "fp_tee_types.h"
// #include <hardware.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#define UNUSED_VAR(X)   (void)(X)

#define FP_FINGERPRINT_MODULE_API_VERSION_0 HARDWARE_MAKE_API_VERSION(0, 0)
#define FP_FINGERPRINT_DEVICE_API_VERSION_0 HARDWARE_DEVICE_API_VERSION(0, 0)

#define FP_FINGERPRINT_HARDWARE_MODULE_ID  "fpsensor_module"
#define FP_FINGERPRINT_SENSOR_DEVICE       "fpsensor_sensor"
#define FP_FINGERPRINT_SYSTEM_DEVICE       "fpsensor_system"
// typedef struct fp_fingerprint_moudule_t
// {
//     struct hw_module_t common; //inheritance

// } fp_fingerprint_moudule_t;
// typedef struct fpsensor_hw_device
// {
//     struct hw_device_t common;
//     void (*tlMain)(const addr_t tciBuffer, const uint32_t tciBufferLen);
// } fpsensor_hw_device;

// typedef struct fpsensor_ta_hal
// {
//     fpsensor_hw_device device;
// }fpsensor_ta_hal;


/**
 * Command ID's for communication Trusted Application Connector (TAC)
 * -> Trusted Application (TA).
 */

#define FP_OPERATION_ID 1
/**
 * Return codes
 */
#define MAP_BUFFER_SIZE  (512 * 1024)


/**
 * Termination codes
 */
#define EXIT_ERROR ((uint32_t)(-1))
typedef struct
{
    tciCommandHeader_t header; /**< Command header */
    uint32_t len; /**< Length of data to process */
    uint32_t sn;//mapdata reuse this variable to cmd sn
} cmdFp_t;

/**
 * Response structure TA -> TAC.
 */
typedef struct
{
    tciResponseHeader_t header; /**< Response header */
    int len; /**< Length of data processed */
    uint32_t mapdata;
} rspFp_t;

/**
 * TCI message data.
 */
typedef union cmd_rsp_header
{
    tciCommandHeader_t command; /**< Command header */
    tciResponseHeader_t response; /**< Response header */
    cmdFp_t cmdFp;
    rspFp_t rspFp;
} cmd_rsp_header_t;

#if FEATURE_TEE_STORAGE
#define MAX_TEE_SHM_SIZE (500 * 1024)
#else
#define MAX_TEE_SHM_SIZE (500 * 1024)  //different tee platform will be different!
#endif

/**
 * Maximum data length.
 */
#define MAX_DATA_LEN    (MAX_TEE_SHM_SIZE - sizeof(cmd_rsp_header_t))

#define MAX_LOAD_CHUNK ((MAX_DATA_LEN) - sizeof(fp_cmd_byte_array_t))
#define MAX_STORE_CHUNK ((MAX_DATA_LEN) - sizeof(fp_rsp_byte_array_t))


typedef struct
{
    cmd_rsp_header_t msg_header;
    uint8_t data[MAX_DATA_LEN];
} tciMessage_t;



// typedef union
// {
//     tciMessage_t msg;
//     uint8_t shm_data[MAX_TEE_SHM_SIZE];
// } fp_transfer_msg_t;

/**
 * TA UUID.
 */
#define TL_SAMPLE_FP_UUID { { 5, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }


// extern int driver_open();
// extern void driver_close();
#endif // TL_SAMPLE_FP_API_H_
