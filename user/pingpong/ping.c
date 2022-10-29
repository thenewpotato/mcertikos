#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("ping started.\n");

    produce(5);
    produce(10);
    produce(7);
    produce(11);

//    // fast producing
//    for (i = 0; i < 10; i++)
//        produce(i);
//
//    // slow producing
//    for (i = 0; i < 40; i++) {
//        if (i % 4 == 0)
//            produce(i);
//    }

    return 0;
}
