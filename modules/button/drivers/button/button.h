#ifndef ZEPHIR_DRIVERS_BUTTON_H
#define ZEPHIR_DRIVERS_BUTTON_H

#include <zephyr/drivers/gpio.h>

// Puisque l'on ne va pas utiliser l'API de zephyr, on va en créer un.

struct button_api {
    int (*get)(const struct device *dev, uint8_t *state);
};

// Config
struct button_config {
    const struct gpio_dt_spec btn;
    uint32_t id;
};

#endif
