#include <linux/device.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/completion.h>
#include <linux/gpio.h>

#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/notifier.h>
#endif

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#endif

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#ifdef CONFIG_MTK_CLKMGR
#include "mach/mt_clkmgr.h"
#else
#include <linux/clk.h>
#endif

#include <net/sock.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/wakelock.h>

/* MTK header */
#include "mt_spi.h"
#include "mt_spi_hal.h"
#include "mt_gpio.h"
#include "mach/gpio_const.h"

#include "fpsensor_spi.h"
#include "fpsensor_platform.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

#if FP_NOTIFY
#include <linux/fb.h>
#include <linux/notifier.h>
#endif
/*device name*/
#define FPSENSOR_DEV_NAME       "fpsensor"
#define FPSENSOR_CLASS_NAME     "fpsensor"
#define FPSENSOR_MAJOR          0
#define N_SPI_MINORS            32    /* ... up to 256 */

#define FPSENSOR_SPI_VERSION    "fpsensor_spi_v0.1"
#define FPSENSOR_INPUT_NAME     "fpsensor_keys"


/***********************input *************************/
#ifndef FPSENSOR_INPUT_HOME_KEY
/* on MTK EVB board, home key has been redefine to KEY_HOMEPAGE! */
/* double check the define on customer board!!! */
#define FPSENSOR_INPUT_HOME_KEY     KEY_HOMEPAGE /* KEY_HOME */
#define FPSENSOR_INPUT_MENU_KEY     KEY_MENU
#define FPSENSOR_INPUT_BACK_KEY     KEY_BACK
#define FPSENSOR_INPUT_FF_KEY       KEY_POWER
#define FPSENSOR_INPUT_CAMERA_KEY   KEY_CAMERA
#define FPSENSOR_INPUT_OTHER_KEY    KEY_VOLUMEDOWN  /* temporary key value for capture use */
#endif

#define FPSENSOR_NAV_UP_KEY     19//KEY_UP
#define FPSENSOR_NAV_DOWN_KEY   20//KEY_DOWN
#define FPSENSOR_NAV_LEFT_KEY   21//KEY_LEFT
#define FPSENSOR_NAV_RIGHT_KEY  22//KEY_RIGHT
#define FPSENSOR_NAV_TAP_KEY    23
/***********************GPIO setting port layer*************************/
/* customer hardware port layer, please change according to customer's hardware */
#define GPIO_PIN_IRQ   86

/*************************************************************/
static struct wake_lock fpsensor_timeout_wakelock;
/* debug log setting */
u8 fpsensor_debug_level = DEBUG_LOG;

fpsensor_data_t *g_fpsensor = NULL;


#define ROUND_UP(x, align)        ((x+(align-1))&~(align-1))


#define FPSENSOR_SPI_BUS_DYNAMIC 1
#if FPSENSOR_SPI_BUS_DYNAMIC
static struct spi_board_info spi_board_devs[] __initdata = {
    [0] = {
        .modalias = FPSENSOR_DEV_NAME,
        .bus_num = 0,
        .chip_select = 0,
        .mode = SPI_MODE_0,
        .controller_data = &fpsensor_spi_conf_mt65xx, //&spi_conf
    },
};
#endif



/* -------------------------------------------------------------------- */
/* fingerprint chip hardware configuration                              */
/* -------------------------------------------------------------------- */
static DEFINE_MUTEX(spidev_set_gpio_mutex);
static void spidev_gpio_as_int(fpsensor_data_t *fpsensor)
{
    FUNC_ENTRY();
    mutex_lock(&spidev_set_gpio_mutex);
    pinctrl_select_state(fpsensor->pinctrl1, fpsensor->eint_as_int);
    mutex_unlock(&spidev_set_gpio_mutex);
    FUNC_EXIT();
}
void fpsensor_gpio_output_dts(int gpio, int level)
{
    FUNC_ENTRY();
    mutex_lock(&spidev_set_gpio_mutex);
    if (gpio == FPSENSOR_RST_PIN) {
        if (level) {
            pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_rst_high);
        } else {
            pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_rst_low);
        }
    } else if (gpio == FPSENSOR_SPI_CS_PIN) {
        if (level) {
            pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_cs_high);
        } else {
            pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_cs_low);
        }
    }

    mutex_unlock(&spidev_set_gpio_mutex);
    FUNC_EXIT();
}


int fpsensor_gpio_wirte(int gpio, int value)
{
    fpsensor_gpio_output_dts(gpio, value);
    return 0;
}
int fpsensor_gpio_read(int gpio)
{
    return gpio_get_value(gpio);
}

int fpsensor_spidev_dts_init(fpsensor_data_t *fpsensor)
{
    struct device_node *node;
    int ret = 0;
    FUNC_ENTRY();
    node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
    if (node) {
        fpsensor->fp_rst_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_finger_rst_low");
        if (IS_ERR(fpsensor->fp_rst_low)) {
            ret = PTR_ERR(fpsensor->fp_rst_low);
            fpsensor_debug(ERR_LOG, "fpensor Cannot find fp pinctrl fpsensor_finger_rst_low!\n");
            return ret;
        }
        fpsensor->fp_rst_high = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_finger_rst_high");
        if (IS_ERR(fpsensor->fp_rst_high)) {
            ret = PTR_ERR(fpsensor->fp_rst_high);
            fpsensor_debug(ERR_LOG, "fpsensor Cannot find fp pinctrl fpsensor_finger_rst_high!\n");
            return ret;
        }

        fpsensor->eint_as_int = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_eint");
        if (IS_ERR(fpsensor->eint_as_int)) {
            ret = PTR_ERR(fpsensor->eint_as_int);
            fpsensor_debug(ERR_LOG, "fpsensor Cannot find fp pinctrl fpsensor_eint!\n");
            return ret;
        }
        fpsensor->eint_in_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_eint_in_low");
        if (IS_ERR(fpsensor->eint_as_int)) {
            ret = PTR_ERR(fpsensor->eint_as_int);
            fpsensor_debug(ERR_LOG, "fpsensor Cannot find fp pinctrl fpsensor_eint_in_low!\n");
            return ret;
        }
        // fpsensor->eint_in_high = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_eint_in_high");
        // if (IS_ERR(fpsensor->eint_in_high))
        // {
        //     ret = PTR_ERR(fpsensor->eint_in_high);
        //     fpsensor_debug(ERR_LOG, " Cannot find fp pinctrl fpsensor_eint_in_high!\n");
        //     return ret;
        // }

        fpsensor->eint_in_float = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_eint_in_float");
        if (IS_ERR(fpsensor->eint_in_float)) {
            ret = PTR_ERR(fpsensor->eint_in_float);
            fpsensor_debug(ERR_LOG, " Cannot find fp pinctrl eint_in_float!\n");
            return ret;
        }

//------------------------------ree
#if FPSENSOR_DUMMYTEE
        fpsensor->fp_spi_miso = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_mi_low");
        if (IS_ERR(fpsensor->fp_spi_miso)) {
            ret = PTR_ERR(fpsensor->fp_spi_miso);
            fpsensor_debug(ERR_LOG, " Cannot find fp pinctrl fpsensor_spi_mi_low!\n");
            return ret;
        }
        fpsensor->fp_spi_mosi = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_mo_low");
        if (IS_ERR(fpsensor->fp_spi_mosi)) {
            ret = PTR_ERR(fpsensor->fp_spi_mosi);
            fpsensor_debug(ERR_LOG, " Cannot find fp pinctrl fpsensor_spi_mo_low!\n");
            return ret;
        }
        fpsensor->fp_cs_high = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_finger_cs_high");
        if (IS_ERR(fpsensor->fp_cs_high)) {
            ret = PTR_ERR(fpsensor->fp_cs_high);
            fpsensor_debug(ERR_LOG, " Cannot find fp pinctrl fpsensor_finger_cs_high!\n");
            return ret;
        }
        fpsensor->fp_cs_low = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_finger_cs_low");
        if (IS_ERR(fpsensor->fp_cs_low)) {
            ret = PTR_ERR(fpsensor->fp_cs_low);
            fpsensor_debug(ERR_LOG, " Cannot find fp pinctrl fpsensor_finger_cs_low!\n");
            return ret;
        }
        fpsensor->fp_spi_clk = pinctrl_lookup_state(fpsensor->pinctrl1, "fpsensor_spi_mclk_low");
        if (IS_ERR(fpsensor->fp_spi_clk)) {
            ret = PTR_ERR(fpsensor->fp_spi_clk);
            fpsensor_debug(ERR_LOG, " Cannot find fp pinctrl fpsensor_spi_mclk_low!\n");
            return ret;
        }
        pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_spi_miso);
        pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_spi_mosi);
        pinctrl_select_state(g_fpsensor->pinctrl1, g_fpsensor->fp_spi_clk);
#endif
    } else {
        fpsensor_debug(ERR_LOG, "compatible_node Cannot find node!\n");
    }
    FUNC_EXIT();
    return 0;
}
/* delay us after reset */
static void fpsensor_hw_reset(int delay)
{
    FUNC_ENTRY();

    fpsensor_gpio_wirte(FPSENSOR_RST_PIN,    1);
    udelay(100);
    fpsensor_gpio_wirte(FPSENSOR_RST_PIN,  0);
    udelay(1000);
    fpsensor_gpio_wirte(FPSENSOR_RST_PIN,  1);
    if (delay) {
        /* delay is configurable */
        udelay(delay);
    }

    FUNC_EXIT();
    return;
}

static void fpsensor_spi_clk_enable(u8 bonoff)
{
#if FPSENSOR_DUMMYTEE

#else
    static int count = 0;
#ifdef CONFIG_MTK_CLKMGR
    if (bonoff && (count == 0)) {
        fpsensor_debug(DEBUG_LOG, "%s, start to enable spi clk && count = %d.\n", __func__, count);
        enable_clock(MT_CG_PERI_SPI0, "spi");
        count = 1;
    } else if ((count > 0) && (bonoff == 0)) {
        fpsensor_debug(DEBUG_LOG, "%s, start to disable spi clk&& count = %d.\n", __func__, count);
        disable_clock(MT_CG_PERI_SPI0, "spi");
        count = 0;
    }
#else
    /* changed after MT6797 platform */
    struct mt_spi_t *ms = NULL;

    ms = spi_master_get_devdata(g_fpsensor->spi->master);

    if (bonoff && (count == 0)) {
        mt_spi_enable_clk(ms);    // FOR MT6797
//      clk_enable(ms->clk_main); // FOR MT6755/MT6750
        count = 1;
    } else if ((count > 0) && (bonoff == 0)) {
        mt_spi_disable_clk(ms);    // FOR MT6797
//      clk_disable(ms->clk_main); // FOR MT6755/MT6750
        count = 0;
    }
#endif
#endif
}
// static void fpsensor_hw_power_enable(u8 onoff)
// {
//     static int enable = 1;
//     if (onoff && enable)
//     {
//         pinctrl_select_state(g_fpsensor->pinctrl_gpios, g_fpsensor->pins_power_on);
//         enable = 0;
//     }
//     else if (!onoff && !enable)
//     {
//         pinctrl_select_state(g_fpsensor->pinctrl_gpios, g_fpsensor->pins_power_off);
//         enable = 1;
//     }
// }

static int fpsensor_irq_gpio_cfg(void)
{
    int error = 0;
    struct device_node *node;
    fpsensor_data_t *fpsensor;
    u32 ints[2] = {0, 0};
    FUNC_ENTRY();

    fpsensor = g_fpsensor;

    spidev_gpio_as_int(fpsensor);

    node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
    if ( node) {
        of_property_read_u32_array( node, "debounce", ints, ARRAY_SIZE(ints));
        gpio_request(ints[0], "fpsensor-irq");
        //gpio_set_debounce(ints[0], ints[1]);
        fpsensor_debug(INFO_LOG, "[fpsensor]ints[0] = %d,is irq_gpio , ints[1] = %d!!\n", ints[0], ints[1]);
        fpsensor->irq_gpio = ints[0];
        fpsensor->irq = irq_of_parse_and_map(node, 0);  // get irq number
        if (!fpsensor->irq) {
            fpsensor_debug(ERR_LOG, "fpsensor irq_of_parse_and_map fail!!\n");
            return -EINVAL;
        }
        fpsensor_debug(INFO_LOG, " [fpsensor]fpsensor->irq= %d,fpsensor>irq_gpio = %d\n", fpsensor->irq,
                       fpsensor->irq_gpio);
    } else {
        fpsensor_debug(ERR_LOG, "fpsensor null irq node!!\n");
        return -EINVAL;
    }
    FUNC_EXIT();
    return error;

}
static void fpsensor_enable_irq(fpsensor_data_t *fpsensor_dev)
{
    FUNC_ENTRY();
    setRcvIRQ(0);
    if (0 == fpsensor_dev->device_available) {
        fpsensor_debug(ERR_LOG, "%s, devices not available\n", __func__);
    } else {
        if (1 == fpsensor_dev->irq_count) {
            fpsensor_debug(ERR_LOG, "%s, irq already enabled\n", __func__);
        } else {
            enable_irq(fpsensor_dev->irq);
            fpsensor_dev->irq_count = 1;
            fpsensor_debug(INFO_LOG, "%s enable interrupt!\n", __func__);
        }
    }
    FUNC_EXIT();
    return;
}

static void fpsensor_disable_irq(fpsensor_data_t *fpsensor_dev)
{
    FUNC_ENTRY();

    if (0 == fpsensor_dev->device_available) {
        fpsensor_debug(ERR_LOG, "%s, devices not available\n", __func__);
    } else {
        if (0 == fpsensor_dev->irq_count) {
            fpsensor_debug(ERR_LOG, "%s, irq already disabled\n", __func__);
        } else {
            disable_irq(fpsensor_dev->irq);
            fpsensor_dev->irq_count = 0;
            fpsensor_debug(DEBUG_LOG, "%s disable interrupt!\n", __func__);
        }
    }
    setRcvIRQ(0);
    FUNC_EXIT();
    return;
}

/* -------------------------------------------------------------------- */
/* file operation function                                              */
/* -------------------------------------------------------------------- */

static ssize_t fpsensor_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
#if FPSENSOR_DUMMYTEE
    int error;
    FUNC_ENTRY();
    error =  fpsensor_spi_dma_fetch_image(g_fpsensor, count);
    if (error) {
        return -EFAULT;
    }
    error = copy_to_user(buf, &g_fpsensor->huge_buffer[0], count);
    if (error) {
        return -EFAULT;
    }
    FUNC_EXIT();
    return count;
#else
    fpsensor_debug(ERR_LOG, "Not support read opertion in TEE version\n");
#endif
    return -EFAULT;
}

static ssize_t fpsensor_write(struct file *filp, const char __user *buf,
                              size_t count, loff_t *f_pos)
{
    fpsensor_debug(ERR_LOG, "Not support write opertion in TEE version\n");
    return -EFAULT;
}

static irqreturn_t fpsensor_irq(int irq, void *handle)
{

    fpsensor_data_t *fpsensor_dev = (fpsensor_data_t *)handle;
#if SLEEP_WAKEUP_HIGH_LEV
    int irqf = 0;
    irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_NO_SUSPEND;
    if (fpsensor_dev->suspend_flag == 1) {
        irq_set_irq_type(fpsensor_dev->irq, irqf);
        fpsensor_dev->suspend_flag = 0;
    }
#endif

    wake_lock_timeout(&fpsensor_timeout_wakelock, msecs_to_jiffies(1000));
#if FPSENSOR_IOCTL
    setRcvIRQ(1);
#endif
    wake_up_interruptible(&fpsensor_dev->wq_irq_return);
    fpsensor_dev->sig_count++;

    return IRQ_HANDLED;
}

void setRcvIRQ(int val)
{
    fpsensor_data_t *fpsensor_dev = g_fpsensor;
    // fpsensor_debug(INFO_LOG, "[rickon]: %s befor val :  %d ; set val : %d   \n", __func__, fpsensor_dev-> RcvIRQ, val);
    fpsensor_dev-> RcvIRQ = val;
}



static long fpsensor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    fpsensor_data_t *fpsensor_dev = NULL;
    struct fpsensor_key fpsensor_key;
#if FPSENSOR_INPUT
    uint32_t key_event;
#endif
#if FPSENSOR_DUMMYTEE
    u8 reg_data[17];
    u8 reg_rx[16];
#endif
    int retval = 0;
    unsigned int val = 0;

    FUNC_ENTRY();
    fpsensor_debug(INFO_LOG, "[rickon]: fpsensor ioctl cmd : 0x%x \n", cmd );
    fpsensor_dev = (fpsensor_data_t *)filp->private_data;
    //clear cancel flag
    fpsensor_dev->cancel = 0 ;
    switch (cmd) {
    case FPSENSOR_IOC_INIT:
        fpsensor_debug(INFO_LOG, "%s: fpsensor init started======\n", __func__);
        fpsensor_irq_gpio_cfg();
        retval = request_threaded_irq(fpsensor_dev->irq, fpsensor_irq, NULL,
                                      IRQF_TRIGGER_RISING | IRQF_ONESHOT, dev_name(&(fpsensor_dev->spi->dev)), fpsensor_dev);
        if (retval == 0) {
            fpsensor_debug(ERR_LOG, " irq thread reqquest success!\n");
        } else {
            fpsensor_debug(ERR_LOG, " irq thread request failed , retval =%d \n", retval);
        }
        fpsensor_dev->device_available = 1;
        fpsensor_dev->irq_count = 1;
        fpsensor_disable_irq(fpsensor_dev);

        fpsensor_dev->sig_count = 0;

        wake_lock_init(&fpsensor_timeout_wakelock, WAKE_LOCK_SUSPEND, "fpsensor timeout wakelock");
        fpsensor_debug(INFO_LOG, "%s: fpsensor init finished======\n", __func__);
        break;

    case FPSENSOR_IOC_EXIT:
        fpsensor_disable_irq(fpsensor_dev);
        if (fpsensor_dev->irq) {
            free_irq(fpsensor_dev->irq, fpsensor_dev);
            fpsensor_dev->irq_count = 0;
        }
        fpsensor_dev->device_available = 0;
        fpsensor_debug(INFO_LOG, "%s: fpsensor exit finished======\n", __func__);
        break;

    case FPSENSOR_IOC_RESET:
        fpsensor_debug(INFO_LOG, "%s: chip reset command\n", __func__);
        fpsensor_hw_reset(1250);
        break;

    case FPSENSOR_IOC_ENABLE_IRQ:
        fpsensor_debug(INFO_LOG, "%s: chip ENable IRQ command\n", __func__);
        fpsensor_enable_irq(fpsensor_dev);
        break;

    case FPSENSOR_IOC_DISABLE_IRQ:
        fpsensor_debug(INFO_LOG, "%s: chip disable IRQ command\n", __func__);
        fpsensor_disable_irq(fpsensor_dev);
        break;
    case FPSENSOR_IOC_GET_INT_VAL:
        val = __gpio_get_value(GPIO_PIN_IRQ);
        if (copy_to_user((void __user *)arg, (void *)&val, sizeof(unsigned int))) {
            fpsensor_debug(ERR_LOG, "Failed to copy data to user\n");
            retval = -EFAULT;
            break;
        }
        retval = 0;
        break;
    case FPSENSOR_IOC_ENABLE_SPI_CLK:
        fpsensor_debug(INFO_LOG, "%s: ENABLE_SPI_CLK ======\n", __func__);
        fpsensor_spi_clk_enable(1);
        break;
    case FPSENSOR_IOC_DISABLE_SPI_CLK:
        fpsensor_debug(INFO_LOG, "%s: DISABLE_SPI_CLK ======\n", __func__);
        fpsensor_spi_clk_enable(0);
        break;

    case FPSENSOR_IOC_ENABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_ENABLE_POWER ======\n", __func__);
        // fpsensor_hw_power_enable(1);
        break;

    case FPSENSOR_IOC_DISABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_DISABLE_POWER ======\n", __func__);
        // fpsensor_hw_power_enable(0);
        break;


    case FPSENSOR_IOC_INPUT_KEY_EVENT:
        if (copy_from_user(&fpsensor_key, (struct fpsensor_key *)arg, sizeof(struct fpsensor_key))) {
            fpsensor_debug(ERR_LOG, "Failed to copy input key event from user to kernel\n");
            retval = -EFAULT;
            break;
        }
#if FPSENSOR_INPUT
        if (FPSENSOR_KEY_HOME == fpsensor_key.key) {
            key_event = FPSENSOR_INPUT_HOME_KEY;
        } else if (FPSENSOR_KEY_POWER == fpsensor_key.key) {
            key_event = FPSENSOR_INPUT_FF_KEY;
        } else if (FPSENSOR_KEY_CAPTURE == fpsensor_key.key) {
            key_event = FPSENSOR_INPUT_CAMERA_KEY;
        } else {
            /* add special key define */
            key_event = FPSENSOR_INPUT_OTHER_KEY;
        }
        fpsensor_debug(INFO_LOG, "%s: received key event[%d], key=%d, value=%d\n",
                       __func__, key_event, fpsensor_key.key, fpsensor_key.value);
        if ((FPSENSOR_KEY_POWER == fpsensor_key.key || FPSENSOR_KEY_CAPTURE == fpsensor_key.key)
            && (fpsensor_key.value == 1)) {
            input_report_key(fpsensor_dev->input, key_event, 1);
            input_sync(fpsensor_dev->input);
            input_report_key(fpsensor_dev->input, key_event, 0);
            input_sync(fpsensor_dev->input);
        } else if (FPSENSOR_KEY_UP == fpsensor_key.key) {
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_UP_KEY, 1);
            input_sync(fpsensor_dev->input);
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_UP_KEY, 0);
            input_sync(fpsensor_dev->input);
        } else if (FPSENSOR_KEY_DOWN == fpsensor_key.key) {
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_DOWN_KEY, 1);
            input_sync(fpsensor_dev->input);
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_DOWN_KEY, 0);
            input_sync(fpsensor_dev->input);
        } else if (FPSENSOR_KEY_RIGHT == fpsensor_key.key) {
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_RIGHT_KEY, 1);
            input_sync(fpsensor_dev->input);
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_RIGHT_KEY, 0);
            input_sync(fpsensor_dev->input);
        } else if (FPSENSOR_KEY_LEFT == fpsensor_key.key) {
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_LEFT_KEY, 1);
            input_sync(fpsensor_dev->input);
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_LEFT_KEY, 0);
            input_sync(fpsensor_dev->input);
        } else  if (FPSENSOR_KEY_TAP == fpsensor_key.key) {
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_TAP_KEY, 1);
            input_sync(fpsensor_dev->input);
            input_report_key(fpsensor_dev->input, FPSENSOR_NAV_TAP_KEY, 0);
            input_sync(fpsensor_dev->input);
        } else if ((FPSENSOR_KEY_POWER != fpsensor_key.key) && (FPSENSOR_KEY_CAPTURE != fpsensor_key.key)) {
            input_report_key(fpsensor_dev->input, key_event, fpsensor_key.value);
            input_sync(fpsensor_dev->input);
        }
#endif
        break;

    case FPSENSOR_IOC_ENTER_SLEEP_MODE:
        fpsensor_dev->is_sleep_mode = 1;
        break;
    case FPSENSOR_IOC_REMOVE:
#if FPSENSOR_INPUT
        if (fpsensor_dev->input != NULL) {
            input_unregister_device(fpsensor_dev->input);
        }
#endif
        if (fpsensor_dev->device != NULL) {
            device_destroy(fpsensor_dev->class, fpsensor_dev->devno);
        }
        if (fpsensor_dev->class != NULL ) {
            unregister_chrdev_region(fpsensor_dev->devno, 1);
            class_destroy(fpsensor_dev->class);
        }
        if (fpsensor_dev->users == 0) {
#if FPSENSOR_INPUT
            if (fpsensor_dev->input != NULL) {
                input_unregister_device(fpsensor_dev->input);
            }
#endif
            kfree(fpsensor_dev);
        }
        fpsensor_spi_clk_enable(0);
#if FP_NOTIFY
        fb_unregister_client(&fpsensor_dev->notifier);
#endif
        fpsensor_debug(INFO_LOG, "%s remove finished\n", __func__);
        break;

    case FPSENSOR_IOC_CANCEL_WAIT:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR CANCEL WAIT\n", __func__);
        fpsensor_dev->cancel = 1;
        wake_up_interruptible(&fpsensor_dev->wq_irq_return);
        break;
#if FPSENSOR_DUMMYTEE
    case FPSENSOR_IOCTL_W_SENSOR_REG:
        if (copy_from_user(reg_data, (void __user *)arg, 17)) {
            // error = -EFAULT;
            break;
        }
        fpsensor_spi_send_recv(fpsensor_dev, reg_data[0], reg_data + 1, reg_rx);
        break;

    case FPSENSOR_IOCTL_R_SENSOR_REG:
        if (copy_from_user(reg_data, (void __user *)arg, 17)) {
            break;
        }
        fpsensor_spi_send_recv(fpsensor_dev, reg_data[0], reg_data + 1, reg_rx);
        fpsensor_debug(INFO_LOG, "%s: reg_rx : %x   %x   %x  %x\n", __func__, reg_rx[0], reg_rx[1],
                       reg_rx[2], reg_rx[3]);
        if (copy_to_user((void __user *)arg, reg_rx, 16) != 0) {
            break;
        }
        break;
    case FPSENSOR_IOCTL_SEND_CMD:
        if (copy_from_user(reg_data, (void __user *)arg, 1)) {
            break;
        }
        fpsensor_spi_send_recv(fpsensor_dev, 1 , reg_data, NULL);
        break;
    case FPSENSOR_IOCTL_SET_SPI_CLK:
        if (copy_from_user(&val, (void __user *)arg, 4)) {
            retval = -EFAULT;
            break;
        }
        fpsensor_dev->spi_freq_khz = val;
        break;
    case FPSENSOR_IOCTL_FETCH_IMAGE:
        if (copy_from_user(reg_data, (void __user *)arg, 8)) {
            retval = -EFAULT;
            break;
        }
        fpsensor_dev->fetch_image_cmd_len = reg_data[1];
        fpsensor_dev->fetch_image_cmd = reg_data[0];
        break;
#endif
#if FP_NOTIFY
    case FPSENSOR_IOC_GET_FP_STATUS :
        val = fpsensor_dev->fb_status;
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_GET_FP_STATUS  %d \n",__func__, fpsensor_dev->fb_status);
        if (copy_to_user((void __user *)arg, (void *)&val, sizeof(unsigned int))) {
            fpsensor_debug(ERR_LOG, "Failed to copy data to user\n");
            retval = -EFAULT;
            break;
        }
        retval = 0;
        break;
#endif
    default:
        fpsensor_debug(ERR_LOG, "fpsensor doesn't support this command(%d)\n", cmd);
        break;
    }

    FUNC_EXIT();
    return retval;
}
static long fpsensor_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return fpsensor_ioctl(filp, cmd, (unsigned long)(arg));
}

static unsigned int fpsensor_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int ret = 0;
    fpsensor_debug(ERR_LOG, " support poll opertion  in   version\n");
    ret |= POLLIN;
    poll_wait(filp, &g_fpsensor->wq_irq_return, wait);
    if (g_fpsensor->cancel == 1 ) {
        fpsensor_debug(ERR_LOG, " cancle\n");
        ret =  POLLERR;
        g_fpsensor->cancel = 0;
        return ret;
    }
    if ( g_fpsensor->RcvIRQ) {
        fpsensor_debug(ERR_LOG, " get irq\n");
        ret |= POLLRDNORM;
    } else {
        ret = 0;
    }
    return ret;
}


/* -------------------------------------------------------------------- */
/* device function                                                      */
/* -------------------------------------------------------------------- */
static int fpsensor_open(struct inode *inode, struct file *filp)
{
    fpsensor_data_t *fpsensor_dev;

    FUNC_ENTRY();
    fpsensor_dev = container_of(inode->i_cdev, fpsensor_data_t, cdev);
    fpsensor_dev->users++;
    fpsensor_dev->device_available = 1;
    filp->private_data = fpsensor_dev;
    FUNC_EXIT();
    return 0;
}

static int fpsensor_release(struct inode *inode, struct file *filp)
{
    fpsensor_data_t *fpsensor_dev;
    int    status = 0;

    FUNC_ENTRY();
    fpsensor_dev = filp->private_data;
    filp->private_data = NULL;

    /*last close??*/
    fpsensor_dev->users--;
    if (!fpsensor_dev->users) {
        fpsensor_debug(INFO_LOG, "%s, disble_irq. irq = %d\n", __func__, fpsensor_dev->irq);
        fpsensor_disable_irq(fpsensor_dev);
    }
    fpsensor_dev->device_available = 0;
    FUNC_EXIT();
    return status;
}

// static int fpsensor_fasync(int fd, struct file *filp, int on)
// {
//     printk("%s enter \n", __func__);
//     return fasync_helper(fd, filp, on, &fasync_queue);
// }

static const struct file_operations fpsensor_fops = {
    .owner =    THIS_MODULE,

    .write =    fpsensor_write,
    .read =        fpsensor_read,
    .unlocked_ioctl = fpsensor_ioctl,
    .compat_ioctl   = fpsensor_compat_ioctl,
    .open =        fpsensor_open,
    .release =    fpsensor_release,
    .poll    = fpsensor_poll,
    // .fasync         = fpsensor_fasync,
};

static int fpsensor_create_class(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor->class = class_create(THIS_MODULE, FPSENSOR_CLASS_NAME);
    if (IS_ERR(fpsensor->class)) {
        fpsensor_debug(ERR_LOG, "%s, Failed to create class.\n", __func__);
        error = PTR_ERR(fpsensor->class);
    }

    return error;
}

static int fpsensor_create_device(fpsensor_data_t *fpsensor)
{
    int error = 0;


    if (FPSENSOR_MAJOR > 0) {
        //fpsensor->devno = MKDEV(FPSENSOR_MAJOR, fpsensor_device_count++);
        //error = register_chrdev_region(fpsensor->devno,
        //                 1,
        //                 FPSENSOR_DEV_NAME);
    } else {
        error = alloc_chrdev_region(&fpsensor->devno,
                                    fpsensor->device_count++,
                                    1,
                                    FPSENSOR_DEV_NAME);
    }

    if (error < 0) {
        fpsensor_debug(ERR_LOG,
                       "%s: FAILED %d.\n", __func__, error);
        goto out;

    } else {
        fpsensor_debug(INFO_LOG, "%s: major=%d, minor=%d\n",
                       __func__,
                       MAJOR(fpsensor->devno),
                       MINOR(fpsensor->devno));
    }

    fpsensor->device = device_create(fpsensor->class, &(fpsensor->spi->dev), fpsensor->devno,
                                     fpsensor, FPSENSOR_DEV_NAME);

    if (IS_ERR(fpsensor->device)) {
        fpsensor_debug(ERR_LOG, "device_create failed.\n");
        error = PTR_ERR(fpsensor->device);
    }
out:
    return error;
}

extern int get_fp_vendor(void);
enum {
    FP_VENDOR_INVALID = 0,
    FPC_VENDOR,
    ELAN_VENDOR,
    GOODIX_VENDOR,
    CHIPONE_VENDOR
};

#if FP_NOTIFY
static int fpsensor_fb_notifier_callback(struct notifier_block* self,
        unsigned long event, void* data)
{
    static char screen_status[64] = {'\0'};
    char* screen_env[2] = { screen_status, NULL };
    struct fb_event* evdata = data;
    unsigned int blank;
    int retval = 0;
    fpsensor_debug(INFO_LOG,"%s enter.\n", __func__);
    fpsensor_data_t *fpsensor_dev = g_fpsensor;
    if (event != FB_EVENT_BLANK /* FB_EARLY_EVENT_BLANK */) {
        return 0;
    }

    blank = *(int*)evdata->data;
    fpsensor_debug(INFO_LOG,"%s enter, blank=0x%x\n", __func__, blank);

    switch (blank) {
    case FB_BLANK_UNBLANK:
        fpsensor_debug(INFO_LOG,"%s: lcd on notify\n", __func__);
        sprintf(screen_status, "SCREEN_STATUS=%s", "ON");
        fpsensor_dev->fb_status = 1;
        break;

    case FB_BLANK_POWERDOWN:
        fpsensor_debug(INFO_LOG,"%s: lcd off notify\n", __func__);
        sprintf(screen_status, "SCREEN_STATUS=%s", "OFF");
        fpsensor_dev->fb_status = 0;
        break;

    default:
        fpsensor_debug(INFO_LOG,"%s: other notifier, ignore\n", __func__);
        break;
    }

    fpsensor_debug(INFO_LOG,"%s %s leave.\n", screen_status, __func__);
    return retval;
}
#endif

#if defined(USE_SPI_BUS)
static int fpsensor_probe(struct spi_device *spi)
#elif defined(USE_PLATFORM_BUS)
static int fpsensor_probe(struct platform_device *spi)
#endif
{
    struct device *dev = &spi->dev;
    fpsensor_data_t *fpsensor_dev = NULL;
    int error = 0;
    // u16 i = 0;
    // unsigned long minor;f
    int status = -EINVAL;

    FUNC_ENTRY();
    /* Allocate driver data */
    fpsensor_dev = kzalloc(sizeof(*fpsensor_dev), GFP_KERNEL);
    if (!fpsensor_dev) {
        fpsensor_debug(ERR_LOG, "%s, Failed to alloc memory for fpsensor device.\n", __func__);
        FUNC_EXIT();
        return -ENOMEM;
    }
    fpsensor_dev->device = dev ;

    g_fpsensor = fpsensor_dev;
    /* Initialize the driver data */
    mutex_init(&fpsensor_dev->buf_lock);

#if defined(USE_SPI_BUS)
    spi_set_drvdata(spi, fpsensor_dev);
#elif defined(USE_PLATFORM_BUS)
    platform_set_drvdata(spi, fpsensor_dev);
#endif
    fpsensor_dev->spi = spi;
    // INIT_LIST_HEAD(&fpsensor_dev->device_entry);
    fpsensor_dev->device_available = 0;
    fpsensor_dev->irq = 0;
    fpsensor_dev->probe_finish = 0;
    fpsensor_dev->device_count     = 0;
    fpsensor_dev->users = 0;
    fpsensor_dev->spi_freq_khz = 8000u;
    fpsensor_dev->fetch_image_cmd = 0x2c;
    fpsensor_dev->fetch_image_cmd_len = 2;
    /*setup fpsensor configurations.*/
    fpsensor_debug(INFO_LOG, "%s, Setting fpsensor device configuration.\n", __func__);
    // fpsensor_irq_gpio_cfg();
    // fpsensor_reset_gpio_cfg();
    // dts read
    spi->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
    fpsensor_dev->pinctrl1 = devm_pinctrl_get(&spi->dev);
    if (IS_ERR(fpsensor_dev->pinctrl1)) {
        error = PTR_ERR(fpsensor_dev->pinctrl1);
        fpsensor_debug(ERR_LOG, "fpsensor Cannot find fp pinctrl1.\n");
        goto err1;
    }
    fpsensor_spidev_dts_init(fpsensor_dev);
#if FPSENSOR_DUMMYTEE
    fpsensor_spi_setup(fpsensor_dev);
    fpsensor_manage_image_buffer(fpsensor_dev, 160 * 160 * 2);
    fpsensor_hw_reset(1250);
    if (fpsensor_check_HWID(fpsensor_dev) == 0) {
        fpsensor_debug(ERR_LOG, "get chip id error .\n");
        goto err2;
    }
#endif
    error = fpsensor_create_class(fpsensor_dev);
    if (error) {
        goto err2;
    }
    error = fpsensor_create_device(fpsensor_dev);
    if (error) {
        goto err2;
    }
    cdev_init(&fpsensor_dev->cdev, &fpsensor_fops);
    fpsensor_dev->cdev.owner = THIS_MODULE;
    error = cdev_add(&fpsensor_dev->cdev, fpsensor_dev->devno, 1);
    if (error) {
        goto err2;
    }
    //register input device
#if FPSENSOR_INPUT
    fpsensor_dev->input = input_allocate_device();
    if (fpsensor_dev->input == NULL) {
        fpsensor_debug(ERR_LOG, "%s, Failed to allocate input device.\n", __func__);
        error = -ENOMEM;
        goto err2;
    }
    __set_bit(EV_KEY, fpsensor_dev->input->evbit);
    __set_bit(FPSENSOR_INPUT_HOME_KEY, fpsensor_dev->input->keybit);

    __set_bit(FPSENSOR_INPUT_MENU_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_INPUT_BACK_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_INPUT_FF_KEY, fpsensor_dev->input->keybit);

    __set_bit(FPSENSOR_NAV_TAP_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_NAV_UP_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_NAV_DOWN_KEY, fpsensor_dev->input->keyCHIPONE_DEV_NAMEbit);
    __set_bit(FPSENSOR_NAV_RIGHT_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_NAV_LEFT_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_INPUT_CAMERA_KEY, fpsensor_dev->input->keybit);
    fpsensor_dev->input->name = FPSENSOR_INPUT_NAME;
    if (input_register_device(fpsensor_dev->input)) {
        fpsensor_debug(ERR_LOG, "%s, Failed to register input device.\n", __func__);
        error = -ENODEV;
        goto err1;
    }
#endif
    fpsensor_dev->device_available = 1;
    fpsensor_dev->irq_count = 1;
    // mt_eint_unmask(fpsensor_dev->irq);

    fpsensor_dev->sig_count = 0;

    fpsensor_debug(INFO_LOG, "%s: fpsensor init finished======\n", __func__);


    fpsensor_dev->probe_finish = 1;
    fpsensor_dev->is_sleep_mode = 0;
//fpsensor_hw_power_enable(1);
    fpsensor_spi_clk_enable(1);

    //init wait queue
    init_waitqueue_head(&fpsensor_dev->wq_irq_return);

    fpsensor_debug(INFO_LOG, "%s probe finished, normal driver version: %s\n", __func__,
                   FPSENSOR_SPI_VERSION);
#if FP_NOTIFY
    fpsensor_dev->notifier.notifier_call = fpsensor_fb_notifier_callback;
    fb_register_client(&fpsensor_dev->notifier);
#endif
    FUNC_EXIT();
    return 0;
err1:
#if FPSENSOR_INPUT
    input_free_device(fpsensor_dev->input);
#endif
    kfree(fpsensor_dev);
    FUNC_EXIT();
    return status;
err2:
    device_destroy(fpsensor_dev->class, fpsensor_dev->devno);
    //fpsensor_hw_power_enable(0);
    fpsensor_spi_clk_enable(0);
    kfree(fpsensor_dev);
    FUNC_EXIT();
    return status;
}

#if defined(USE_SPI_BUS)
static int fpsensor_remove(struct spi_device *spi)
#elif defined(USE_PLATFORM_BUS)
static int fpsensor_remove(struct platform_device *spi)
#endif
{
#if defined(USE_PLATFORM_BUS)
    fpsensor_data_t *fpsensor_dev = g_fpsensor;
#elif defined(USE_SPI_BUS)
    fpsensor_data_t *fpsensor_dev = spi_get_drvdata(spi);
#endif

    FUNC_ENTRY();

    /* make sure ops on existing fds can abort cleanly */
    if (fpsensor_dev->irq) {
        free_irq(fpsensor_dev->irq, fpsensor_dev);
    }

    fpsensor_dev->spi = NULL;
    spi_set_drvdata(spi, NULL);
    device_destroy(fpsensor_dev->class, fpsensor_dev->devno);
    unregister_chrdev_region(fpsensor_dev->devno, 1);
    class_destroy(fpsensor_dev->class);
    if (fpsensor_dev->users == 0) {
#if FPSENSOR_INPUT
        if (fpsensor_dev->input != NULL) {
            input_unregister_device(fpsensor_dev->input);
        }
#endif
    }
#if FP_NOTIFY
    fb_unregister_client(&fpsensor_dev->notifier);
#endif

    fpsensor_debug(INFO_LOG, "%s remove finished\n", __func__);
    kfree(fpsensor_dev);
#if 0
    if (s_DEVINFO_fpsensor != NULL) {
        kfree(s_DEVINFO_fpsensor);
    }
#endif
    FUNC_EXIT();
    return 0;
}
#ifdef CONFIG_PM
#if defined(USE_PLATFORM_BUS)
static int fpsensor_suspend(struct platform_device *pdev, pm_message_t state)
#else
static int fpsensor_suspend(struct device *pdev)
#endif
{
    int irqf = 0;

    fpsensor_debug(INFO_LOG, "%s\n", __func__);

#if SLEEP_WAKEUP_HIGH_LEV
    irqf = IRQF_TRIGGER_HIGH | IRQF_ONESHOT | IRQF_NO_SUSPEND;
    fpsensor_disable_irq(g_fpsensor);
    irq_set_irq_type(g_fpsensor->irq, irqf);
    g_fpsensor->suspend_flag = 1;
    fpsensor_enable_irq(g_fpsensor);
#endif

    fpsensor_debug(INFO_LOG, "%s exit\n", __func__);
    //fpsensor_debug(INFO_LOG, "%s, exit irq is %d\n", __func__, gpio_get_value(g_fpsensor->irq_gpio));
    return 0;
}

#if defined(USE_PLATFORM_BUS)
static int fpsensor_resume(struct platform_device *pdev)
#else
static int fpsensor_resume(struct device *pdev)
#endif
{
    fpsensor_debug(INFO_LOG, "%s\n", __func__);
    return 0;
}
#endif

#ifdef CONFIG_OF
static struct of_device_id fpsensor_of_match[] = {
    { .compatible = "mediatek,fingerprint", },
    {}
};
MODULE_DEVICE_TABLE(of, fpsensor_of_match);
#endif
struct spi_device_id fpsensor_spi_id_table = {FPSENSOR_DEV_NAME, 0};

#if defined(USE_SPI_BUS)
static const struct dev_pm_ops fpsensor_pm = {
    .suspend = fpsensor_suspend,
    .resume = fpsensor_resume
};

static struct spi_driver fpsensor_spi_driver = {
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        .bus = &spi_bus_type,
        .owner = THIS_MODULE,
        // .mode = SPI_MODE_0,
        // .controller_data = &fpsensor_spi_conf_mt65xx, //&spi_conf
#ifdef CONFIG_PM
        .pm = &fpsensor_pm,
#endif
#ifdef CONFIG_OF
        .of_match_table = fpsensor_of_match,
#endif
    },
    .id_table = &fpsensor_spi_id_table,
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
};
#elif defined(USE_PLATFORM_BUS)
/*
static const struct dev_pmm fpsensor_pm_ops = {
    .suspend = NULL,
    .resume = NULL,
};
*/
static struct platform_driver fpsensor_plat_driver = {
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
#ifdef CONFIG_PM
    .suspend = fpsensor_suspend,
    .resume = fpsensor_resume,
#endif
    .shutdown = NULL,
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        //.pm = &fpsensor_pm_ops,
        .owner = THIS_MODULE,
        //.bus  = &spi_bus_type,
#ifdef CONFIG_OF
        .of_match_table = fpsensor_of_match,
#endif /* CONFIG_OF */
    },
};
#endif

static int __init fpsensor_init(void)
{
    int status;

    FUNC_ENTRY();

#if defined(USE_PLATFORM_BUS)
    status = platform_driver_register(&fpsensor_plat_driver);
#elif defined(USE_SPI_BUS)
#if FPSENSOR_SPI_BUS_DYNAMIC
    spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
#endif
    status = spi_register_driver(&fpsensor_spi_driver);
#endif

    if (status < 0) {
        fpsensor_debug(ERR_LOG, "%s, Failed to register SPI driver.\n", __func__);
    }

    FUNC_EXIT();
    return status;
}
module_init(fpsensor_init);

static void __exit fpsensor_exit(void)
{
    FUNC_ENTRY();

#if defined(USE_PLATFORM_BUS)
    platform_driver_unregister(&fpsensor_plat_driver);
#elif defined(USE_SPI_BUS)
    spi_unregister_driver(&fpsensor_spi_driver);
#endif

    FUNC_EXIT();
}
module_exit(fpsensor_exit);

MODULE_AUTHOR("xhli");
MODULE_DESCRIPTION(" Fingerprint chip driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:fpsensor_spi");
