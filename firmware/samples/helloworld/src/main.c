#include <zephyr/kernel.h>

int main(void)
{
	while(true)
	{
		printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
		k_msleep(1000);
	}
    
	return 0;
}
