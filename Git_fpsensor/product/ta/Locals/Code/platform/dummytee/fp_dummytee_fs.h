#ifndef __FP_DUMMYTEE_FS_H__
#define __FP_DUMMYTEE_FS_H__

#define FP_TEE_HANDLE_NULL  (0)

/**
 * may:
 *  SO_MODE_READ   Opens for reading, If the so does not exist or can not be found, then #fp_tee_file_open call fails.
 *  SO_MODE_WRITE  Opens an empty secure object for writing, If the given secure object exists, its contents are destroyed.
 */
///< Opens for reading, If the so does not exist or can not be found,
///< then #fp_tee_file_open call fails.
#define  SO_MODE_READ       (0x1)
#define  SO_MODE_WRITE      (0x2)

typedef int32_t FP_ObjectHandle; //modify base on TEE OS.
extern int fp_tee_file_open(uint8_t *path, uint32_t mode, FP_ObjectHandle *so_handle);
extern void fp_tee_file_close(FP_ObjectHandle so_handle);
extern int fp_tee_file_delete(uint8_t *path);
extern int fp_tee_get_file_size(FP_ObjectHandle so_handle, uint32_t *file_size, uint8_t *path);
extern int fp_tee_file_read(FP_ObjectHandle so_handle, uint8_t *buffer, uint32_t size,
                            uint8_t *path);
extern int fp_tee_file_write(uint8_t *path, FP_ObjectHandle *so_handle, uint8_t *buffer,
                             uint32_t size);
extern int fp_tee_file_rename(FP_ObjectHandle so_handle, uint8_t *path_old, uint8_t *path_new);

#endif
