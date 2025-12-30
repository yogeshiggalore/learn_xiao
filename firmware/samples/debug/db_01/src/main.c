/* include kernel components */
#include <zephyr/kernel.h>

/* include device drivers */
#include <zephyr/drivers/gpio.h>

#define LED_SLEEP_TIME_MS   1000

/* the device tree indentifier for the appled0 node */
#define LED0_NODE DT_ALIAS(appled0)

/* get devicetree spec for led0 node */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

struct db_params {
	uint32_t param1;
	uint32_t param2;
};

static volatile uint32_t cntr;
struct db_params params = {
	.param1 = 10,
	.param2 = 20,
};

int main(void)
{
	int ret;

	cntr=0;

	/* check if the led port is ready */
	if (!device_is_ready(led.port))
	{
		return -1;
	}

	/* configure the led pin */
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		return -1;
	}
	/* blink the led */
	while (1)
	{
		/* toggle the led pin */
		gpio_pin_toggle(led.port, led.pin);
		/* sleep for some time */
		k_msleep(LED_SLEEP_TIME_MS);
		cntr++;
		printk("cntr:%d\n", cntr);
		params.param1 += 1;
		params.param2 += 2;
	}

    return 0;
}
