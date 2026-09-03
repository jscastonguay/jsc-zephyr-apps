#include <stdio.h>
#include <zephyr/kernel.h>

#include "button.h"

static const uint32_t sleep_time_ms = 10;
static const struct device *btn_1 = DEVICE_DT_GET(DT_ALIAS(my_button_1));
static const struct device *btn_2 = DEVICE_DT_GET(DT_ALIAS(my_button_2));

int main(void)
{
    uint8_t state_1 = 0;
    uint8_t state_2 = 0;

    if (!device_is_ready(btn_1)) {
        printk("Button 1 device not ready\n");
        return -1;
    }

    if (!device_is_ready(btn_2)) {
        printk("Button 2 device not ready\n");
        return -1;
    }

    // On a pas besoin de récupérer l'API à chaque itération, on peut le faire une seule fois avant la boucle.
    // L'idée est que chaque button partage les mêmes méthodes. Ce sera l'instance qui sera passée en paramètre pour
    // interagir avec le bon périphérique. 
    // On aurait pu récupérer l'API à chaque itération, mais c'est moins efficace.
    const struct button_api *api_btn = (const struct button_api *)btn_1->api;


    while (1) {
        int ret;

        ret = api_btn->get(btn_1, &state_1);
        if (ret < 0) {
            printk("Error reading button 1 state: %d\n", ret);
            continue;
        }

        ret = api_btn->get(btn_2, &state_2);
        if (ret < 0) {
            printk("Error reading button 2 state: %d\n", ret);
            continue;
        }

        printk("Button 1 state: %d, Button 2 state: %d\n", state_1, state_2);
        k_msleep(sleep_time_ms);
    }

    return 0;
}