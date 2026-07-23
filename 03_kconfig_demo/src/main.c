#include <zephyr/random/random.h>

#ifdef CONFIG_SAY_HELLO
#include "say_hello.h"
#endif

static const uint32_t sleepTime_ms = 1000;

int main(void)
{
    uint32_t randomValue;
    double randomFloat;

    while (1) {
        randomValue = sys_rand32_get();
        randomFloat = ((double)randomValue) / UINT32_MAX;
        printk("Random Value: %f\n", randomFloat);

#ifdef CONFIG_SAY_HELLO
        say_hello();
#endif

        k_msleep(sleepTime_ms);
    }

    return 0;
}