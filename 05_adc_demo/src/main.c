#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

static const uint32_t sleep_time = 100;

#define MV_ADC_CH DT_ALIAS(my_adc_channel)
#define ADC_RESOLUTION DT_PROP(MV_ADC_CH, zephyr_resolution)

static const struct device *adc = DEVICE_DT_GET(DT_ALIAS(my_adc));
static const struct adc_channel_cfg adc_ch = ADC_CHANNEL_CFG_DT(MV_ADC_CH);

int main(void) {
    int ret;
    uint16_t buf;
    int32_t vref_mv;
    int32_t val_mv;

    if (!device_is_ready(adc)) {
        printk("ADC device not ready\n");
        return 1;
    }

    ret = adc_channel_setup(adc, &adc_ch);
    if (ret) {
        printk("ADC channel setup failed with error %d\n", ret);
        return 1;
    }

    ret = adc_ref_internal(adc);
    if (ret <= 0) {
        printk("Impossible d'obtenir la tension de référence interne\n");
        return 1;
    }
    vref_mv = ret;

    struct adc_sequence seq = {
        .channels = BIT(adc_ch.channel_id),
        .buffer = &buf,
        .buffer_size = sizeof(buf),
        .resolution = ADC_RESOLUTION,
    };

    while (1) {
        ret = adc_read(adc, &seq);
        if (ret) {
            printk("ADC read failed with error %d\n", ret);
            continue;
        }

        val_mv = buf * vref_mv / ((1 << ADC_RESOLUTION) - 1);

        printk("ADC value: %d, %d mV\n", buf, val_mv);

        k_msleep(sleep_time);
    }
}