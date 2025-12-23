#include <zephyr/kernel.h>
#include "w25qxx.h"
#include "ym_print.h"
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

static struct w25qxx_device *ptr_dev;


void w25qxx_init(struct w25qxx_device *dev)
{
    ptr_dev = dev;
    w25qxx_spi_int(dev);
}

void w25qxx_spi_int(struct w25qxx_device *dev)
{
    if (!spi_is_ready_dt(dev->cfg.dev))
    {
        dev->cfg.ready = false;
        dev->cfg.dev = NULL;
    }
    else
    {
        dev->cfg.ready = true;
    }
    printk("[%s] ready:%d\n", __func__, dev->cfg.ready);
}

void w25qxx_read_all_reg(struct w25qxx_device *dev)
{
    w25qxx_read_all_status_reg(dev);
    w25qxx_read_manufr_id(dev);
    w25qxx_read_jedec_id(dev);
    w25qxx_read_unique_id(dev);
    w25qxx_read_sfdp_reg(dev);
    w25qxx_read_security_reg(dev);
    w25qxx_print_read_reg(dev);   
}

void w25qxx_read_all_status_reg(struct w25qxx_device *dev)
{
    w25qxx_read_status_reg(dev, W25QXX_CMD_READ_STATUS_REG1);
    w25qxx_read_status_reg(dev, W25QXX_CMD_READ_STATUS_REG2);
    w25qxx_read_status_reg(dev, W25QXX_CMD_READ_STATUS_REG3);
}

void w25qxx_read_status_reg(struct w25qxx_device *dev, uint8_t addr)
{
    int err;
    uint8_t tx_buffer[1] = { 0x9F};
    uint8_t rx_buffer[1] = { 0 };

    tx_buffer[0] = addr;

    struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    
    /* 5. Pass the spec pointer correctly */
    err = spi_transceive_dt(dev->cfg.dev, &tx, &rx);
    
    if(err == 0)
	{
        if(addr == W25QXX_CMD_READ_STATUS_REG1)
        {
            dev->data.rd_reg.sts1.val = rx_buffer[0];
        } 

        if(addr == W25QXX_CMD_READ_STATUS_REG2)
        {
            dev->data.rd_reg.sts2.val = rx_buffer[0];
        }

        if(addr == W25QXX_CMD_READ_STATUS_REG3)
        {
            dev->data.rd_reg.sts3.val = rx_buffer[0];
        }
    }
    else
    {
        printk("[%s] err:%d\n", __func__, err);
    }
}

void w25qxx_print_read_reg(struct w25qxx_device *dev)
{
    printk("STATUS1 = %02X " YM_PRINT_BYTE_TO_BINARY_PATTERN "\n", dev->data.rd_reg.sts1.val, YM_PRINT_BYTE_TO_BINARY(dev->data.rd_reg.sts1.val));
    printk("STATUS2 = %02X " YM_PRINT_BYTE_TO_BINARY_PATTERN "\n", dev->data.rd_reg.sts2.val, YM_PRINT_BYTE_TO_BINARY(dev->data.rd_reg.sts2.val));
    printk("STATUS3 = %02X " YM_PRINT_BYTE_TO_BINARY_PATTERN "\n", dev->data.rd_reg.sts3.val, YM_PRINT_BYTE_TO_BINARY(dev->data.rd_reg.sts3.val));
    printk("MANFRID = %02X %02X %02X \n", dev->data.rd_reg.manf_id[0],dev->data.rd_reg.manf_id[1],dev->data.rd_reg.manf_id[2]);
    printk("JEDECID = %02X %02X %02X \n", dev->data.rd_reg.jedec_id[0],dev->data.rd_reg.jedec_id[1],dev->data.rd_reg.jedec_id[2]);
    printk("UNIQUEID = %02X %02X %02X %02X %02X %02X %02X %02X\n", dev->data.rd_reg.unique_id[0],dev->data.rd_reg.unique_id[1],dev->data.rd_reg.unique_id[2], \
                                                                    dev->data.rd_reg.unique_id[3], dev->data.rd_reg.unique_id[4], dev->data.rd_reg.unique_id[5], \
                                                                    dev->data.rd_reg.unique_id[6], dev->data.rd_reg.unique_id[7]);

    
    printk("SFDP = ");
    for(int i=0; i<256;)
    {
        for(int j=0; j<16; j++)
        {
            printk("%02X ", dev->data.rd_reg.sfdp[i]);
            i++;
        }
        printk("\n       ");
    }
    printk("\n");
}

void w25qxx_read_manufr_id(struct w25qxx_device *dev)
{
    /* 0x90, (3 bytes of address) 2 dummy */
    int err;
    uint8_t tx_buffer[6] = { 0x90,0,0,0,0, 0 };
    uint8_t rx_buffer[6] = { 0 };

    struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    
    /* 5. Pass the spec pointer correctly */
    err = spi_transceive_dt(dev->cfg.dev, &tx, &rx);
    
    if(err == 0)
	{
        dev->data.rd_reg.manf_id[0] = rx_buffer[3];
        dev->data.rd_reg.manf_id[1] = rx_buffer[4];
        dev->data.rd_reg.manf_id[2] = rx_buffer[5];
    }
    else
    {
        printk("[%s] err:%d\n", __func__, err);
    }
}

void w25qxx_read_jedec_id(struct w25qxx_device *dev)
{
    int err;
    uint8_t tx_buffer[4] = { 0x9F, 0x00, 0x00, 0x00 };
    uint8_t rx_buffer[4] = { 0 };

    struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    
    /* 5. Pass the spec pointer correctly */
    err = spi_transceive_dt(dev->cfg.dev, &tx, &rx);
    
    if(err == 0)
	{
        /* Byte 0 is garbage during the command send */
        dev->data.rd_reg.jedec_id[0] = rx_buffer[1];
        dev->data.rd_reg.jedec_id[1] = rx_buffer[2];
        dev->data.rd_reg.jedec_id[2] = rx_buffer[3];
    }
}

void w25qxx_read_unique_id(struct w25qxx_device *dev)
{
    /* 0x4B, (3 bytes of address) 2 dummy */
    int err;
    uint8_t tx_buffer[5] = {0};
    uint8_t rx_buffer[13] = { 0 };

    tx_buffer[0] = 0x4B;

    struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    
    /* 5. Pass the spec pointer correctly */
    err = spi_transceive_dt(dev->cfg.dev, &tx, &rx);
    
    if(err == 0)
	{
        dev->data.rd_reg.unique_id[0] = rx_buffer[5];
        dev->data.rd_reg.unique_id[1] = rx_buffer[6];
        dev->data.rd_reg.unique_id[2] = rx_buffer[7];
        dev->data.rd_reg.unique_id[3] = rx_buffer[8];
        dev->data.rd_reg.unique_id[4] = rx_buffer[9];
        dev->data.rd_reg.unique_id[5] = rx_buffer[10];
        dev->data.rd_reg.unique_id[6] = rx_buffer[11];
        dev->data.rd_reg.unique_id[7] = rx_buffer[12];
    }
    else
    {
        printk("[%s] err:%d\n", __func__, err);
    }
}

void w25qxx_read_sfdp_reg(struct w25qxx_device *dev)
{
    /* 0x5A 3 byte address 8 bytes dummy */
     int err;
    uint8_t tx_buffer[12] = {0};
    uint8_t rx_buffer[256] = { 0 };

    tx_buffer[0] = 0x5A;

    struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    
    /* 5. Pass the spec pointer correctly */
    err = spi_transceive_dt(dev->cfg.dev, &tx, &rx);
    
    if(err == 0)
	{
        memcpy(dev->data.rd_reg.sfdp, rx_buffer, 256);
    }
    else
    {
        printk("[%s] err:%d\n", __func__, err);
    }
}

void w25qxx_read_security_reg(struct w25qxx_device *dev)
{

}