/* include files */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#define MAX_I2C_ADDRESS 127

struct device *dev_appi2c0;
struct device *dev_appi2c1;

#define I2C_0_NODE  DT_ALIAS(appi2c0)
#define I2C_1_NODE  DT_ALIAS(appi2c1)
	
void scan_i2c_devices(const struct device *dev);

int main(void)
{
    k_sleep(K_MSEC(1000));

    dev_appi2c0 = DEVICE_DT_GET(I2C_0_NODE);

    if (!dev_appi2c0)
    {
        printk("I2C_0 device not found\n");
        return -1;
    }
    else
    {
        printk("I2C_0 device found\n");
    }

	dev_appi2c1 = DEVICE_DT_GET(I2C_1_NODE);

    if (!dev_appi2c1)
    {
        printk("I2C_1 device not found\n");
        return -1;
    }
    else
    {
        printk("I2C_1 device found\n");
    }

    while(true)
    {
        scan_i2c_devices(dev_appi2c0);
		scan_i2c_devices(dev_appi2c1);
        k_sleep(K_MSEC(5000));
    }
    
    return 0;
}

void scan_i2c_devices(const struct device *dev)
{
    uint8_t data;
	uint8_t i2c_addr;
	uint8_t status;
	uint8_t number_of_bytes;

	struct i2c_msg i2cmsg;

	i2cmsg.buf = &data;
	i2cmsg.len = 1;
	i2cmsg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	number_of_bytes = 1;

	printk("started scanning i2c devices port %s\n\n", dev->name);
	printk("    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n00:    ");
	for(i2c_addr=1; i2c_addr<= MAX_I2C_ADDRESS; i2c_addr++)
	{
		if((i2c_addr % 0x10) == 0)	
		{
			printk("\n%2x: ",i2c_addr);
		}

		status = i2c_transfer(dev, &i2cmsg, number_of_bytes, i2c_addr);

		if(status == 0)
		{
			printk("%2x ",i2c_addr);
		}
		else
		{
			printk("-- ");
		}

		k_sleep(K_MSEC(100));	
	}
	printk("\n");
}