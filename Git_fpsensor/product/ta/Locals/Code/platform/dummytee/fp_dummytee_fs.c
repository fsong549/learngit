#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fp_log.h"
#include "fp_dummytee_fs.h"

int fp_tee_file_open(uint8_t *path, uint32_t mode, FP_ObjectHandle *so_handle)
{
    int fd = -1;

    if (path == NULL)
    {
        LOGE("%s path is NULL, wrong!!!", __func__);
        return -EINVAL;
    }

    fd = open((const char *)path, O_RDONLY);
    if (fd < 0)
    {
        LOGE("%s: failed open: %s", __func__, path);
        return -EFAULT;
    }
    LOGD("open file OK!,fd:%d", fd);
    // open success, file exist, close it now
    close(fd);
    return 0;
}

void fp_tee_file_close(FP_ObjectHandle so_handle)
{
    /*
     * we do open/close in every file read/write routine,
     * so do nothing in this handler
     */
    return ;
}

int fp_tee_file_delete(uint8_t *path)
{
    int ret = 0;

    ret = remove((const char *)path);
    LOGE("%s: %s with ret %d", __func__, path, ret);
    return ret;
}

int fp_tee_file_read(FP_ObjectHandle so_handle, uint8_t *buffer, uint32_t size, uint8_t *path)
{
    int ret = -1;
    int fd = 0;

    if (!path || !buffer)
    {
        LOGE("%s parameters is NULL, wrong!!!", __func__);
        return -EINVAL;
    }

    fd = open((const char *)path, O_RDWR);
    if (fd < 0)
    {
        LOGE("%s: failed open: %s", __func__, path);
        return -EFAULT;
    }

    ret = read(fd, buffer, size);
    // read ok
    if (ret == size)
    {
        LOGD("%s:read %d bytes ok!", __func__, ret);
        ret = 0;
        goto out;
    }
    // read error
    if (ret < 0)
    {
        LOGE("%s read failed %d, check!!!", __func__, ret);
        ret = -EIO;
        goto out;
    }
    // read bytes error
    if (ret < size)
    {
        LOGE("%s read byte %d is not correct, check!!!", __func__, ret);
        ret = -EIO;
        goto out;
    }
out:
    close(fd);
    return ret;
}

int fp_tee_file_write(uint8_t *path, FP_ObjectHandle *so_handle, uint8_t *buffer, uint32_t size)
{
    int ret = -1;
    int fd = 0;

    if (!path || !buffer)
    {
        LOGE("%s parameters is NULL, wrong!!!", __func__);
        return -EINVAL;
    }

    fd = open((const char *)path, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        LOGE("%s: failed open: %s, ret:%d", __func__, path, fd);
        return -EFAULT;
    }

    ret = write(fd, buffer, size);
    // write ok
    if (ret == size)
    {
        LOGD("%s write %d bytes data OK!", __func__, size);
        ret = 0;
        goto out;
    }
    // write error
    if (ret < 0)
    {
        LOGD("%s write failed %d, check!!!", __func__, ret);
        ret = -EIO;
        goto out;
    }
    // write bytes error
    if (ret < size)
    {
        LOGD("%s write byte %d is not correct, check!!!", __func__, ret);
        ret = -EIO;
        goto out;
    }
out:
    close(fd);
    return ret;
}

int fp_tee_get_file_size(FP_ObjectHandle so_handle, uint32_t *size, uint8_t *path)
{
    int ret = 0;
    int fd = -1;

    fd = open((const char *)path, O_RDONLY);
    if (-1 == fd)
    {
        LOGE("%s: failed to open the file : %s", __func__, path);
        return -EFAULT;
    }
    ret = lseek(fd, 0, SEEK_END);
    if (ret == -1)
    {
        LOGE("%s:failed to get the file %s size", __func__, path);
        close(fd);
        return -EIO;
    }
    *size = ret;
    ret = close(fd);
    if (ret == -1)
    {
        LOGE("%s:failed to close the file %s", __func__, path);
        return -EFAULT;
    }
    LOGD("%s: file:%s size:%d", __func__ , path, *size);
    return 0;
}

int fp_tee_file_rename(FP_ObjectHandle so_handle, uint8_t *path_old, uint8_t *path_new)
{
    int status = 0;

    status = rename((char *)path_old, (char *)path_new);
    LOGD("%s: %s ---> %s, status:%d", __func__, path_old, path_new, status);

    return status;
}
