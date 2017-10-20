#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <net/sock.h>


#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "fpsensor_spi.h"
#include "fpsensor_platform.h"
#if FP_NOTIFY
#include <linux/fb.h>
#include <linux/notifier.h>
#endif
#define FPSENSOR_SPI_VERSION              "fpsensor_spi_v0.20"
//#define FPSENSOR_USE_QCOM_POWER_GPIO    1

/* debug log setting */
u8 fpsensor_debug_level = DEBUG_LOG;
/* global variables */
static fpsensor_data_t *g_fpsensor = NULL;

/* -------------------------------------------------------------------- */
/* fingerprint chip hardware configuration                              */
/* -------------------------------------------------------------------- */
static void fpsensor_gpio_free(fpsensor_data_t *fpsensor)
{
    struct device *dev = &fpsensor->spi->dev;

    if (fpsensor->irq_gpio != 0 ) {
        devm_gpio_free(dev, fpsensor->irq_gpio);
        fpsensor->irq_gpio = 0;
    }
    if (fpsensor->reset_gpio != 0) {
        devm_gpio_free(dev, fpsensor->reset_gpio);
        fpsensor->reset_gpio = 0;
    }
#ifdef FPSENSOR_USE_QCOM_POWER_GPIO
    if (fpsensor->power_gpio != 0) {
        devm_gpio_free(dev, fpsensor->power_gpio);
        fpsensor->power_gpio = 0;
    }
#endif
}

static void fpsensor_irq_gpio_cfg(fpsensor_data_t *fpsensor)
{
    int error = 0;

    error = gpio_direction_input(fpsensor->irq_gpio);
    if (error) {
        fpsensor_debug(ERR_LOG, "setup fpsensor irq gpio for input failed!error[%d]\n", error);
        return ;
    }

    fpsensor->irq = gpio_to_irq(fpsensor->irq_gpio);
    fpsensor_debug(DEBUG_LOG, "fpsensor irq number[%d]\n", fpsensor->irq);
    if (fpsensor->irq <= 0) {
        fpsensor_debug(ERR_LOG, "fpsensor irq gpio to irq failed!\n");
        return ;
    }

    return;
}

static int fpsensor_request_named_gpio(fpsensor_data_t *fpsensor_dev, const char *label, int *gpio)
{
    struct device *dev = &fpsensor_dev->spi->dev;
    struct device_node *np = dev->of_node;
    int ret = of_get_named_gpio(np, label, 0);

    if (ret < 0) {
        fpsensor_debug(ERR_LOG, "failed to get '%s'\n", label);
        return ret;
    }
    *gpio = ret;
    ret = devm_gpio_request(dev, *gpio, label);
    if (ret) {
        fpsensor_debug(ERR_LOG, "failed to request gpio %d\n", *gpio);
        return ret;
    }

    fpsensor_debug(ERR_LOG, "%s %d\n", label, *gpio);
    return ret;
}

static int fpsensor_get_gpio_dts_info(fpsensor_data_t *fpsensor)
{
    int ret = 0;

    FUNC_ENTRY();
    // get interrupt gpio resource
    ret = fpsensor_request_named_gpio(fpsensor, "fp-gpio-int", &fpsensor->irq_gpio);
    if (ret) {
        fpsensor_debug(ERR_LOG, "Failed to request irq GPIO. ret = %d\n", ret);
        return -1;
    }

    // get reest gpio resourece
    ret = fpsensor_request_named_gpio(fpsensor, "fp-gpio-reset", &fpsensor->reset_gpio);
    if (ret) {
        fpsensor_debug(ERR_LOG, "Failed to request reset GPIO. ret = %d\n", ret);
        return -1;
    }
#ifdef FPSENSOR_USE_QCOM_POWER_GPIO
    // get power gpio resourece
    ret = fpsensor_request_named_gpio(fpsensor, "fp-gpio-power", &fpsensor->power_gpio);
    if (ret) {
        fpsensor_debug(ERR_LOG, "Failed to request power GPIO. ret = %d\n", ret);
        return -1;
    }
    // set power direction output
    gpio_direction_output(fpsensor->power_gpio, 1);
    gpio_set_value(fpsensor->power_gpio, 1);
#endif
    // set reset direction output
    gpio_direction_output(fpsensor->reset_gpio, 1);
    fpsensor_hw_reset(1250);

    return ret;
}

/* delay us after reset */
static void fpsensor_hw_reset(int delay)
{
    FUNC_ENTRY();
    gpio_set_value(g_fpsensor->reset_gpio, 1);

    udelay(100);
    gpio_set_value(g_fpsensor->reset_gpio, 0);

    udelay(1000);
    gpio_set_value(g_fpsensor->reset_gpio, 1);

    if (delay) {
        udelay(delay);
    }

    FUNC_EXIT();
    return;
}

static void fpsensor_hw_power_enable(u8 onoff)
{
    return ;
}

static void fpsensor_enable_irq(fpsensor_data_t *fpsensor_dev)
{
    FUNC_ENTRY();
    setRcvIRQ(0);
    /* Request that the interrupt should be wakeable */
    if (fpsensor_dev->irq_enabled == 0) {
        enable_irq(fpsensor_dev->irq);
        fpsensor_dev->irq_enabled = 1;
    }
    FUNC_EXIT();
    return;
}

static void fpsensor_disable_irq(fpsensor_data_t *fpsensor_dev)
{
    FUNC_ENTRY();

    if (0 == fpsensor_dev->device_available) {
        fpsensor_debug(ERR_LOG, "%s, devices not available\n", __func__);
        goto out;
    }

    if (0 == fpsensor_dev->irq_enabled) {
        fpsensor_debug(ERR_LOG, "%s, irq already disabled\n", __func__);
        goto out;
    }

    if (fpsensor_dev->irq) {
        disable_irq_nosync(fpsensor_dev->irq);
        fpsensor_debug(DEBUG_LOG, "%s disable interrupt!\n", __func__);
    }
    fpsensor_dev->irq_enabled = 0;

out:
    setRcvIRQ(0);
    FUNC_EXIT();
    return;
}

static irqreturn_t fpsensor_irq(int irq, void *handle)
{
    fpsensor_data_t *fpsensor_dev = (fpsensor_data_t *)handle;

    /* Make sure 'wakeup_enabled' is updated before using it
    ** since this is interrupt context (other thread...) */
    smp_rmb();
    wake_lock_timeout(&fpsensor_dev->ttw_wl, msecs_to_jiffies(1000));
    setRcvIRQ(1);
    wake_up_interruptible(&fpsensor_dev->wq_irq_return);

    return IRQ_HANDLED;
}

static void setRcvIRQ(int val)
{
    fpsensor_data_t *fpsensor_dev = g_fpsensor;
    fpsensor_dev->RcvIRQ = val;
}

static long fpsensor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    fpsensor_data_t *fpsensor_dev = NULL;
    int retval = 0;
    unsigned int val = 0;
    int irqf;
#if FPSENSOR_DUMMYTEE
    u8 reg_data[17] = {0};
    u8 reg_rx[16] = {0};
#endif


    fpsensor_debug(INFO_LOG, "[rickon]: fpsensor ioctl cmd : 0x%x \n", cmd );
    fpsensor_dev = (fpsensor_data_t *)filp->private_data;
    fpsensor_dev->cancel = 0 ;
    switch (cmd) {
    case FPSENSOR_IOC_INIT:
        fpsensor_debug(INFO_LOG, "%s: fpsensor init started======\n", __func__);
        retval = fpsensor_get_gpio_dts_info(fpsensor_dev);
        if (retval) {
            break;
        }
        fpsensor_irq_gpio_cfg(fpsensor_dev);
        //regist irq
        irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT;
        retval = devm_request_threaded_irq(&g_fpsensor->spi->dev, g_fpsensor->irq, fpsensor_irq,
                                           NULL, irqf, dev_name(&g_fpsensor->spi->dev), g_fpsensor);
        if (retval == 0) {
            fpsensor_debug(INFO_LOG, " irq thread reqquest success!\n");
        } else {
            fpsensor_debug(INFO_LOG, " irq thread request failed , retval =%d \n", retval);
        }
        enable_irq_wake(g_fpsensor->irq);
        fpsensor_dev->device_available = 1;
        fpsensor_disable_irq(fpsensor_dev);
        fpsensor_debug(INFO_LOG, "%s: fpsensor init finished======\n", __func__);
        break;

    case FPSENSOR_IOC_EXIT:
        fpsensor_disable_irq(fpsensor_dev);
        if (fpsensor_dev->irq) {
            free_irq(fpsensor_dev->irq, fpsensor_dev);
            fpsensor_dev->irq_enabled = 0;
        }
        fpsensor_dev->device_available = 0;
        fpsensor_gpio_free(fpsensor_dev);
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
        val = gpio_get_value(fpsensor_dev->irq_gpio);
        if (copy_to_user((void __user *)arg, (void *)&val, sizeof(unsigned int))) {
            fpsensor_debug(ERR_LOG, "Failed to copy data to user\n");
            retval = -EFAULT;
            break;
        }
        retval = 0;
        break;
    case FPSENSOR_IOC_ENABLE_SPI_CLK:
        fpsensor_debug(INFO_LOG, "%s: ENABLE_SPI_CLK ======\n", __func__);
        break;
    case FPSENSOR_IOC_DISABLE_SPI_CLK:
        fpsensor_debug(INFO_LOG, "%s: DISABLE_SPI_CLK ======\n", __func__);
        break;
    case FPSENSOR_IOC_ENABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_ENABLE_POWER ======\n", __func__);
        fpsensor_hw_power_enable(1);
        break;
    case FPSENSOR_IOC_DISABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_DISABLE_POWER ======\n", __func__);
        fpsensor_hw_power_enable(0);
        break;
    case FPSENSOR_IOC_REMOVE:
        fpsensor_disable_irq(fpsensor_dev);
        if (fpsensor_dev->irq) {
            free_irq(fpsensor_dev->irq, fpsensor_dev);
            fpsensor_dev->irq_enabled = 0;
        }
        fpsensor_dev->device_available = 0;
        fpsensor_gpio_free(fpsensor_dev);
        fpsensor_dev_cleanup(fpsensor_dev);
#if FP_NOTIFY
        fb_unregister_client(&fpsensor_dev->notifier);
#endif
        fpsensor_debug(INFO_LOG, "%s remove finished\n", __func__);
        break;
    case FPSENSOR_IOC_CANCEL_WAIT:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR CANCEL WAIT\n", __func__);
        wake_up_interruptible(&fpsensor_dev->wq_irq_return);
        fpsensor_dev->cancel = 1;
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

    //FUNC_EXIT();
    return retval;
}

static long fpsensor_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return fpsensor_ioctl(filp, cmd, (unsigned long)(arg));
}

static unsigned int fpsensor_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int ret = 0;

    ret |= POLLIN;
    poll_wait(filp, &g_fpsensor->wq_irq_return, wait);
    if (g_fpsensor->cancel == 1) {
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
    if (fpsensor_dev->users <= 0) {
        fpsensor_debug(INFO_LOG, "%s, disble_irq. irq = %d\n", __func__, fpsensor_dev->irq);
        fpsensor_disable_irq(fpsensor_dev);
    }
    fpsensor_dev->device_available = 0;
    FUNC_EXIT();
    return status;
}

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

static ssize_t fpsensor_write(struct file *filp, const char __user *buf, size_t count,
                              loff_t *f_pos)
{
    fpsensor_debug(ERR_LOG, "Not support write opertion in TEE version\n");
    return -EFAULT;
}

static const struct file_operations fpsensor_fops = {
    .owner          = THIS_MODULE,
    .write          = fpsensor_write,
    .read           = fpsensor_read,
    .unlocked_ioctl = fpsensor_ioctl,
    .compat_ioctl   = fpsensor_compat_ioctl,
    .open           = fpsensor_open,
    .release        = fpsensor_release,
    .poll           = fpsensor_poll,

};

// create and register a char device for fpsensor
static int fpsensor_dev_setup(fpsensor_data_t *fpsensor)
{
    int ret = 0;
    dev_t dev_no = 0;
    struct device *dev = NULL;
    int fpsensor_dev_major = FPSENSOR_DEV_MAJOR;
    int fpsensor_dev_minor = 0;

    FUNC_ENTRY();

    if (fpsensor_dev_major) {
        dev_no = MKDEV(fpsensor_dev_major, fpsensor_dev_minor);
        ret = register_chrdev_region(dev_no, FPSENSOR_NR_DEVS, FPSENSOR_DEV_NAME);
    } else {
        ret = alloc_chrdev_region(&dev_no, fpsensor_dev_minor, FPSENSOR_NR_DEVS, FPSENSOR_DEV_NAME);
        fpsensor_dev_major = MAJOR(dev_no);
        fpsensor_dev_minor = MINOR(dev_no);
        fpsensor_debug(INFO_LOG, "fpsensor device major is %d, minor is %d\n",
                       fpsensor_dev_major, fpsensor_dev_minor);
    }

    if (ret < 0) {
        fpsensor_debug(ERR_LOG, "can not get device major number %d\n", fpsensor_dev_major);
        goto out;
    }

    cdev_init(&fpsensor->cdev, &fpsensor_fops);
    fpsensor->cdev.owner = THIS_MODULE;
    fpsensor->cdev.ops   = &fpsensor_fops;
    fpsensor->devno      = dev_no;
    ret = cdev_add(&fpsensor->cdev, dev_no, FPSENSOR_NR_DEVS);
    if (ret) {
        fpsensor_debug(ERR_LOG, "add char dev for fpsensor failed\n");
        goto release_region;
    }

    fpsensor->class = class_create(THIS_MODULE, FPSENSOR_CLASS_NAME);
    if (IS_ERR(fpsensor->class)) {
        fpsensor_debug(ERR_LOG, "create fpsensor class failed\n");
        ret = PTR_ERR(fpsensor->class);
        goto release_cdev;
    }

    dev = device_create(fpsensor->class, &fpsensor->spi->dev, dev_no, fpsensor, FPSENSOR_DEV_NAME);
    if (IS_ERR(dev)) {
        fpsensor_debug(ERR_LOG, "create device for fpsensor failed\n");
        ret = PTR_ERR(dev);
        goto release_class;
    }
    FUNC_EXIT();
    return ret;

release_class:
    class_destroy(fpsensor->class);
    fpsensor->class = NULL;
release_cdev:
    cdev_del(&fpsensor->cdev);
release_region:
    unregister_chrdev_region(dev_no, FPSENSOR_NR_DEVS);
out:
    FUNC_EXIT();
    return ret;
}

// release and cleanup fpsensor char device
static void fpsensor_dev_cleanup(fpsensor_data_t *fpsensor)
{
    FUNC_ENTRY();

    cdev_del(&fpsensor->cdev);
    unregister_chrdev_region(fpsensor->devno, FPSENSOR_NR_DEVS);
    device_destroy(fpsensor->class, fpsensor->devno);
    class_destroy(fpsensor->class);

    FUNC_EXIT();
}

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
static int fpsensor_probe(struct spi_device *pdev)
#elif defined(USE_PLATFORM_BUS)
static int fpsensor_probe(struct platform_device *pdev)
#endif
{
    int status = 0;
    fpsensor_data_t *fpsensor_dev = NULL;

    FUNC_ENTRY();

    /* Allocate driver data */
    fpsensor_dev = kzalloc(sizeof(*fpsensor_dev), GFP_KERNEL);
    if (!fpsensor_dev) {
        status = -ENOMEM;
        fpsensor_debug(ERR_LOG, "%s, Failed to alloc memory for fpsensor device.\n", __func__);
        goto out;
    }

    /* Initialize the driver data */
    g_fpsensor = fpsensor_dev;
    fpsensor_dev->spi               = pdev ;
    fpsensor_dev->device_available  = 0;
    fpsensor_dev->users             = 0;
    fpsensor_dev->irq               = 0;
    fpsensor_dev->power_gpio        = 0;
    fpsensor_dev->reset_gpio        = 0;
    fpsensor_dev->irq_gpio          = 0;
    fpsensor_dev->irq_enabled       = 0;
#if FPSENSOR_DUMMYTEE
    fpsensor_dev->spi_freq_khz = 8000u;
    fpsensor_dev->fetch_image_cmd = 0x2c;
    fpsensor_dev->fetch_image_cmd_len = 2;
    spi_set_drvdata(pdev, fpsensor_dev);
    status = fpsensor_get_gpio_dts_info(fpsensor_dev);
    if (status) {
        fpsensor_debug(ERR_LOG, "fpsensor get DTS info failed\n");
        goto release_drv_data;
    }
    fpsensor_spi_setup(fpsensor_dev);
    fpsensor_manage_image_buffer(fpsensor_dev, 160 * 160 * 2);
    fpsensor_hw_reset(1250);
    if (fpsensor_check_HWID(fpsensor_dev) == 0) {
        fpsensor_debug(ERR_LOG, "get chip id error .\n");
        goto release_drv_data;
    }
#endif
    /* setup a char device for fpsensor */
    status = fpsensor_dev_setup(fpsensor_dev);
    if (status) {
        fpsensor_debug(ERR_LOG, "fpsensor setup char device failed, %d", status);
        goto release_drv_data;
    }
    init_waitqueue_head(&fpsensor_dev->wq_irq_return);
    wake_lock_init(&g_fpsensor->ttw_wl, WAKE_LOCK_SUSPEND, "fpsensor_ttw_wl");
    fpsensor_dev->device_available = 1;
#if FP_NOTIFY
    fpsensor_dev->notifier.notifier_call = fpsensor_fb_notifier_callback;
    fb_register_client(&fpsensor_dev->notifier);
#endif
    fpsensor_debug(INFO_LOG, "%s finished, driver version: %s\n", __func__, FPSENSOR_SPI_VERSION);
    goto out;

release_drv_data:
    kfree(fpsensor_dev);
    fpsensor_dev = NULL;
out:
    FUNC_EXIT();
    return status;
}

#if defined(USE_SPI_BUS)
static int fpsensor_remove(struct spi_device *pdev)
#elif defined(USE_PLATFORM_BUS)
static int fpsensor_remove(struct platform_device *pdev)
#endif
{
    fpsensor_data_t *fpsensor_dev = g_fpsensor;

    FUNC_ENTRY();
    fpsensor_disable_irq(fpsensor_dev);
    if (fpsensor_dev->irq) {
        free_irq(fpsensor_dev->irq, fpsensor_dev);
    }
#if FP_NOTIFY
    fb_unregister_client(&fpsensor_dev->notifier);
#endif
    fpsensor_gpio_free(fpsensor_dev);
    fpsensor_dev_cleanup(fpsensor_dev);
    wake_lock_destroy(&fpsensor_dev->ttw_wl);
    kfree(fpsensor_dev);
    g_fpsensor = NULL;

    FUNC_EXIT();
    return 0;
}




static struct of_device_id fpsensor_of_match[] = {
    { .compatible = "qcom,fingerprint-gpio" },
    {}
};
MODULE_DEVICE_TABLE(of, fpsensor_of_match);


struct spi_device_id fpsensor_spi_id_table = {FPSENSOR_DEV_NAME, 0};

#if defined(USE_SPI_BUS)
static struct spi_driver fpsensor_spi_driver = {
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        .bus = &spi_bus_type,
        .owner = THIS_MODULE,
        //.mode = SPI_MODE_0,
#ifdef CONFIG_PM
        //.suspend = fpsensor_suspend,
        //.resume = fpsensor_resume,
#endif
        .of_match_table = fpsensor_of_match,
    },
    .id_table = &fpsensor_spi_id_table,
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
};
#elif defined(USE_PLATFORM_BUS)
static struct platform_driver fpsensor_driver = {
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(fpsensor_of_match),
    },
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
    .suspend = fpsensor_suspend,
    .resume = fpsensor_resume,
};
#endif
#define FPSENSOR_SPI_BUS_DYNAMIC 1
#if FPSENSOR_SPI_BUS_DYNAMIC
static struct spi_board_info spi_board_devs[] __initdata = {
    [0] = {
        .modalias = FPSENSOR_DEV_NAME,
        .bus_num = 0,
        .chip_select = 0,
        .mode = SPI_MODE_0,
    },
};
#endif
static int __init fpsensor_init(void)
{
    int status;

#if defined(USE_PLATFORM_BUS)
    status = platform_driver_register(&fpsensor_driver);
#elif defined(USE_SPI_BUS)
#if FPSENSOR_SPI_BUS_DYNAMIC
    spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
#endif
    status = spi_register_driver(&fpsensor_spi_driver);
#endif
    if (status < 0) {
        fpsensor_debug(ERR_LOG, "%s, Failed to register SPI driver.\n", __func__);
    }

    return status;
}
module_init(fpsensor_init);

static void __exit fpsensor_exit(void)
{
#if defined(USE_PLATFORM_BUS)
    platform_driver_unregister(&fpsensor_plat_driver);
#elif defined(USE_SPI_BUS)
    spi_unregister_driver(&fpsensor_spi_driver);
#endif
}
module_exit(fpsensor_exit);

MODULE_AUTHOR("xhli");
MODULE_DESCRIPTION(" Fingerprint chip TEE driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:fpsensor-drivers");
