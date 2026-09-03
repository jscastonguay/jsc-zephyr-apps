// Ties to the 'compatible = "custom,button"' node in the device tree.
#define DT_DRV_COMPAT custom_button

#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#include "button.h"

// Active le logging au niveau par défaut.
LOG_MODULE_REGISTER(button);

static int button_init(const struct device *dev);
static int button_state_get(const struct device *dev, uint8_t *state);

static int button_init(const struct device *dev) {

  // dev->config est de type void.
  const struct button_config *cfg = dev->config;

  const struct gpio_dt_spec *btn = &cfg->btn;

  LOG_DBG("Initializing button device: %d", cfg->id);

  if (!gpio_is_ready_dt(btn)) {
    LOG_ERR("GPIO device not ready");
    return -ENODEV;
  }

  int ret = gpio_pin_configure_dt(btn, GPIO_INPUT);
  if (ret < 0) {
    LOG_ERR("Failed to configure button GPIO pin");
    return ret;
  }

  return 0;
}

static int button_state_get(const struct device *dev, uint8_t *state) {
  const struct button_config *cfg = dev->config;
  const struct gpio_dt_spec *btn = &cfg->btn;

  int ret = gpio_pin_get_dt(btn);
  if (ret < 0) {
    LOG_ERR("Failed to read button state");
    return ret;
  }

  *state = (uint8_t)ret;
  return 0;
}

// Device tree handling

static const struct button_api button_api_funcs = {
    .get = button_state_get,
};

// On récupère les informations du device tree.
#define BUTTON_DEVICE_INIT(inst)                                               \
  static const struct button_config button_config_##inst = {                   \
      .btn = GPIO_DT_SPEC_GET(DT_PHANDLE(DT_INST(inst, custom_button), pin),   \
                              gpios),                                          \
      .id = inst};                                                             \
                                                                               \
  /* On crée les instances à partir du device tree. */                         \
  DEVICE_DT_INST_DEFINE(inst, button_init, NULL, NULL, &button_config_##inst,  \
                        POST_KERNEL, CONFIG_BUTTON_INIT_PRIORITY,              \
                        &button_api_funcs);

// Le build process des instances.
// La macro BUTTON_DEVICE_INIT sera "appelée" pour chaque instance
// avec inst = 0, 1, 2, ... jusqu'au nombre d'instances définies dans le device
// tree.
DT_INST_FOREACH_STATUS_OKAY(BUTTON_DEVICE_INIT)