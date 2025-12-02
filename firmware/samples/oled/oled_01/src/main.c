// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/display.h>
// #include <zephyr/display/cfb.h>
// #include <zephyr/sys/printk.h>

// static const struct device *dev_dis = DEVICE_DT_GET(DT_NODELABEL(ssd1306));

// int main(void)
// {
// 	int ret;
// 	if (dev_dis == NULL)
// 	{
// 		printk("device pointer is NULL");
// 		return;
// 	}

// 	if (!device_is_ready(dev_dis))
// 	{
// 		printk("display device is not ready");
// 		return;
// 	}

// 	if(cfb_framebuffer_init(dev_dis))
// 	{
// 		printk("cfb_framebuffer_init failed\n");
// 		return;
// 	}

// 	printk("Display initialized\n");

// 	// struct display_capabilities capabilities;
// 	// display_get_capabilities(dev_dis, &capabilities);

// 	// const uint16_t x_res = capabilities.x_resolution;
// 	// const uint16_t y_res = capabilities.y_resolution;

// 	// printk("x_resolution: %d\n", x_res);
// 	// printk("y_resolution: %d\n", y_res);
// 	// printk("supported pixel formats: %d\n", capabilities.supported_pixel_formats);
// 	// printk("screen_info: %d\n", capabilities.screen_info);
// 	// printk("current_pixel_format: %d\n", capabilities.current_pixel_format);
// 	// printk("current_orientation: %d\n", capabilities.current_orientation);

// 	// cfb_framebuffer_clear(dev_dis, true);
// 	// cfb_framebuffer_invert(dev_dis);

// 	ret = cfb_print(dev_dis, "Hello World", 0, 0); // At position (0,0)
// 	cfb_framebuffer_finalize(dev_dis);
// 	return 0;
// }


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

/* Define the device label that matches your device tree overlay (e.g., "SSD1306") */
static const struct device *display_dev = DEVICE_DT_GET(DT_NODELABEL(ssd1306));

void main(void) {
    if (!device_is_ready(display_dev)) {
        printk("Display device not ready.\n");
        return;
    }

    printk("Display device found. Initializing LVGL.\n");

    lv_obj_t *hello_label = lv_label_create(lv_scr_act()); // Create a label on the active screen
    lv_label_set_text(hello_label, "hello");               // Set the text content
    lv_obj_align(hello_label, LV_ALIGN_CENTER, 0, 0);     // Center the text on the screen

    /* Main application loop */
    while (1) {
        lv_task_handler();      // Required to update LVGL logic
        display_blanking_off(display_dev); // Ensure display is not blanked (on)
        k_sleep(K_MSEC(100));   // Sleep for a short period
    }
}
