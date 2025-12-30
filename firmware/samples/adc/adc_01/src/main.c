#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>


static const struct adc_dt_spec adc_chan0 = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
int main(void)
{
    int err;
    int16_t buf;
    struct adc_sequence sequence = 
	{
        .buffer = &buf,
        .buffer_size = sizeof(buf),
    };

    /* Check if the ADC device is ready */
    if (!adc_is_ready_dt(&adc_chan0)) {
        printk("ADC device not ready\n");
        return 0;
    }

    /* Setup the channel using the DT spec */
    err = adc_channel_setup_dt(&adc_chan0);
    if (err < 0) {
        printk("Could not setup channel (%d)\n", err);
        return 0;
    }

    while (1) {
        /* Initialize sequence from devicetree specs (resolution, etc.) */
        err = adc_sequence_init_dt(&adc_chan0, &sequence);
        if (err < 0) {
            printk("Sequence init failed (%d)\n", err);
            continue;
        }

        /* Read the raw value */
        err = adc_read(adc_chan0.dev, &sequence);
        if (err == 0) {
            int32_t val_mv = buf;
            /* Convert raw value to millivolts using the DT spec */
            err = adc_raw_to_millivolts_dt(&adc_chan0, &val_mv);
            if (err < 0) {
                printk("Raw: %d (mV conversion not supported)\n", buf);
            } else {
                printk("Raw: %d, Voltage: %d mV\n", buf, val_mv);
            }
        } else {
            printk("ADC read failed (%d)\n", err);
        }

        k_msleep(1000);
    }

	return 0;
}