#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/printk.h>

static const struct device *dev_dis = NULL;

int main(void)
{
	int ret;
	while(1)
	{
		k_msleep(5000);
		printk("oled display");
	}
	
	dev_dis = DEVICE_DT_GET(DT_NODELABEL(ssd1306_1));
	if (dev_dis == NULL)
	{
        while(1)
        {
            printk("device pointer is NULL");
            k_msleep(1000);
        }
		return;
	}

	if (!device_is_ready(dev_dis))
	{
        while(1)
        {
            printk("display device is not ready");
            k_msleep(1000);
        }
		
		return;
	}

	if(cfb_framebuffer_init(dev_dis))
	{
		while(1)
		{
			printk("cfb_framebuffer_init failed\n");
			k_msleep(1000);
		}
		return;
	}

	printk("Display initialized\n");

	struct display_capabilities capabilities;
	display_get_capabilities(dev_dis, &capabilities);

	const uint16_t x_res = capabilities.x_resolution;
	const uint16_t y_res = capabilities.y_resolution;

	printk("x_resolution: %d\n", x_res);
	printk("y_resolution: %d\n", y_res);
	printk("supported pixel formats: %d\n", capabilities.supported_pixel_formats);
	printk("screen_info: %d\n", capabilities.screen_info);
	printk("current_pixel_format: %d\n", capabilities.current_pixel_format);
	printk("current_orientation: %d\n", capabilities.current_orientation);

	// // cfb_framebuffer_clear(dev_dis, true);
	// // cfb_framebuffer_invert(dev_dis);

	// ret = cfb_print(dev_dis, "Hello World", 0, 0); // At position (0,0)
	// cfb_framebuffer_finalize(dev_dis);
	return 0;
}

