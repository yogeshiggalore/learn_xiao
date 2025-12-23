#ifndef W25QXXH_
#define W25QXXH_

#include <stdint.h> // For standard integer types
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif


enum W25QXX_CMD
{
    W25QXX_CMD_NONE=0,
    W25QXX_CMD_PAGE_PROGRAM=0x02,
    W25QXX_CMD_READ_DATA=0x03,
    W25QXX_CMD_WRITE_DISABLE=0x04,
    W25QXX_CMD_WRITE_ENABLE=0x6,
    W25QXX_CMD_FAST_READ=0x0B,
    W25QXX_CMD_SR_WRITE_ENABLE=0x50,
    W25QXX_CMD_RELEASE_PWR_DOWN=0xAB,
    W25QXX_CMD_MNUF_DEV_ID=0x90,
    W25QXX_CMD_JEDEC_ID=0x9F,
    W25QXX_CMD_UNIQUE_ID=0x4B,
    W25QXX_CMD_ERASE_SECTOR=0x20,
    W25QXX_CMD_ERASE_BLOCK32=0x52,
    W25QXX_CMD_ERASE_BLOCK64=0xD8,
    W25QXX_CMD_CHIP_ERASE1=0xC7,
    W25QXX_CMD_CHIP_ERASE2=0x60,
    W25QXX_CMD_READ_STATUS_REG1=0x05,
    W25QXX_CMD_WRITE_STATUS_REG1=0x01,
    W25QXX_CMD_READ_STATUS_REG2=0x35,
    W25QXX_CMD_WRITE_STATUS_REG2=0x31,
    W25QXX_CMD_READ_STATUS_REG3=0x15,
    W25QXX_CMD_WRITE_STATUS_REG3=0x11,
    W25QXX_CMD_READ_SFDP_REG=0x5A,
    W25QXX_CMD_ERASE_SECURITY_REG=0x44,
    W25QXX_CMD_PROGRAM_SECURITY_REG=0x42,
    W25QXX_CMD_READ_SECURITY_REG=0x48,
    W25QXX_CMD_GLOBAL_BLOCK_LOCK=0x7E,
    W25QXX_CMD_GLOBAL_BLOCK_UNLOCK=0x98,
    W25QXX_CMD_READ_BLOCK_LOCK=0x3D,
    W25QXX_CMD_IND_BLOCK_LOCK=0x36,
    W25QXX_CMD_IND_BLOCK_UNLOCK=0x39,
    W25QXX_CMD_PROGRAM_SUSPEND=0x75,
    W25QXX_CMD_PROGRAM_RESUME=0x7A,
    W25QXX_CMD_POWER_DOWN=0xB9,
    W25QXX_CMD_ENABLE_RESET=0x66,
    W25QXX_CMD_RESET_DEVICE=0x99,
};

typedef union
{
	uint8_t val;
	struct
	{
		uint8_t busy : 1; //write in progress
		uint8_t wel: 1; // write enable latch
		uint8_t bp0 : 1; // block protect bit
		uint8_t bp1 : 1; // block protect bit
		uint8_t bp2 : 1; // block protect bit
		uint8_t tb : 1; // top/bottom protect
		uint8_t sec : 1; // sector protect
		uint8_t srp : 1; //status register protect
	} bits;
} MAX20357_STATUS1_t;

typedef union
{
	uint8_t val;
	struct
	{
		uint8_t srl : 1; //status register lock
		uint8_t qe: 1; // quad enable
		uint8_t r : 1; // reserved
		uint8_t lb1 : 1; // security register lock bits1
		uint8_t lb2 : 1; // security register lock bits2
		uint8_t lb3 : 1; // security register lock bits3
		uint8_t cmp : 1; // compliment protect
		uint8_t sus : 1; // suspend status
	} bits;
} MAX20357_STATUS2_t;

typedef union
{
	uint8_t val;
	struct
	{
		uint8_t r1 : 1; // reserved
		uint8_t r2: 1; // reserved
		uint8_t wps : 1; // write protect selection
        uint8_t r3 : 1; // reserved
		uint8_t r4 : 1; // reserved
		uint8_t drv2 : 1; // output drive strength2
		uint8_t drv1 : 1; // output drive strength2
		uint8_t r5 : 1; // reserved
	} bits;
} MAX20357_STATUS3_t;

struct w25qxx_reg_list
{
    MAX20357_STATUS1_t sts1;
    MAX20357_STATUS2_t sts2;
    MAX20357_STATUS3_t sts3;
    uint8_t manf_id[3];
    uint8_t jedec_id[3];
    uint8_t unique_id[8];
    uint8_t sfdp[256];
};

struct w25qxx_data
{
    struct w25qxx_reg_list rd_reg;
};

struct w25qxx_config
{
    struct spi_dt_spec *dev;
    uint8_t ready;
};

struct w25qxx_device
{
    struct w25qxx_config cfg;
    struct w25qxx_data data;
};

void w25qxx_init(struct w25qxx_device *dev);
void w25qxx_spi_int(struct w25qxx_device *dev);
void w25qxx_read_all_reg(struct w25qxx_device *dev);
void w25qxx_print_read_reg(struct w25qxx_device *dev);

void w25qxx_read_all_status_reg(struct w25qxx_device *dev);
void w25qxx_read_status_reg(struct w25qxx_device *dev, uint8_t addr);

void w25qxx_read_manufr_id(struct w25qxx_device *dev);
void w25qxx_read_jedec_id(struct w25qxx_device *dev);
void w25qxx_read_unique_id(struct w25qxx_device *dev);
void w25qxx_read_sfdp_reg(struct w25qxx_device *dev);
void w25qxx_read_security_reg(struct w25qxx_device *dev);

#ifdef __cplusplus
}
#endif

#endif // W25QXXH_