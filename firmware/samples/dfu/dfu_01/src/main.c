#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>

#define DEVICE_NAME CONFIG_BOARD
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Step 1: Prepare the advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Step 2: Prepare the scan response data */
static const uint8_t sd_uuid[] = {
	BT_UUID_128_ENCODE(0x00001523, 0x1212, 0xefde, 0x1523, 0x785feabcd123)
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_UUID128_ALL, sd_uuid, sizeof(sd_uuid)),
};

#define ADV_INTERVAL_1S 1600	/* 1 second in units of 0.625 ms */

static const struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY,
			ADV_INTERVAL_1S,
			ADV_INTERVAL_1S,
			NULL);

int main(void)
{
    int err;

	printk("Starting Bluetooth DFU Sample\n");
	printk("Device Name: %s\n", DEVICE_NAME);

	k_sleep(K_SECONDS(5));

    /* Step 3: Initialize the Bluetooth Subsystem */
    err = bt_enable(NULL);
    if (err) {
		printk("Bluetooth initialization failed (err %d)\n", err);
        return 0;
    }

    /* Step 4: Start advertising */
    /* BT_LE_ADV_NCONN: non-connectable advertising */
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
		printk("Advertising failed to start (err %d)\n", err);
        return 0;
    }

	while(1)
	{
		printk("Advertising...\n");
		printk("FW: %s\n", CONFIG_BT_DIS_FW_REV_STR);
		k_sleep(K_SECONDS(5));
	}
    return 0;
}