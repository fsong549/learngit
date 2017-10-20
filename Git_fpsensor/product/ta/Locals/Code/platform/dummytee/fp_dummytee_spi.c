
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include "fp_log.h"
#include <linux/types.h>
#include <linux/netlink.h>
#include <sys/ioctl.h>
#include <linux/wait.h>
#define FPTAG "fp_ree_spi.c "
/**********************IO Magic**********************/
#define FPSENSOR_IOC_MAGIC    0xf0    //CHIP
#define FPSENSOR_IOCTL_R_SENSOR_REG              _IOWR(FPSENSOR_IOC_MAGIC,14,unsigned char [17])
#define FPSENSOR_IOCTL_W_SENSOR_REG              _IOWR(FPSENSOR_IOC_MAGIC,15,unsigned char [17])
#define FPSENSOR_IOCTL_SEND_CMD                   _IOWR(FPSENSOR_IOC_MAGIC,16,unsigned char *)
#define FPSENSOR_IOCTL_SET_SPI_CLK               _IOWR(FPSENSOR_IOC_MAGIC,17,unsigned int*)
#define FPSENSOR_IOCTL_FETCH_IMAGE               _IOWR(FPSENSOR_IOC_MAGIC,18,unsigned char [8])

char *kDevFile = "/dev/fpsensor";
int _file_descriptor_ = 0;
int driver_open_ta()
{
    LOGD(FPTAG"[rickon]----open \n");
    if (_file_descriptor_)
    {
        LOGD("[rickon]----already open \n");
        return 0;
    }
    _file_descriptor_ = open(kDevFile,  /*O_RDONLY*/O_RDWR);
    LOGD(FPTAG"[rickon]----after open /dev/fpsensor: %d\n", _file_descriptor_);
    if (_file_descriptor_ < 0)
    {
        _file_descriptor_ = 0;
        return -1;
    }
    return 0;
}

void driver_close_ta()
{
    LOGD(FPTAG"[rickon]----close \n");
    if (_file_descriptor_)
    {
        close(_file_descriptor_);
        _file_descriptor_ = 0;
    }
}

int driver_poll_ta(unsigned int timeout)
{
//  LOGD("[rickon]----poll \n");
    struct pollfd pfd;
    pfd.fd = _file_descriptor_;
    pfd.events = POLLIN | POLLHUP;
    int status = poll(&pfd, 1, timeout);
    if (status < 0)
    {
        return -1;
    }
    return pfd.revents;
}


// ------------------------------------------------------------------------------------------------
int fpsensor_spi_init(int freq_low_khz, int freq_high_khz)
{
    int retval = 0;
    return retval;
}
int fpsensor_spi_clk_set(unsigned int clk)
{
    int retval = 0;
    retval = ioctl(_file_descriptor_, FPSENSOR_IOCTL_SET_SPI_CLK, &clk);
    return retval;
}
// ------------------------------------------------------------------------------------------------
int fpsensor_spi_writeread_fifo(char *tx, char *rx, int tx_len, int send_len)
{
    int retval = 0;
    unsigned char buffer[18];
    buffer[0] = (unsigned char)send_len;
    memcpy(buffer + 1, tx, tx_len);

    retval = ioctl(_file_descriptor_, FPSENSOR_IOCTL_R_SENSOR_REG, buffer);

    memcpy(rx, buffer, send_len);
    return retval;
}
// -----------------------------------------------------------------------------------------------
// unsigned char *image_temp_buffer;
int fpsensor_spi_writeread_dma(char *tx, char *rx, int send_len)
{
    int retval = 0;

    tx[1] = 2; // cmd len
    retval = ioctl(_file_descriptor_, FPSENSOR_IOCTL_FETCH_IMAGE, tx);
    if (retval < 0)
    {
        retval = -1;
    }

    retval = read(_file_descriptor_, rx + 2, send_len - 2);
    if (retval < 0)
    {
        retval = -1;
    }
    else
    {
        retval = 0;
    }
    return retval;
}
