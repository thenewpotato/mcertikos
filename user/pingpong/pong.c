#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("pong started.\n");

    for (i = 0; i < 256; i++) {
            consume();
    }

    printf("pong ended\n");

    return 0;
}
