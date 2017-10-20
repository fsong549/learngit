#ifndef FPSENSOR_GPIO_H
#define FPSENSOR_GPIO_H

#include "mmi_module.h"

#define  LOGD(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

class fpGpioHal
{
public:
    fpGpioHal();
    ~fpGpioHal();
    int WaitForGpioIrq();
    int IOCtrlInit(void);
    int ChipGpioReset(void);
private:
    int IrqPoll(void);
};

#endif
