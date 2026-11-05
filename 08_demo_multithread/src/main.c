#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const int32_t blink_sleep_time = 500;
static const int32_t print_sleep_time = 700;

#define BLINK_THREAD_STACK_SIZE 256
K_THREAD_STACK_DEFINE(blink_stack, BLINK_THREAD_STACK_SIZE);

static struct k_thread blink_thread;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);

void blink_thread_start(void *arg1, void *arg2, void *arg3)
{
    int ret;
    int state = 0;



    while (1) {
        //gpio_pin_toggle_dt(&led);

        state = !state;
        ret = gpio_pin_set_dt(&led, state);
        
        if (ret < 0) {
            printk("Error %d: failed to set LED pin %d\n", ret, led.pin);
            return;
        }   

        k_msleep(blink_sleep_time);
    }
}

int main(void)
{
    int ret;
    k_tid_t blink_tid;

    if (!gpio_is_ready_dt(&led)) {
        printk("Error: LED device %s is not ready\n", led.port->name);
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin %d\n", ret, led.pin);
        return -1;
    }

    blink_tid = k_thread_create(&blink_thread, blink_stack,
                                K_THREAD_STACK_SIZEOF(blink_stack),
                                blink_thread_start,
                                NULL, NULL, NULL,
                                7, 0, K_NO_WAIT);

    while (1) {
        printk("Hello from main thread!\n");
        k_msleep(print_sleep_time); 
    }

    return 0;
}
