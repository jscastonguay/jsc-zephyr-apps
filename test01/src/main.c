#include <stdio.h>
#include <stdbool.h>

unsigned int i = 0;

int main()
{
    while (true)
    {
        printf("Hello, World: %d\n", i);
        i++;
    }
    return 0;
}
