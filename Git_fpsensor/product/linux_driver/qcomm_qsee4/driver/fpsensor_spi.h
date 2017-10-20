#ifndef __FPSENSOR_SPI_H
#define __FPSENSOR_SPI_H

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/wakelock.h>

#define FPSENSOR_DEV_NAME           "fpsensor"
#define FPSENSOR_CLASS_NAME         "fpsensor"
#define FPSENSOR_DEV_MAJOR          255
#define N_SPI_MINORS                32    /* ... up to 256 */
#define FPSENSOR_NR_DEVS            1


#define  USE_PLATFORM_BUS     1
// #define  USE_SPI_BUS    1
/**************************feature control******************************/
#define FPSENSOR_IOCTL    1
#define FPSENSOR_SYSFS    0
#define FPSENSOR_INPUT    0
#define FPSENSOR_DUMMYTEE 0
#define SLEEP_WAKEUP_HIGH_LEV 0
#define FP_NOTIFY         1

extern u8 fpsensor_debug_level;
#define ERR_LOG     (0)
#define INFO_LOG    (1)
#define DEBUG_LOG   (2)
#define fpsensor_debug(level, fmt, args...) do { \
        if (fpsensor_debug_level >= level) {\
            printk( "[fpsensor] " fmt, ##args); \
        } \
    } while (0)
#define FUNC_ENTRY()  fpsensor_debug(DEBUG_LOG, "%s, %d, entry\n", __func__, __LINE__)
#define FUNC_EXIT()   fpsensor_debug(DEBUG_LOG, "%s, %d, exit\n", __func__, __LINE__)

/**********************IO Magic**********************/
#define FPSENSOR_IOC_MAGIC    0xf0    //CHIP

/* define commands */
#define FPSENSOR_IOC_INIT                       _IOWR(FPSENSOR_IOC_MAGIC,0,unsigned int)
#define FPSENSOR_IOC_EXIT                       _IOWR(FPSENSOR_IOC_MAGIC,1,unsigned int)
#define FPSENSOR_IOC_RESET                      _IOWR(FPSENSOR_IOC_MAGIC,2,unsigned int)
#define FPSENSOR_IOC_ENABLE_IRQ                 _IOWR(FPSENSOR_IOC_MAGIC,3,unsigned int)
#define FPSENSOR_IOC_DISABLE_IRQ                _IOWR(FPSENSOR_IOC_MAGIC,4,unsigned int)
#define FPSENSOR_IOC_GET_INT_VAL                _IOWR(FPSENSOR_IOC_MAGIC,5,unsigned int)
#define FPSENSOR_IOC_DISABLE_SPI_CLK            _IOWR(FPSENSOR_IOC_MAGIC,6,unsigned int)
#define FPSENSOR_IOC_ENABLE_SPI_CLK             _IOWR(FPSENSOR_IOC_MAGIC,7,unsigned int)
#define FPSENSOR_IOC_ENABLE_POWER               _IOWR(FPSENSOR_IOC_MAGIC,8,unsigned int)
#define FPSENSOR_IOC_DISABLE_POWER              _IOWR(FPSENSOR_IOC_MAGIC,9,unsigned int)
/* fp sensor has change to sleep mode while screen off */
#define FPSENSOR_IOC_ENTER_SLEEP_MODE           _IOWR(FPSENSOR_IOC_MAGIC,11,unsigned int)
#define FPSENSOR_IOC_REMOVE                     _IOWR(FPSENSOR_IOC_MAGIC,12,unsigned int)
#define FPSENSOR_IOC_CANCEL_WAIT                _IOWR(FPSENSOR_IOC_MAGIC,13,unsigned int)

#define FPSENSOR_IOCTL_R_SENSOR_REG                _IOWR(FPSENSOR_IOC_MAGIC,14,unsigned char [17])
#define FPSENSOR_IOCTL_W_SENSOR_REG                _IOWR(FPSENSOR_IOC_MAGIC,15,unsigned char [17])
#define FPSENSOR_IOCTL_SEND_CMD                   _IOWR(FPSENSOR_IOC_MAGIC,16,unsigned char *)
#define FPSENSOR_IOCTL_SET_SPI_CLK               _IOWR(FPSENSOR_IOC_MAGIC,17,unsigned int*)
#define FPSENSOR_IOCTL_FETCH_IMAGE               _IOWR(FPSENSOR_IOC_MAGIC,18,unsigned char [8])
#define FPSENSOR_IOC_GET_FP_STATUS              _IOWR(FPSENSOR_IOC_MAGIC,19,unsigned int*)
#define FPSENSOR_IOC_MAXNR    64  /* THIS MACRO IS NOT USED NOW... */
#define FPSENSOR_MAX_VER_BUF_LEN  64
#define FPSENSOR_MAX_CHIP_NAME_LEN 64
typedef struct {
    dev_t devno;
    struct class *class;
    struct cdev cdev;
#if defined(USE_SPI_BUS)
    struct spi_device *spi;
#elif defined(USE_PLATFORM_BUS)
    struct platform_device *spi;
#endif
    int users;
    u8 device_available;    /* changed during fingerprint chip sleep and wakeup phase */
    u8 irq_enabled;
    volatile unsigned int RcvIRQ;
    int irq;
    int irq_gpio;
    int reset_gpio;
    int power_gpio;
    struct wake_lock ttw_wl;
    wait_queue_head_t wq_irq_return;
    int cancel;

    unsigned char *huge_buffer;
    size_t                 huge_buffer_size;
    u16                    spi_freq_khz;
    unsigned char          fetch_image_cmd;
    unsigned char          fetch_image_cmd_len;
#if  FP_NOTIFY
    struct notifier_block notifier;
    u8 fb_status;
#endif
} fpsensor_data_t;

static void fpsensor_dev_cleanup(fpsensor_data_t *fpsensor);
static void fpsensor_hw_reset(int delay);
static void setRcvIRQ(int val);

#endif    /* __FPSENSOR_SPI_H */
