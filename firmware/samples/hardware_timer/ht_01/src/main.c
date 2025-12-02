
/* include kernel, gpio device drivers and hardware timer */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>

/* timer value 500 ms*/
#define TIMER_VALUE 500

#define ALARM_CHANNEL_ID 0

/* led0 node */
#define LED0_NODE DT_ALIAS(appled0)

/* led0 device tree */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* timer0 configuration */
#define APP_TIMER0 DT_ALIAS(apptimer0)
struct counter_alarm_cfg timer0_cfg;
const struct device *const dev_timer0 = DEVICE_DT_GET(APP_TIMER0);

void app_timer0_cb(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data);

int main(void)
{
	int ret=0;
	int err=0;

	/* check for led0 device readiness */
	if(!gpio_is_ready_dt(&led0))
	{
		printk("led0 device not ready\n");
		return -1;
	}
	else
	{
		printk("led0 device ready\n");
	}

	/* configure led0 pin as output */
	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		printk("led0 configure failed\n");
		return ret;
	}
	else
	{
		printk("led0 configured\n");
	}

	/* check for timer0 device readiness */
	if(!device_is_ready(dev_timer0))
	{
		printk("timer0 device not ready\n");
		return -1;
	}
	else
	{
		printk("timer0 device ready\n");
	}

	printk("configure timer0\n");

	/* start timer with TIMER_VALUE */
	counter_start(dev_timer0);
	timer0_cfg.flags = 0;
	timer0_cfg.ticks = counter_us_to_ticks(dev_timer0, (TIMER_VALUE * 1000));
	timer0_cfg.callback = app_timer0_cb;
	timer0_cfg.user_data = (void*)&timer0_cfg;

	/* set alarm */
	err = counter_set_channel_alarm(dev_timer0, ALARM_CHANNEL_ID, &timer0_cfg);
	if (err < 0)
	{
		printk("timer0 set alarm failed\n");
		return err;
	}
	else
	{
		printk("timer0 set alarm\n");
	}

	printk("led0 on\n");
	gpio_pin_set_dt(&led0, false);

	while (true)
	{
		k_sleep(K_FOREVER);
	}

	return 0;
}

void app_timer0_cb(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data)
{
	int err;
    struct counter_alarm_cfg *cfg = (struct counter_alarm_cfg*)user_data;

    gpio_pin_toggle_dt(&led0);
	printk("led0 toggle\n");

	printk("reconfigure timer0 alarm\n");

    // Set ticks to the interval you want (relative to now)
    cfg->ticks = counter_us_to_ticks(dev_timer0, (TIMER_VALUE * 1000));
	cfg->callback = app_timer0_cb;
	cfg->flags = 0;
	cfg->user_data = (void*)cfg;

    err = counter_set_channel_alarm(dev_timer0, ALARM_CHANNEL_ID, cfg);
}
