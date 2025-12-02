#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* the device tree indentifier for the led0 node */
#define LED0_NODE DT_ALIAS(appled0)

/* define sw0 node */
#define SW0_NODE DT_ALIAS(appsw0)

/* get devicetree spec for led0 node */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* get dt spec for sw0 */
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

/* gpio callback */
static struct gpio_callback sw0_cb_data;

struct k_work_delayable led_work;

void sw0_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{		
	k_work_schedule(&led_work, K_MSEC(100));
}

void led_work_handler(struct k_work *work)
{
	gpio_pin_toggle_dt(&led);
}

int main(void)
{
    
	int ret=0;

	k_work_init_delayable(&led_work, led_work_handler);

	/* check if sw0 is ready */
	if (!device_is_ready(sw0.port))
	{
		return -1;
	}

	/* configure sw0 as input */
	ret = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	if (ret != 0)
	{
		return -1;
	}

	/* configure sw0 interrupt */
	ret = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0)
	{
		return -1;
	}

	/* register sw0 callback */
	gpio_init_callback(&sw0_cb_data, sw0_cb, BIT(sw0.pin));
	gpio_add_callback(sw0.port, &sw0_cb_data);

	/* check if the led port is ready */
	if (!device_is_ready(led.port))
	{
		ret = -1;
	}
	else
	{
		/* configure the led pin */
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		if (ret < 0)
		{
			ret = -2;
		}
	}

    return 0;
}
