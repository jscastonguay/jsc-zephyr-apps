#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/console/console.h>

static const int32_t blink_max_ms = 2000;
static const int32_t blink_min_ms = 100;

#define BLINK_THREAD_STACK_SIZE 512
#define INPUT_THREAD_STACK_SIZE 512

K_THREAD_STACK_DEFINE(blink_stack, BLINK_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(input_stack, INPUT_THREAD_STACK_SIZE);

static struct k_thread blink_thread;
static struct k_thread input_thread;

K_MUTEX_DEFINE(my_mutex);

static int32_t blink_sleep_time = 500;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);

void input_thread_start(void *arg1, void *arg2, void *arg3)
{
    char * line;
    int32_t inc;

    while (1) {
        printk("Enter + or - to change blink speed: ");
        line = console_getline();
        if (!line) {
            printk("Error reading input\n");
            continue;
        }

        if (line[0] == '+') {
            inc = 1;
        } else if (line[0] == '-') {
            inc = -1;
        } else {
            continue;
        }

        k_mutex_lock(&my_mutex, K_FOREVER);
        blink_sleep_time += inc * 100;
        if (blink_sleep_time < blink_min_ms) {
            blink_sleep_time = blink_min_ms;
        } else if (blink_sleep_time > blink_max_ms) {
            blink_sleep_time = blink_max_ms;
        }
        k_mutex_unlock(&my_mutex);

        printk("Blink sleep time updated to %d ms\n", blink_sleep_time);
    }
}

void blink_thread_start(void *arg1, void *arg2, void *arg3)
{
    int ret;
    int state = 0;
    int32_t sleep_time;


    while (1) {
        //gpio_pin_toggle_dt(&led);

        k_mutex_lock(&my_mutex, K_FOREVER);
        sleep_time = blink_sleep_time;
        k_mutex_unlock(&my_mutex);

        state = !state;
        ret = gpio_pin_set_dt(&led, state);
        
        if (ret < 0) {
            printk("Error %d: failed to set LED pin %d\n", ret, led.pin);
            return;
        }   

        k_msleep(sleep_time);
    }
}

int main(void)
{
    int ret;
    k_tid_t blink_tid;
    k_tid_t input_tid;

    console_getline_init();

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

    input_tid = k_thread_create(&input_thread, input_stack,
                                K_THREAD_STACK_SIZEOF(input_stack),
                                input_thread_start,
                                NULL, NULL, NULL,
                                8, 0, K_NO_WAIT);

    while (1) {
        //printk("Hello from main thread!\n");
        k_msleep(1000); 
    }

    return 0;
}
