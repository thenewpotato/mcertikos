#include "zoo.h"

#include <syscall.h>
#include <stdio.h>

#define MAX_IMAGES 6
char images[MAX_IMAGES][VGA_ROWS][VGA_COLS] = {
        {
#include "include/giraffe.inc"
        },
        {
#include "include/dolphin.inc"
        },
        {
#include "include/peacock.inc"
        },
        {
#include "include/tiger.inc"
        },
        {
#include "include/jaguar.inc"
        },
        {
#include "include/swan.inc"
        },
};

int current = 0;

void render() {
    sys_setvideo(VGA_MODE_VIDEO);
    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            sys_draw_pixel(row, col, images[current][row][col]);
        }
    }
}

void zoo_run() {
    sys_setvideo(VGA_MODE_VIDEO);

    render();

    while (1) {
        char c = getc();
//        if (c != -1) {
//            printf("%d\n", c);
//        }
        if (c == 45) {
            break;
        } else if (c == 77) {
            if (current == MAX_IMAGES - 1) {
                current = 0;
            } else {
                current++;
            }
            render();
        } else if (c == 75) {
            if (current == 0) {
                current = MAX_IMAGES - 1;
            } else {
                current--;
            }
            render();
        }
    }
    sys_setvideo(VGA_MODE_TERMINAL);
}