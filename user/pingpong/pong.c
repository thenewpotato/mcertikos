#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("pong started.\n");

//    for (i = 0; i < 20; i++) {
//        if (i % 2 == 0)
//            consume();
//    }

    produce(5);
    produce(10);
    produce(7);
    produce(11);
    produce(19);
    produce(51);
    produce(23);
    produce(47);

    return 0;
}
