/*
 * Copyright (C) 2012-2022 fpsensor Technology (Beijing) Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <net/sock.h>
#include <linux/compat.h>
#include <linux/notifier.h>
#include "fpsensor_spi.h"
#include "fpsensor_platform.h"
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#endif
#if FP_NOTIFY
#include <linux/fb.h>
#include <linux/notifier.h>
#endif
#if FPSENSOR_USE_DTS == 0
#define FP_INT_GPIO             155
#define FP_RST_GPIO             156
#endif

/* debug log setting */
u8 fpsensor_debug_level = INFO_LOG;

/*platform device name*/
#define FPSENSOR_DEV_NAME       "fpsensor"
/*device name after register in charater*/
#define FPSENSOR_CLASS_NAME     "fpsensor"
#define FPSENSOR_MAJOR          255
#define N_SPI_MINORS            32    /* ... up to 256 */
static DECLARE_BITMAP(minors, N_SPI_MINORS);
#define FPSENSOR_SPI_VERSION "fpsensor_spi_tee_v0.11"
#define FPSENSOR_INPUT_NAME  "fpsensor_keys"

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

#define FPSENSOR_NAV_UP_KEY     19  /*KEY_UP*/
#define FPSENSOR_NAV_DOWN_KEY   20  /*KEY_DOWN*/
#define FPSENSOR_NAV_LEFT_KEY   21  /*KEY_LEFT*/
#define FPSENSOR_NAV_RIGHT_KEY  22  /*KEY_RIGHT*/
#define FPSENSOR_NAV_TAP_KEY    23

/*************************************************************/
/* global variables                         */
fpsensor_data_t *g_fpsensor = NULL;
EXPORT_SYMBOL(g_fpsensor);

static unsigned bufsiz = (150 * 150);
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");



/* -------------------------------------------------------------------- */
/* fingerprint chip hardware configuration                                  */
/* -------------------------------------------------------------------- */
static void fpsensor_gpio_free(fpsensor_data_t *fpsensor_dev)
{
    if (g_fpsensor->irq_gpio != 0) {
        //devm_gpio_free(dev, g_fpsensor->irq_gpio);
        gpio_free(g_fpsensor->irq_gpio);
    }
    if (g_fpsensor->reset_gpio != 0) {
        //devm_gpio_free(dev, g_fpsensor->reset_gpio);
        gpio_free(g_fpsensor->reset_gpio);
    }
}

static void fpsensor_irq_gpio_cfg(void)
{
    int error;

    fpsensor_debug(DEBUG_LOG, "********(%d)(%s)\n", __LINE__, __func__);
    error = gpio_direction_input(g_fpsensor->irq_gpio);

    if (error) {
        fpsensor_debug(ERR_LOG, "setup fp interrupt gpio for input failed!error[%d]\n", error);
        return ;
    }

    fpsensor_debug(DEBUG_LOG, "********(%d)(%s)\n", __LINE__, __func__);
    g_fpsensor->irq = gpio_to_irq(g_fpsensor->irq_gpio);
    fpsensor_debug(INFO_LOG, "fp interrupt irq number[%d]\n", g_fpsensor->irq);

    if (g_fpsensor->irq < 0) {
        fpsensor_debug(ERR_LOG, "fp interrupt gpio to irq failed!\n");
        return ;
    }

    return;
}

static int fpsensor_request_named_gpio(fpsensor_data_t *fpsensor_dev, const char *label, int *gpio,
                                       int gpio_index)
{
    struct device *dev = &fpsensor_dev->platform_device->dev;
    int rc = 0;
#if FPSENSOR_USE_DTS
    struct device_node *np = dev->of_node;
    rc = of_get_named_gpio(np, label, 0);
    if (rc < 0) {
        fpsensor_debug(ERR_LOG, "failed to get '%s'\n", label);
        return rc;
    }
    *gpio = rc;
    rc = gpio_request(*gpio, label);
    if (rc) {
        fpsensor_debug(ERR_LOG, "failed to request gpio %d\n", *gpio);
        return rc;
    }
#else
    if (gpio_index == 1) { //reset
        *gpio = FP_RST_GPIO;
    } else if (gpio_index == 0) { //irq
        *gpio = FP_INT_GPIO;
    }
    rc = gpio_request(*gpio, label);
#endif
    fpsensor_debug(ERR_LOG, "%s %d\n", label, *gpio);
    return 0;
}

/* delay ms after reset */
static void fpsensor_hw_reset(int delay)
{
    FUNC_ENTRY();
    gpio_set_value(g_fpsensor->reset_gpio, 1);

    udelay(100);
    gpio_set_value(g_fpsensor->reset_gpio, 0);

    udelay(1000);
    gpio_set_value(g_fpsensor->reset_gpio, 1);

    if (delay) {
        udelay(delay);/* delay is configurable */
    }
    FUNC_EXIT();
    return;
}

static int fpsensor_get_gpio_dts_info(void)
{
    int rc = 0;

    FUNC_ENTRY();
    /*get reset resource*/
    rc = fpsensor_request_named_gpio(g_fpsensor, "fpint-gpios", &g_fpsensor->irq_gpio, 0);
    if (rc) {
        fpsensor_debug(ERR_LOG, "Failed to request irq GPIO. rc = %d\n", rc);
        return -1;
    }

    rc = fpsensor_request_named_gpio(g_fpsensor, "fpreset-gpios", &g_fpsensor->reset_gpio, 1);
    if (rc) {
        fpsensor_debug(ERR_LOG, "Failed to request reset GPIO. rc = %d\n", rc);
        return -1;
    }
    gpio_direction_output(g_fpsensor->reset_gpio, 1);

    return rc;
}

static void fpsensor_spi_clk_enable(u8 bonoff)
{

}

static void fpsensor_hw_power_enable(u8 onoff)
{

}

static void fpsensor_enable_irq(fpsensor_data_t *fpsensor_dev)
{
    FUNC_ENTRY();
    setRcvIRQ(0);
    /* Request that the interrupt should be wakeable */
    if (fpsensor_dev->irq_count == 0) {
        enable_irq(fpsensor_dev->irq);
        fpsensor_dev->irq_count = 1;
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
            disable_irq_nosync(fpsensor_dev->irq);
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
    return -EFAULT;
#endif
}

static ssize_t fpsensor_write(struct file *filp, const char __user *buf, size_t count,
                              loff_t *f_pos)
{
    fpsensor_debug(ERR_LOG, "Not support write opertion in TEE version\n");
    return -EFAULT;
}

static irqreturn_t fpsensor_irq(int irq, void *handle)
{
    fpsensor_data_t *fpsensor_dev = (fpsensor_data_t *)handle;
    int irqf = 0;
    (void)irqf;

    //fpsensor_debug(ERR_LOG, "*************** irq %s %d\n", __func__, gpio_get_value(g_fpsensor->irq_gpio));

#if FPSENSOR_SLEEP_WAKEUP_HIGH_LEV
    irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_NO_SUSPEND;
    if (fpsensor_dev->suspend_flag == 1) {
        irq_set_irq_type(fpsensor_dev->irq, irqf);
        fpsensor_dev->suspend_flag = 0;
    }
#endif

    /* Make sure 'wakeup_enabled' is updated before using it
    ** since this is interrupt context (other thread...) */
    smp_rmb();
    wake_lock_timeout(&fpsensor_dev->ttw_wl, msecs_to_jiffies(1000));

#if FPSENSOR_IOCTL
    setRcvIRQ(1);
#endif
    wake_up_interruptible(&fpsensor_dev->wq_irq_return);
    fpsensor_dev->sig_count++;

    return IRQ_HANDLED;
}

void setRcvIRQ(int  val)
{
    fpsensor_data_t *fpsensor_dev = g_fpsensor;
    fpsensor_dev->RcvIRQ = val;
}

static long fpsensor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    fpsensor_data_t *fpsensor_dev = NULL;
    struct fpsensor_key fpsensor_key;
#if FPSENSOR_INPUT
    uint32_t key_event;
#endif
    int retval = 0;
    unsigned int val = 0;
    int irqf;
#if FPSENSOR_DUMMYTEE
    u8 reg_data[17] = {0};
    u8 reg_rx[16] = {0};
#endif
    fpsensor_debug(INFO_LOG, "[rickon]: fpsensor ioctl cmd : 0x%x \n", cmd);
    fpsensor_dev = (fpsensor_data_t *)filp->private_data;
    /*clear cancel flag*/
    fpsensor_dev->cancel = 0 ;
    switch (cmd) {
    case FPSENSOR_IOC_INIT:
        fpsensor_debug(INFO_LOG, "%s: fpsensor init started======\n", __func__);
        retval = fpsensor_get_gpio_dts_info();
        if (retval) {
            break;
        }
        fpsensor_irq_gpio_cfg();
        /*regist irq*/
        irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_NO_SUSPEND;
        //irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT;

        retval = request_threaded_irq(g_fpsensor->irq, fpsensor_irq, NULL, irqf,
                                      dev_name(&g_fpsensor->platform_device->dev), g_fpsensor);

        if (retval == 0) {
            fpsensor_debug(INFO_LOG, " irq thread reqquest success!\n");
        } else {
            fpsensor_debug(INFO_LOG, " irq thread request failed , retval =%d \n", retval);
        }
        enable_irq_wake(g_fpsensor->irq);
        fpsensor_dev->device_available = 1;
        fpsensor_dev->irq_count = 0;
        /*sunbo: add to avoid "unbalanced enable for IRQ 419" warning - begin*/
        fpsensor_dev->irq_count = 1;
        fpsensor_disable_irq(fpsensor_dev);
        /*sunbo: add to avoid "unbalanced enable for IRQ 419" warning - end*/

        fpsensor_dev->sig_count = 0;

        fpsensor_debug(INFO_LOG, "%s: fpsensor init finished======\n", __func__);
        break;

    case FPSENSOR_IOC_EXIT:
        fpsensor_disable_irq(fpsensor_dev);
        if (fpsensor_dev->irq) {
            free_irq(fpsensor_dev->irq, fpsensor_dev);
            fpsensor_dev->irq_count = 0;
        }
        fpsensor_dev->device_available = 0;
        fpsensor_gpio_free(g_fpsensor);
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
        //fpsensor_debug(INFO_LOG, "%s: ENABLE_SPI_CLK ======\n", __func__);
        fpsensor_spi_clk_enable(1);
        break;
    case FPSENSOR_IOC_DISABLE_SPI_CLK:
        //fpsensor_debug(INFO_LOG, "%s: DISABLE_SPI_CLK ======\n", __func__);
        fpsensor_spi_clk_enable(0);
        break;
    case FPSENSOR_IOC_ENABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_ENABLE_POWER ======\n", __func__);
        fpsensor_hw_power_enable(1);
        break;
    case FPSENSOR_IOC_DISABLE_POWER:
        fpsensor_debug(INFO_LOG, "%s: FPSENSOR_IOC_DISABLE_POWER ======\n", __func__);
        fpsensor_hw_power_enable(0);
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
        break;
#endif
    case FPSENSOR_IOC_ENTER_SLEEP_MODE:
        fpsensor_dev->is_sleep_mode = 1;
        break;
    case FPSENSOR_IOC_REMOVE:
#if FPSENSOR_USE_IOC_REMOVE
#if FPSENSOR_INPUT
        if (fpsensor_dev->input != NULL) {
            input_unregister_device(fpsensor_dev->input);
        }
#endif
        device_destroy(fpsensor_dev->class, fpsensor_dev->devno);
        unregister_chrdev_region(fpsensor_dev->devno, 1);
        class_destroy(fpsensor_dev->class);
#endif
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
    if (g_fpsensor->cancel == 1) {
        fpsensor_debug(ERR_LOG, " cancle\n");
        ret =  POLLERR;
        g_fpsensor->cancel = 0;
        return ret;
    }
    if (g_fpsensor->RcvIRQ) {
        fpsensor_debug(ERR_LOG, " ******get irq\n");
        ret |= POLLRDNORM;
    } else {
        ret = 0;
    }
    return ret;
}

/* -------------------------------------------------------------------- */
/* device function                                  */
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

/* -------------------------------------------------------------------- */
static int fpsensor_create_class(fpsensor_data_t *fpsensor)
{
    int error = 0;
    BUILD_BUG_ON(N_SPI_MINORS > 256);
    error = register_chrdev(FPSENSOR_MAJOR, FPSENSOR_DEV_NAME, &fpsensor_fops);
    fpsensor->class = class_create(THIS_MODULE, FPSENSOR_CLASS_NAME);
    if (IS_ERR(fpsensor->class)) {
        fpsensor_debug(ERR_LOG, "%s, Failed to create class.\n", __func__);
        error = PTR_ERR(fpsensor->class);
    }

    return error;
}

/* -------------------------------------------------------------------- */
static int fpsensor_create_device(fpsensor_data_t *fpsensor)
{
    int error = 0;
    unsigned long minor;
    minor = find_first_zero_bit(minors, N_SPI_MINORS);
    if (minor < N_SPI_MINORS) {
        struct device *dev;

        fpsensor->devno = MKDEV(FPSENSOR_MAJOR, minor);
        dev = device_create(fpsensor->class, &fpsensor->platform_device->dev,  fpsensor->devno,
                            fpsensor, FPSENSOR_DEV_NAME);
        error = IS_ERR(dev) ? PTR_ERR(dev) : 0;
    } else {
        fpsensor_debug(ERR_LOG, "no minor number available!\n");
        error = -ENODEV;
    }
    set_bit(minor, minors);
    return error;
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

/*-------------------------------------------------------------------------*/
#if defined(FPSENSOR_USE_SPI_BUS)
static int fpsensor_probe(struct spi_device *pdev)
#elif defined(FPSENSOR_USE_PLATFORM_BUS)
static int fpsensor_probe(struct platform_device *pdev)
#endif
{
    fpsensor_data_t *fpsensor_dev = NULL;
    int error = 0;
    int status = -EINVAL;

    FUNC_ENTRY();
    /* Allocate driver data */
    fpsensor_dev = kzalloc(sizeof(*fpsensor_dev), GFP_KERNEL);
    if (!fpsensor_dev) {
        fpsensor_debug(ERR_LOG, "%s, Failed to alloc memory for fpsensor device.\n", __func__);
        FUNC_EXIT();
        return -ENOMEM;
    }
#if defined(FPSENSOR_USE_PLATFORM_BUS)
    fpsensor_dev->platform_device = pdev ;
#endif

    g_fpsensor = fpsensor_dev;
    /* Initialize the driver data */
    mutex_init(&fpsensor_dev->buf_lock);

    fpsensor_dev->device_available = 0;
    fpsensor_dev->irq = 0;
    fpsensor_dev->probe_finish = 0;
    fpsensor_dev->device_count = 0;
    fpsensor_dev->users = 0;
    g_fpsensor->suspend_flag = 0;
#if FPSENSOR_DUMMYTEE
    fpsensor_dev->spi_freq_khz = 8000u;
    fpsensor_dev->fetch_image_cmd = 0x2c;
    fpsensor_dev->fetch_image_cmd_len = 2;
    spi_set_drvdata(pdev, fpsensor_dev);
    status = fpsensor_get_gpio_dts_info();
    if (status) {
        fpsensor_debug(ERR_LOG, "fpsensor get DTS info failed\n");
        goto err2;
    }
    fpsensor_spi_setup(fpsensor_dev);
    fpsensor_manage_image_buffer(fpsensor_dev, 160 * 160 * 2);
    fpsensor_hw_reset(1250);
    if (fpsensor_check_HWID(fpsensor_dev) == 0) {
        fpsensor_debug(ERR_LOG, "get chip id error .\n");
        goto err2;
    }
#endif
    /*setup fpsensor configurations.*/
    fpsensor_debug(INFO_LOG, "%s, Setting fpsensor device configuration.\n", __func__);
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

    fpsensor_dev->input = input_allocate_device();
    if (fpsensor_dev->input == NULL) {
        fpsensor_debug(ERR_LOG, "%s, Failed to allocate input device.\n", __func__);
        error = -ENOMEM;
        goto err2;
    }
#if FPSENSOR_INPUT
    __set_bit(EV_KEY, fpsensor_dev->input->evbit);
    __set_bit(FPSENSOR_INPUT_HOME_KEY, fpsensor_dev->input->keybit);

    __set_bit(FPSENSOR_INPUT_MENU_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_INPUT_BACK_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_INPUT_FF_KEY, fpsensor_dev->input->keybit);

    __set_bit(FPSENSOR_NAV_TAP_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_NAV_UP_KEY, fpsensor_dev->input->keybit);
    __set_bit(FPSENSOR_NAV_DOWN_KEY, fpsensor_dev->input->keybit);
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
    fpsensor_dev->irq_count = 0;
    fpsensor_dev->sig_count = 0;
    fpsensor_debug(INFO_LOG, "%s: fpsensor init finished======\n", __func__);

    fpsensor_dev->probe_finish = 1;
    fpsensor_dev->is_sleep_mode = 0;
    fpsensor_spi_clk_enable(1);

    init_waitqueue_head(&fpsensor_dev->wq_irq_return);
    wake_lock_init(&g_fpsensor->ttw_wl, WAKE_LOCK_SUSPEND, "fpsensor_ttw_wl");
    fpsensor_debug(INFO_LOG, "%s probe finished, normal driver version: %s\n", __func__,
                   FPSENSOR_SPI_VERSION);

    fpsensor_hw_power_enable(1);
    udelay(1000);
#if FP_NOTIFY
    fpsensor_dev->notifier.notifier_call = fpsensor_fb_notifier_callback;
    fb_register_client(&fpsensor_dev->notifier);
#endif
    FUNC_EXIT();
    return 0;

#if FPSENSOR_INPUT
err1:
    input_free_device(fpsensor_dev->input);
#endif
err2:
    device_destroy(fpsensor_dev->class, fpsensor_dev->devno);
    fpsensor_hw_power_enable(0);
    fpsensor_spi_clk_enable(0);
    clear_bit(MINOR(fpsensor_dev->devno), minors);

    kfree(fpsensor_dev);
    FUNC_EXIT();
    return status;
}

#if defined(FPSENSOR_USE_SPI_BUS)
static int fpsensor_remove(struct spi_device *pdev)
#elif defined(FPSENSOR_USE_PLATFORM_BUS)
static int fpsensor_remove(struct platform_device *pdev)
#endif
{
    fpsensor_data_t *fpsensor_dev = g_fpsensor;
    FUNC_ENTRY();

    /* make sure ops on existing fds can abort cleanly */
    if (fpsensor_dev->irq) {
        free_irq(fpsensor_dev->irq, fpsensor_dev);
    }
    device_destroy(fpsensor_dev->class, fpsensor_dev->devno);
    unregister_chrdev_region(fpsensor_dev->devno, 1);
    class_destroy(fpsensor_dev->class);
    if (fpsensor_dev->users == 0) {
#if FPSENSOR_INPUT
        if (fpsensor_dev->input != NULL) {
            input_unregister_device(fpsensor_dev->input);
        }
#endif
        if (fpsensor_dev->buffer != NULL) {
            kfree(fpsensor_dev->buffer);
        }
    }
#if FP_NOTIFY
    fb_unregister_client(&fpsensor_dev->notifier);
#endif
    clear_bit(MINOR(fpsensor_dev->devno), minors);
    fpsensor_hw_power_enable(0);
    wake_lock_destroy(&fpsensor_dev->ttw_wl);

    fpsensor_debug(INFO_LOG, "%s remove finished\n", __func__);
    kfree(fpsensor_dev);
    FUNC_EXIT();
    return 0;
}

#ifdef CONFIG_PM
#if defined(FPSENSOR_USE_PLATFORM_BUS)
static int fpsensor_suspend(struct platform_device *pdev, pm_message_t state)
#else
static int fpsensor_suspend(struct device *pdev, pm_message_t state)
#endif
{
    int irqf = 0;

    (void) irqf;
    fpsensor_debug(INFO_LOG, "%s\n", __func__);

#if FPSENSOR_SLEEP_WAKEUP_HIGH_LEV
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

#if defined(FPSENSOR_USE_PLATFORM_BUS)
static int fpsensor_resume(struct platform_device *pdev)
#else
static int fpsensor_resume(struct device *pdev)
#endif
{
    fpsensor_debug(INFO_LOG, "%s\n", __func__);
    return 0;
}
#endif

/*-------------------------------------------------------------------------*/
static struct of_device_id fpsensor_of_match[] = {
    { .compatible = "fpsensor,fingerprint", },
    {}
};
MODULE_DEVICE_TABLE(of, fpsensor_of_match);
struct spi_device_id fpsensor_spi_id_table = {FPSENSOR_DEV_NAME, 0};
#if defined(FPSENSOR_USE_SPI_BUS)
#if FPSENSOR_SPI_BUS_DYNAMIC
static struct spi_board_info spi_board_devs[] __initdata = {
    [0] = {
        .modalias = FPSENSOR_DEV_NAME,
        .bus_num        = 0,
        .chip_select = 0,
        .max_speed_hz       = 6000000,
        .mode = SPI_MODE_0,
    }
};
#endif
static struct spi_driver fpsensor_spi_driver = {
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        .bus = &spi_bus_type,
        .owner = THIS_MODULE,
        .of_match_table = fpsensor_of_match,
#ifdef CONFIG_PM
        .suspend = fpsensor_suspend,
        .resume = fpsensor_resume,
#endif
    },
    .id_table = &fpsensor_spi_id_table,
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
};
#elif defined(FPSENSOR_USE_PLATFORM_BUS)
static struct platform_driver fpsensor_platform_driver = {
    .driver = {
        .name = FPSENSOR_DEV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = fpsensor_of_match,
    },
#ifdef CONFIG_PM
    .suspend = fpsensor_suspend,
    .resume = fpsensor_resume,
#endif
    .probe = fpsensor_probe,
    .remove = fpsensor_remove,
};
#endif
static int __init fpsensor_init(void)
{
    int status;

    FUNC_ENTRY();
    fpsensor_debug(INFO_LOG, "%s\n", __func__);

#if defined(FPSENSOR_USE_PLATFORM_BUS)
    status = platform_driver_register(&fpsensor_platform_driver);
#elif defined(FPSENSOR_USE_SPI_BUS)
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
#if defined(FPSENSOR_USE_PLATFORM_BUS)
    platform_driver_unregister(&fpsensor_platform_driver);
#elif defined(FPSENSOR_USE_SPI_BUS)
    spi_unregister_driver(&fpsensor_spi_driver);
#endif
    FUNC_EXIT();
}
module_exit(fpsensor_exit);

MODULE_AUTHOR("xhli");
MODULE_DESCRIPTION(" Fingerprint fpsensor TEE driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:fpsensor-drivers");

