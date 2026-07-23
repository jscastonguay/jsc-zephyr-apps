#include <zephyr/random/random.h>

static const uint32_t sleepTime_ms = 1000;

int main(void)
{
    uint32_t randomValue;

    while (1) {
        randomValue = sys_rand32_get();
        printk("Random Value: %u\n", randomValue);
        k_msleep(sleepTime_ms);
    }

    return 0;
}