#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("ping started.\n");

    // fast producing
    for (i = 0; i < 128; i++)
        produce(i);

    for (i = 128; i < 128 * 5; i++) {
        if (i % 4 == 0)
            produce(i);
    }
    printf("ping ended\n");

    return 0;
}
