#include <linux/spi/spi.h>

#include "fpsensor_platform.h"
#include "fpsensor_spi.h"

#define FPSENSOR_REG_MAX_SIZE (64)

/* -------------------------------------------------------------------- */
int fpsensor_spi_setup(fpsensor_data_t *fpsensor)
{
    int error = 0;

    FUNC_ENTRY();
    fpsensor->spi->mode = SPI_MODE_0;
    fpsensor->spi->bits_per_word = 8;
//    fpsensor->spi->chip_select = 0;

    spi_setup(fpsensor->spi);
    if (error) {
        fpsensor_debug(ERR_LOG, "spi_setup failed\n");
        goto out_err;
    }
out_err:
    FUNC_EXIT() ;
    return error;
}

int fpsensor_spi_send_recv(fpsensor_data_t *fpsensor, size_t len , u8 *tx_buffer, u8 *rx_buffer)
{
    struct spi_message msg;
    struct spi_transfer cmd = {
        .cs_change = 0,
        .delay_usecs = 0,
        .speed_hz = (u32)fpsensor->spi_freq_khz * 1000u,
        .tx_buf = tx_buffer,
        .rx_buf = rx_buffer,
        .len    = len,
        .tx_dma = 0,
        .rx_dma = 0,
        .bits_per_word = 8,
    };
    int error = 0 ;
    FUNC_ENTRY();

    spi_message_init(&msg);
    spi_message_add_tail(&cmd,  &msg);
    spi_sync(fpsensor->spi, &msg);
    if (error) {
        fpsensor_debug(ERR_LOG, "spi_sync failed.\n");
    }
    fpsensor_debug(DEBUG_LOG, "tx_len : %d \n", (int)len);
    fpsensor_debug(DEBUG_LOG, "tx_buf : %x  %x  %x  %x  %x  %x\n",
                   (len > 0) ? tx_buffer[0] : 0,
                   (len > 1) ? tx_buffer[1] : 0,
                   (len > 2) ? tx_buffer[2] : 0,
                   (len > 3) ? tx_buffer[3] : 0,
                   (len > 4) ? tx_buffer[4] : 0,
                   (len > 5) ? tx_buffer[5] : 0);
    fpsensor_debug(DEBUG_LOG, "rx_buf : %x  %x  %x  %x  %x  %x\n",
                   (len > 0) ? rx_buffer[0] : 0,
                   (len > 1) ? rx_buffer[1] : 0,
                   (len > 2) ? rx_buffer[2] : 0,
                   (len > 3) ? rx_buffer[3] : 0,
                   (len > 4) ? rx_buffer[4] : 0,
                   (len > 5) ? rx_buffer[5] : 0);

    FUNC_EXIT() ;
    return error;
}


int fpsensor_spi_dma_fetch_image(fpsensor_data_t *fpsensor , size_t image_size_bytes)
{

//-------------------------------------
    int error = 0;
    struct spi_message msg;
    unsigned char *buffer = fpsensor->huge_buffer;
    static struct spi_transfer fpsensor_xfer[2];
    static u8 rx_buf0[FPSENSOR_REG_MAX_SIZE] = {0};
    static u8 tx_buf0[FPSENSOR_REG_MAX_SIZE] = {0};
    memset(rx_buf0, 0x00, FPSENSOR_REG_MAX_SIZE);
    memset(tx_buf0, 0x00, FPSENSOR_REG_MAX_SIZE);
    spi_message_init(&msg);
//--------send fetch bus addr
    tx_buf0[0] = fpsensor->fetch_image_cmd;
    tx_buf0[1] = 0;
    fpsensor_xfer[0].tx_buf = tx_buf0;
    fpsensor_xfer[0].rx_buf = rx_buf0;
    fpsensor_xfer[0].len = fpsensor->fetch_image_cmd_len;
    fpsensor_xfer[0].cs_change = 0;
    spi_message_add_tail(&fpsensor_xfer[0], &msg);
//-----------fetch image
    fpsensor_xfer[1].rx_buf = &buffer[0];
    fpsensor_xfer[1].tx_buf = &buffer[1600];
    fpsensor_xfer[1].len = image_size_bytes;
    spi_message_add_tail(&fpsensor_xfer[0], &msg);
    error =  spi_sync(fpsensor->spi, &msg);
    if (error) {
        fpsensor_debug(ERR_LOG, "spi_sync failed.\n");
    }
    return error;
}

int fpsensor_check_HWID(fpsensor_data_t *fpsensor)
{
    unsigned int hwid = 0;
    unsigned char  tx[3];
    unsigned char  rx[3];
    int match = 0;
    tx[0] = 0x08;
    tx[1] = 0x55;
    fpsensor_spi_send_recv(fpsensor, 2, tx, rx);

    tx[0] = 0x00;
    rx[1] = 0x00;
    rx[2] = 0x00;
    fpsensor_spi_send_recv(fpsensor, 3, tx, rx);
    hwid = ((rx[1] << 8) | (rx[2]));
    fpsensor_debug(ERR_LOG,"HWID 0x%x .\n",hwid);
    if ((hwid == 0x7153) || (hwid == 0x7230) ||(hwid == 0x7222)) {
        match = 1;
    }
    return match;
}

int fpsensor_manage_image_buffer(fpsensor_data_t *fpsensor , size_t new_size)
{
    int error = 0;
    int buffer_order_new, buffer_order_curr;

    buffer_order_curr = get_order(fpsensor->huge_buffer_size);
    buffer_order_new  = get_order(new_size);

    if (new_size == 0) {
        if (fpsensor->huge_buffer) {
            free_pages((unsigned long)fpsensor->huge_buffer,
                       buffer_order_curr);

            fpsensor->huge_buffer = NULL;
        }
        fpsensor->huge_buffer_size = 0;
        error = 0;

    } else {
        if (fpsensor->huge_buffer &&
            (buffer_order_curr != buffer_order_new)) {

            free_pages((unsigned long)fpsensor->huge_buffer,
                       buffer_order_curr);

            fpsensor->huge_buffer = NULL;
        }

        if (fpsensor->huge_buffer == NULL) {
            fpsensor->huge_buffer =
                (u8 *)__get_free_pages(GFP_KERNEL,
                                       buffer_order_new);

            fpsensor->huge_buffer_size = (fpsensor->huge_buffer) ?
                                         (size_t)PAGE_SIZE << buffer_order_new : 0;

            error = (fpsensor->huge_buffer_size == 0) ? -ENOMEM : 0;
        }
    }

    if (error) {
        fpsensor_debug(ERR_LOG, "%s, failed %d\n",
                       __func__, error);
    } else {
        fpsensor_debug(DEBUG_LOG, "%s, size=%d bytes\n",
                       __func__, (int)fpsensor->huge_buffer_size);
    }

    return error;
}

