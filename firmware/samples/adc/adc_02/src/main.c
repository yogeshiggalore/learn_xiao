#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

#define USER_NODE DT_PATH(zephyr_user)

#if !DT_NODE_EXISTS(USER_NODE)
#error "zephyr,user node missing"
#endif

static const struct gpio_dt_spec mic_en = GPIO_DT_SPEC_GET(USER_NODE, mic_en_gpios);
static const struct adc_dt_spec  mic_adc = ADC_DT_SPEC_GET(USER_NODE);

int main(void)
{
    int ret;
    int32_t mv;
    int16_t sample_buf;

    printk("\n--- Analog MIC (ADC) example ---\n");

    if (!device_is_ready(mic_en.port)) {
        printk("MIC_EN GPIO not ready\n");
        return 0;
    }

    if (!adc_is_ready_dt(&mic_adc)) {
        printk("ADC not ready\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&mic_en, GPIO_OUTPUT_INACTIVE);
    if (ret) {
        printk("gpio_pin_configure_dt failed: %d\n", ret);
        return 0;
    }

    gpio_pin_set_dt(&mic_en, 1);
    k_sleep(K_MSEC(50));

    ret = adc_channel_setup_dt(&mic_adc);
    if (ret) {
        printk("adc_channel_setup_dt failed: %d\n", ret);
        return 0;
    }

    struct adc_sequence sequence = {
        .buffer = &sample_buf,
        .buffer_size = sizeof(sample_buf),
    };

    while (1) {
        ret = adc_sequence_init_dt(&mic_adc, &sequence);
        if (ret) {
            printk("adc_sequence_init_dt failed: %d\n", ret);
            break;
        }

        ret = adc_read_dt(&mic_adc, &sequence);
        if (ret) {
            printk("adc_read_dt failed: %d\n", ret);
        } else {
            mv = sample_buf;
            ret = adc_raw_to_millivolts_dt(&mic_adc, &mv);
            if (ret < 0) {
                printk("raw=%d\n", sample_buf);
            } else {
                printk("raw=%d mv=%ld\n", sample_buf, (long)mv);
            }
        }

        k_sleep(K_MSEC(20));
    }

    return 0;
}
