#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define IMU_NODE DT_ALIAS(imu0)
#define I2C_BUS  DT_BUS(IMU_NODE)

#define LSM6DSL_REG_WHOAMI  0x0F
#define LSM6DSL_WHOAMI_VAL  0x6A

static void print_sv(const char *name, const struct sensor_value *v)
{
    int32_t v1 = v->val1;
    int32_t v2 = v->val2;

    /* Normalize so fraction is always positive when we print */
    if (v1 < 0 || v2 < 0) {
        /* Convert to total micro-units first */
        int64_t total = (int64_t)v1 * 1000000LL + (int64_t)v2;
        if (total < 0) {
            total = -total;
            printk("%s=-%lld.%06lld ", name,
                   total / 1000000LL, total % 1000000LL);
            return;
        }
    }

    printk("%s=%d.%06d ", name, v1, v2);
}

static int whoami_check(const struct device *i2c, uint8_t addr)
{
    uint8_t reg = LSM6DSL_REG_WHOAMI;
    uint8_t val = 0;

    int ret = i2c_write_read(i2c, addr, &reg, 1, &val, 1);
    if (ret) {
        printk("WHO_AM_I read failed: %d\n", ret);
        return ret;
    }

    printk("WHO_AM_I = 0x%02x (expected 0x%02x)\n", val, LSM6DSL_WHOAMI_VAL);
    return (val == LSM6DSL_WHOAMI_VAL) ? 0 : -1;
}

static int configure_imu(const struct device *imu)
{
    int ret;
    struct sensor_value odr;
    struct sensor_value fs;

    /* Set ODR (Hz) */
    odr.val1 = 104; odr.val2 = 0; /* 104 Hz */
    ret = sensor_attr_set(imu, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    if (ret) {
        printk("accel ODR set failed: %d\n", ret);
        return ret;
    }

    ret = sensor_attr_set(imu, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    if (ret) {
        printk("gyro ODR set failed: %d\n", ret);
        return ret;
    }

    /* Set full scale */
    fs.val1 = 16; fs.val2 = 0; /* accel +/-16g (driver uses g units) */
    ret = sensor_attr_set(imu, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &fs);
    if (ret) {
        printk("accel FS set failed: %d\n", ret);
        /* not fatal on some builds, continue */
    }

    fs.val1 = 2000; fs.val2 = 0; /* gyro +/-2000 dps */
    ret = sensor_attr_set(imu, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &fs);
    if (ret) {
        printk("gyro FS set failed: %d\n", ret);
        /* not fatal on some builds, continue */
    }

    return 0;
}

int main(void)
{
    const struct device *imu = DEVICE_DT_GET(IMU_NODE);
    const struct device *i2c = DEVICE_DT_GET(I2C_BUS);
    const uint8_t imu_addr = DT_REG_ADDR(IMU_NODE);

    printk("\n--- LSM6DSL demo ---\n");

    k_sleep(K_MSEC(50));

    if (!device_is_ready(i2c)) {
        printk("I2C bus not ready: %s\n", i2c->name);
        return 0;
    }

    /* Prove sensor responds */
    if (whoami_check(i2c, imu_addr) != 0) {
        printk("IMU responds on I2C but WHO_AM_I mismatch -> check part/address\n");
        return 0;
    }

    if (!device_is_ready(imu)) {
        printk("IMU device not ready (driver didn't bind)\n");
        return 0;
    }

    printk("IMU bound: %s\n", imu->name);

    /* Force config so it is not in power-down */
    configure_imu(imu);

    /* Give it time to produce first samples */
    k_sleep(K_MSEC(50));

    while (1) {
        struct sensor_value ax, ay, az;
        struct sensor_value gx, gy, gz;

        int ret = sensor_sample_fetch(imu);
        if (ret) {
            printk("sensor_sample_fetch failed: %d\n", ret);
            k_sleep(K_MSEC(200));
            continue;
        }

        ret  = sensor_channel_get(imu, SENSOR_CHAN_ACCEL_X, &ax);
        ret |= sensor_channel_get(imu, SENSOR_CHAN_ACCEL_Y, &ay);
        ret |= sensor_channel_get(imu, SENSOR_CHAN_ACCEL_Z, &az);
        ret |= sensor_channel_get(imu, SENSOR_CHAN_GYRO_X,  &gx);
        ret |= sensor_channel_get(imu, SENSOR_CHAN_GYRO_Y,  &gy);
        ret |= sensor_channel_get(imu, SENSOR_CHAN_GYRO_Z,  &gz);

        if (ret) {
            printk("sensor_channel_get failed: %d\n", ret);
            k_sleep(K_MSEC(200));
            continue;
        }

        printk("ACC: ");
        print_sv("ax", &ax);
        print_sv("ay", &ay);
        print_sv("az", &az);

        printk("| GYR: ");
        print_sv("gx", &gx);
        print_sv("gy", &gy);
        print_sv("gz", &gz);

        printk("\n");
        k_sleep(K_MSEC(1000));
    }
}
