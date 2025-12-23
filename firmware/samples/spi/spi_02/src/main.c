#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "w25qxx.h"

#define EFLASH_NODE DT_ALIAS(appflash)
#define SPI_OP (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

struct w25qxx_device w25q;

static struct spi_dt_spec eflash_dev = SPI_DT_SPEC_GET(EFLASH_NODE, SPI_OP, 0);

int main(void)
{
    w25q.cfg.dev = &eflash_dev;
    w25qxx_init(&w25q);

    while(true)
    {
        w25qxx_read_all_reg(&w25q);
        k_msleep(5000);
    }
	return 0;
}
