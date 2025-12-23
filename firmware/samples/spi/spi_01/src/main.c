#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#define EFLASH_NODE DT_ALIAS(appflash)

/* 1. Define Operation Flags here */
#define SPI_OP (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

/* 2. Create the Spec 
 * SPI_DT_SPEC_GET requires 3 args: (NodeID, Operation, Delay)
 * This macro automatically pulls the Chip Select (CS) info from Device Tree
 */
static const struct spi_dt_spec eflash_dev = SPI_DT_SPEC_GET(EFLASH_NODE, SPI_OP, 0);

struct jedec_id {
    uint8_t manuf_id;
    uint8_t mem_type;
    uint8_t capacity;
};

struct jedec_id id;
uint8_t uniqueid[8];

void read_jedec_id(const struct spi_dt_spec *spec, struct jedec_id *id);
void read_unique_id(const struct spi_dt_spec *spec, uint8_t *id, uint8_t len);

int main(void)
{
    printk("SPI_01 demo\n");

    /* 4. Use the specific helper for DT specs */
    if (!spi_is_ready_dt(&eflash_dev)) {
        printk("SPI device not ready!\n");
        return 0;
    }

    /* Note: No need to manually configure GPIO CS here. 
       spi_transceive_dt handles pin configuration automatically. */

    while(true) {
        read_jedec_id(&eflash_dev, &id);
		read_unique_id(&eflash_dev, &(uniqueid[0]), sizeof(uniqueid));
        k_sleep(K_SECONDS(5));
    }
    return 0;
}

void read_jedec_id(const struct spi_dt_spec *spec, struct jedec_id *id)
{
    int err;
    uint8_t tx_buffer[4] = { 0x9F, 0x00, 0x00, 0x00 };
    uint8_t rx_buffer[4] = { 0 };

    struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    
    /* 5. Pass the spec pointer correctly */
    err = spi_transceive_dt(spec, &tx, &rx);
    
    if(err == 0)
	{
        /* Byte 0 is garbage during the command send */
        id->manuf_id = rx_buffer[1];
        id->mem_type = rx_buffer[2];
        id->capacity = rx_buffer[3];
    }

	printk("manf id: %02X mem_type: %02X capacity: %02X\n", id->manuf_id, id->mem_type, id->capacity);
}

void read_unique_id(const struct spi_dt_spec *spec, uint8_t *id, uint8_t len)
{
	int err;
    uint8_t tx_buffer[4] = { 0x4B, 0x00, 0x00, 0x00, 0x00};
    uint8_t rx_buffer[12] = { 0 };

    struct spi_buf tx_buf = { .buf = tx_buffer, .len = sizeof(tx_buffer) };
    struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = rx_buffer, .len = sizeof(rx_buffer) };
    struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };
    
    /* 5. Pass the spec pointer correctly */
    err = spi_transceive_dt(spec, &tx, &rx);
    
    if(err == 0)
	{
		int j=0;
        for(int i=(sizeof(rx_buffer)-1);i>3;i--)
		{
			id[j] = rx_buffer[i];
			j++;
		}
    }

	printk("unique id: ");
	for(int j=0;j<len;j++)
	{
		printk("%2X ", id[j]);
	}
	printk("\n");
}