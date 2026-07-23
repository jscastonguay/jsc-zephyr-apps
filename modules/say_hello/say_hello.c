#include "say_hello.h"
#include <zephyr/kernel.h>

void say_hello(void)
{
    printk("Hello, World!\n");
}