#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>

#include "fpsensor_gpio.h"

#define FPTAG "fpsensor_gpio.cpp"

/* define commands, must be same as driver */
#define FPSENSOR_IOC_MAGIC                      0xf0    //CHIP
#define FPSENSOR_IOC_INIT                       _IOWR(FPSENSOR_IOC_MAGIC,0,unsigned int)
#define FPSENSOR_IOC_EXIT                       _IOWR(FPSENSOR_IOC_MAGIC,1,unsigned int)
#define FPSENSOR_IOC_RESET                      _IOWR(FPSENSOR_IOC_MAGIC,2,unsigned int)
#define FPSENSOR_IOC_ENABLE_IRQ                 _IOWR(FPSENSOR_IOC_MAGIC,3,unsigned int)
#define FPSENSOR_IOC_DISABLE_IRQ                _IOWR(FPSENSOR_IOC_MAGIC,4,unsigned int)
#define FPSENSOR_IOC_GET_INT_VAL                _IOWR(FPSENSOR_IOC_MAGIC,5,unsigned int)
#define FPSENSOR_IOC_ENABLE_POWER               _IOWR(FPSENSOR_IOC_MAGIC,8,unsigned int)

const char *kDevFile = "/dev/fpsensor";
int file_descriptor_ = 0;

fpGpioHal::fpGpioHal()
{
    LOGD(FPTAG"constructor invoked");
    unsigned int value = 0;

    if (0 == file_descriptor_)
    {
        file_descriptor_ = open(kDevFile, O_RDWR);
        LOGD(FPTAG"%s-%s----fpsensor after open %s : %d\n", __DATE__, __TIME__, kDevFile, file_descriptor_);
    }

    if (file_descriptor_ < 0)
    {
        LOGE(FPTAG"open %s failed file_descriptor_:%d", kDevFile, file_descriptor_);
    }
}

fpGpioHal::~fpGpioHal()
{
    LOGD(FPTAG"destructor invoked");

    close(file_descriptor_);
    file_descriptor_ = 0;
}

int fpGpioHal::ChipGpioReset(void)
{
    unsigned int value = 0;
    int result   = 0;
    result = ::ioctl(file_descriptor_, FPSENSOR_IOC_RESET, &value);
    LOGD(FPTAG"ChipGpioReset result:%d", result);

    return result;
}

int fpGpioHal::IOCtrlInit(void)
{

    LOGD(FPTAG"IOCtrlInit invoked");
    int result   = 0;
    unsigned int value   = 0;

    result = ::ioctl(file_descriptor_, FPSENSOR_IOC_INIT, &value);

    if (result < 0)
    {
        LOGD(FPTAG"/dev/FPSENSOR_IOC_INIT error:%d\n", result);
    }
    else
    {
        LOGD(FPTAG"/dev/FPSENSOR_IOC_INIT ok \n");
    }
    result = ::ioctl(file_descriptor_, FPSENSOR_IOC_ENABLE_IRQ, &value);

    if (result < 0)
    {
        LOGD(FPTAG"/dev/FPSENSOR_IOC_ENABLE_IRQ error:%d\n", result);
    }
    else
    {
        LOGD(FPTAG"/dev/FPSENSOR_IOC_ENABLE_IRQ ok \n");
    }

    return result;
}

int fpGpioHal::IrqPoll(void)
{
    LOGD(FPTAG"IrqPoll");

    struct pollfd pfd;
    int status = 0;

    memset(&pfd, 0x00, sizeof(pfd));
    pfd.fd = file_descriptor_;
    pfd.events = POLLIN | POLLHUP | POLLERR;

    status = ::poll(&pfd, 1, 2000);//-1
    LOGI(FPTAG" IrqPoll status:%d", status);

    if (status < 0)
    {
        LOGE(FPTAG" IrqPoll error!!");
        status = -errno;
    }
    else if (pfd.revents & POLLERR)
    {
        LOGI(FPTAG"capture image be stoped!!");
        status = -EINTR;
    }
    else if (0 == status)
    {
        status = -EAGAIN;
    }
    else
    {
        LOGD(FPTAG"Got the gpio irq  OK!!!!!!!");
        status = 0;
    }

    return status;
}

int fpGpioHal::WaitForGpioIrq(void)
{
    // LOGD(FPTAG"WaitForGpioIrq  invoked");
    int iRet = 0;

    iRet = IrqPoll();

    if (-EAGAIN == iRet)
    {
        LOGD(FPTAG"IRQ test failed!!!!!!! -->  poll time out!!!!!!!!!");
    }

    return iRet;
}
