#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const int32_t sleep_time_ms = 100;
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(mon_bouton), gpios);

int main(void) {
    
    int ret;
    int state;

    if (!gpio_is_ready_dt(&btn)) {
        printk("Error: button device %s is not ready\n", btn.port->name);
        return -1;
    }

    ret = gpio_pin_configure_dt(&btn, GPIO_INPUT);
    if (ret < 0) {
        printk("Error %d: failed to configure button device %s pin %d\n", ret, btn.port->name, btn.pin);
        return -1;
    }

    printk("Juste pour voir les flags settés par le devicetree ET dans ce code: 0x%08x\n", btn.dt_flags);

    while (1) {
        state = gpio_pin_get_dt(&btn);
        if (state < 0) {
            printk("Error %d: failed to read button device %s pin %d\n", state, btn.port->name, btn.pin);
        } else {
            printk("Button state: %d\n", state);
        }

        k_msleep(sleep_time_ms);
    }

    return 0;
}
