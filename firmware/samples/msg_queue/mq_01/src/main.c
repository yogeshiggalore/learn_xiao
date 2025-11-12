/* include zephyr kernel */
#include <zephyr/kernel.h>

/* include zephyr devicetree */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

/* define led0 node */
#define LED0_NODE DT_ALIAS(led0)

/* define sw0 node */
#define SW0_NODE DT_ALIAS(sw0)

/* get dt spec for led0 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* get dt spec for sw0 */
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

#define LED0_THREAD_STACK_SIZE 1024
#define LED0_THREAD_PRIORITY 5
#define SW0_THREAD_STACK_SIZE 1024
#define SW0_THREAD_PRIORITY 5

K_THREAD_STACK_DEFINE(led0_thread_stack, LED0_THREAD_STACK_SIZE);
struct k_thread led0_thread_data;

K_THREAD_STACK_DEFINE(sw0_thread_stack, SW0_THREAD_STACK_SIZE);
struct k_thread sw0_thread_data;

void led0_thread_fun(void);
void sw0_thread_fun(void);

typedef struct {
    uint32_t button_state;
    uint32_t timestamp;
} ButtonMessage;

K_MSGQ_DEFINE(button_message_queue, sizeof(ButtonMessage), 16, 4);

int main(void)
{
    return 0;
}

K_THREAD_DEFINE(led0_thread_id, LED0_THREAD_STACK_SIZE, led0_thread_fun, NULL, NULL, NULL, LED0_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(sw0_thread_id, SW0_THREAD_STACK_SIZE, sw0_thread_fun, NULL, NULL, NULL, SW0_THREAD_PRIORITY, 0, 0);

void led0_thread_fun(void)
{
	int ret = 0;
	ButtonMessage received_msg;

	/* check if the led port is ready */
	if (!device_is_ready(led0.port))
	{
		ret = -1;
	}
	else
	{
		/* configure the led pin */
		ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
		if (ret < 0)
		{
			ret = -2;
		}
	}
	
	while (true)
	{
		/* Wait for message from queue */
        ret = k_msgq_get(&button_message_queue, &received_msg, K_FOREVER);

        /* Control LED based on button state */
        if (received_msg.button_state) {
            gpio_pin_set_dt(&led0, 1); /* Turn LED on */
        } else {
            gpio_pin_set_dt(&led0, 0); /* Turn LED off */
        }
	}
}


void sw0_thread_fun(void)
{
	int ret = 0;
	ButtonMessage msg;

	/* check if sw0 is ready */
	if (!device_is_ready(sw0.port))
	{
		ret = -1;
	}

	/* configure sw0 as input */
	ret = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	if (ret != 0)
	{
		ret = -2;
	}
	
	while (true)
	{
        msg.button_state = gpio_pin_get_dt(&sw0);
        msg.timestamp = k_uptime_get_32();

        ret = k_msgq_put(&button_message_queue, &msg, K_FOREVER);
        k_msleep(100);
	}
}