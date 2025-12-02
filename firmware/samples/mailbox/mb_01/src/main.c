#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_THREAD_STACK_SIZE 1024
#define LED0_THREAD_PRIORITY 5

#define LED0_SLEEP_TIME_MS 100

/* the device tree indentifier for the led0 node */
#define LED0_NODE DT_ALIAS(appled0)

/* define sw0 node */
#define SW0_NODE DT_ALIAS(appsw0)

#define MSG_LED_ON  1
#define MSG_LED_OFF 0

/* get devicetree spec for led0 node */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* get dt spec for sw0 */
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

K_THREAD_STACK_DEFINE(led0_thread_stack, LED0_THREAD_STACK_SIZE);

k_tid_t led0_thread_id;
struct k_thread led0_thread_data;

struct k_mbox led_mbox;

void led0_thread_fun(void *p1, void *p2, void *p3);

/* gpio callback */
static struct gpio_callback sw0_cb_data;

void sw0_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{		
	static uint32_t led_state = MSG_LED_OFF;
	struct k_mbox_msg msg;
	int ret;

	/* Toggle the state and prepare the message info field */
	led_state = (led_state == MSG_LED_OFF) ? MSG_LED_ON : MSG_LED_OFF;
	msg.info = led_state;
	msg.size = 0; /* No actual data buffer is passed, just the info field */
	msg.tx_data = NULL;
	msg.tx_target_thread = K_ANY; /* Any waiting thread can receive it */

	/* Send message synchronously, waiting until a consumer is ready */
	ret = k_mbox_put(&led_mbox, &msg, K_FOREVER);
}

int main(void)
{
    
	int ret=0;

	k_mbox_init(&led_mbox);

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

	led0_thread_id = k_thread_create(&led0_thread_data, led0_thread_stack, 
                            LED0_THREAD_STACK_SIZE, led0_thread_fun,
                            NULL, NULL, NULL, LED0_THREAD_PRIORITY, 0, K_NO_WAIT);

    return 0;
}

void led0_thread_fun(void *p1, void *p2, void *p3)
{
	int ret = 0;
	struct k_mbox_msg msg;
	bool current_led_state = false;

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

		msg.size = 0; // We expect info in the 'info' field, not data buffer
		msg.rx_source_thread = K_ANY; /* Any thread can send the message */

		/* Wait forever for a message to arrive */
		ret = k_mbox_get(&led_mbox, &msg, NULL, K_FOREVER);

		/* sleep for some time */
		k_msleep(LED0_SLEEP_TIME_MS);

		if(ret == 0)
		{
			current_led_state = (msg.info == MSG_LED_ON);
			gpio_pin_set_dt(&led, current_led_state);
		}
	}
}

