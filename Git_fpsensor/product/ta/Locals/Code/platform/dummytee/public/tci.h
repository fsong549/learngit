#ifndef TCI_H_
#define TCI_H_

/**
 * may:
 *  SO_MODE_READ   Opens for reading, If the so does not exist or can not be found, then #fp_tee_file_open call fails.
 *  SO_MODE_WRITE  Opens an empty secure object for writing, If the given secure object exists, its contents are destroyed.
 */
///< Opens for reading, If the so does not exist or can not be found,
///< then #fp_tee_file_open call fails.
#define  SO_MODE_READ       (0x1)
#define  SO_MODE_WRITE      (0x2)
#define TEE_OBJECT_ID_MAX_LEN           64
typedef unsigned int tlApiResult_t;
#define TLAPI_OK                            0x00000000    /**< Returns on successful execution of a function. */
typedef unsigned short u_int16_t;
// typedef unsigned int   pthread_t;
typedef unsigned char uint8_t;
typedef unsigned char u_int8_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef void        *void_ptr;  /**< a pointer to anything. */
typedef void_ptr    addr_t;     /**< an address, can be physical or virtual */
typedef uint32_t    bool_t;     /**< boolean data type. */
// typedef unsigned long long uint64_t;
// typedef long long int64_t;


typedef uint32_t tciCommandId_t;
typedef uint32_t tciResponseId_t;
typedef uint32_t tciReturnCode_t;

/**< Responses have bit 31 set */
#define RSP_ID_MASK (1U << 31)
#define RSP_ID(cmdId) (((uint32_t)(cmdId)) | RSP_ID_MASK)
#define IS_CMD(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == 0)
#define IS_RSP(cmdId) ((((uint32_t)(cmdId)) & RSP_ID_MASK) == RSP_ID_MASK)

/**
 * Return codes of Trustlet commands.
 */

#define RET_OK                                      0            /**< Set, if processing is error free */
#define RET_ERR_UNKNOWN_CMD                         1            /**< Unknown command */
#define RET_CUSTOM_START                            2

/**
 * TCI command header.
 */
typedef struct
{
    tciCommandId_t commandId; /**< Command ID */
} tciCommandHeader_t;

/**
 * TCI response header.
 */
typedef struct
{
    tciResponseId_t responseId; /**< Response ID (must be command ID | RSP_ID_MASK )*/
    tciReturnCode_t returnCode; /**< Return code of command */
} tciResponseHeader_t;

#endif // TCI_H_
