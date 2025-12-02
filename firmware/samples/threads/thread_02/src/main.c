#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_THREAD_STACK_SIZE 1024
#define LED0_THREAD_PRIORITY 5
#define LED0_SLEEP_TIME_MS 100

/* the device tree indentifier for the led0 node */
#define LED0_NODE DT_ALIAS(appled0)

/* get devicetree spec for led0 node */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

K_THREAD_STACK_DEFINE(led0_thread_stack, LED0_THREAD_STACK_SIZE);
k_tid_t led0_thread_id;
struct k_thread led0_thread_data;


void led0_thread_fun(void *p1, void *p2, void *p3);

int main(void)
{

    led0_thread_id = k_thread_create(&led0_thread_data, led0_thread_stack, 
                            LED0_THREAD_STACK_SIZE, led0_thread_fun,
                            NULL, NULL, NULL, LED0_THREAD_PRIORITY, 0, K_NO_WAIT);

    return 0;
}

void led0_thread_fun(void *p1, void *p2, void *p3)
{
	int ret = 0;

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
	
	while (true)
	{
		/* sleep for some time */
		k_msleep(LED0_SLEEP_TIME_MS);

		if(ret == 0)
		{
			/* toggle the led pin */
			gpio_pin_toggle(led.port, led.pin);
		}
	}
}