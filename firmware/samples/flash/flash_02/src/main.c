#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>


#define DATA_OFFSET      0

struct flash_data
{
	uint8_t id;
	uint8_t val8;
	uint16_t val16;
	uint32_t val32;
	uint8_t array[4];
};

const struct flash_area *fa;
const struct device *flash_dev;
struct flash_pages_info info;

struct flash_data data;

int main(void)
{
	int ret;
	size_t erase_sz;

	k_sleep(K_MSEC(5000));
	
	printk("get flash area");
	ret = flash_area_open(FIXED_PARTITION_ID(ext_nvs), &fa);
    if (ret)
	{
        printk("flash_area_open failed: %d\n", ret);
        return 0;
    }

	printk("get flash device\n");
    flash_dev = flash_area_get_device(fa);
    if (!device_is_ready(flash_dev))
	{
        printk("Flash device not ready\n");
        flash_area_close(fa);
        return 0;
    }

    printk("Config partition: off=0x%lx size=0x%lx\n", (unsigned long)fa->fa_off, (unsigned long)fa->fa_size);

    printk("get flash area info\n");
    ret = flash_get_page_info_by_offs(flash_dev, fa->fa_off, &info);
    if (ret)
	{
        printk("flash_get_page_info_by_offs failed: %d\n", ret);
        flash_area_close(fa);
        return 0;
    }

    erase_sz = info.size;
    if (erase_sz > fa->fa_size)
	{
        printk("Erase size bigger than partition?!\n");
        flash_area_close(fa);
        return 0;
    }

	printk(":flash test:\n");
	printk("step1: read flash data and print\n");
	printk("step2: erase flash \n");
	printk("step3: increament flash data \n");
	printk("step4: write flash data\n");
	printk("step5: read flash data and print\n");

	printk("step1: read flash data\n");
	ret = flash_area_read(fa, DATA_OFFSET, &data, sizeof(data));
    if (ret)
	{
        printk("flash_area_read failed: %d\n", ret);
        flash_area_close(fa);
        return 0;
    }

	printk("flash data \n");
	printk("val8:%d\n", data.val8);
	printk("val16:%d\n", data.val16);
	printk("val32:%d\n", data.val32);
	printk("array:%d %d %d %d\n\n\n", data.array[0],data.array[1],data.array[2],data.array[3]);

	printk("step2: erase flash \n");
	ret = flash_area_erase(fa, 0, erase_sz);
    if (ret)
	{
        printk("flash_area_erase failed: %d\n", ret);
        flash_area_close(fa);
        return 0;
    }
    printk("Erased %u bytes at config+0x0\n", (unsigned)erase_sz);

	printk("step3: increament flash data \n");
	if(data.id == 0x07)
	{
		data.val8++;
		data.val16++;
		data.val32++;
		data.array[0]++;
		data.array[1]++;
		data.array[2]++;
		data.array[3]++;
	}
	else
	{
		data.id = 7;
		data.val8 = 0;
		data.val16= 0;
		data.val32= 0;
		data.array[0]= 0;
		data.array[1]= 0;
		data.array[2]= 0;
		data.array[3]= 0;
	}

	printk("step4: write flash data\n");
	ret = flash_area_write(fa, DATA_OFFSET, &data, sizeof(data));
    if (ret)
	{
        printk("flash_area_write failed: %d\n", ret);
        flash_area_close(fa);
        return 0;
    }
    printk("Wrote %u bytes at config+0x%x\n", (unsigned)sizeof(data), DATA_OFFSET);

	printk("step5: read flash data and print\n");
	ret = flash_area_read(fa, DATA_OFFSET, &data, sizeof(data));
    if (ret)
	{
        printk("flash_area_read failed: %d\n", ret);
        flash_area_close(fa);
        return 0;
    }

	printk("flash data \n");
	printk("val8:%d\n", data.val8);
	printk("val16:%d\n", data.val16);
	printk("val32:%d\n", data.val32);
	printk("array:%d %d %d %d\n\n\n", data.array[0],data.array[1],data.array[2],data.array[3]);

	flash_area_close(fa);

    return 0;
}