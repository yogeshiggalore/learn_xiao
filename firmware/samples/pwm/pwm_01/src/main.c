/* include headers */
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#define PWM_PERIOD 	20
#define PWM_STEP	1

#define DIRECTION_FORWARD 0
#define DIRECTION_BACKWARD 1

#define PWM_NODE DT_ALIAS(apppwm)

const struct device *dev_pwm;

uint32_t cycle=5;
int direction=DIRECTION_FORWARD;

int main(void)
{
	int err=0;
	dev_pwm = DEVICE_DT_GET(PWM_NODE);
	if (!device_is_ready(dev_pwm)) {
		printk("PWM device not found\n");
		while(1)
		{
			printk("PWM device not found\n");
			k_sleep(K_MSEC(1000));
		}
	}
	while(true)
	{
		//err = pwm_set(dev_pwm,0,PWM_USEC(PWM_PERIOD),PWM_USEC(cycle),PWM_POLARITY_INVERTED);
		err = pwm_set(dev_pwm,0,PWM_MSEC(PWM_PERIOD),PWM_MSEC(cycle),PWM_POLARITY_NORMAL);
		if(direction == DIRECTION_FORWARD)
		{
			if(cycle < PWM_PERIOD)
			{
				cycle += PWM_STEP; 
			}
			else
			{
				direction = DIRECTION_BACKWARD;
				cycle = PWM_PERIOD;
			}
		}
		else
		{
			if(cycle > 0)
			{
				cycle -= PWM_STEP; 
			}
			else
			{
				direction = DIRECTION_FORWARD;
				cycle = 0;
			}
		}
		k_sleep(K_MSEC(PWM_PERIOD));
		printk("Cycle=%d\n",cycle);
	}
	return 0;
}
