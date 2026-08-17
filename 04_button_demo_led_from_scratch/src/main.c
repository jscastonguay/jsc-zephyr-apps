#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* Port GPIOB, broche 0 = LD1 sur la Nucleo-H753ZI */
#define GPIO_NODE DT_NODELABEL(gpiob)
#define PIN       14

int main(void)
{
    const struct device *gpio_dev = DEVICE_DT_GET(GPIO_NODE);

    if (!device_is_ready(gpio_dev)) {
        return -1;
    }

    gpio_pin_configure(gpio_dev, PIN, GPIO_OUTPUT_ACTIVE);

    while (1) {
        gpio_pin_toggle(gpio_dev, PIN);
        k_msleep(1000);
    }

    return 0;
}