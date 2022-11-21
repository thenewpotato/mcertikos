#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("ping started.\n");

    // fast producing
    for (i = 0; i < 100; i++)
        produce(i);

    // slow producing
    for (i = 0; i < 400; i++)
    {
        if (i % 4 == 0)
            produce(i);
    }

    printf("ping ended\n");

    return 0;
}
