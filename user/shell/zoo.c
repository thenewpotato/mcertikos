#include "zoo.h"

#include <syscall.h>
#include <stdio.h>

char giraffe[VGA_ROWS][VGA_COLS] = {
#include "giraffe.inc"
};

char dolphin[VGA_ROWS][VGA_COLS] = {
#include "dolphin.inc"
};

char images[2][VGA_ROWS][VGA_COLS] = {
        {
#include "giraffe.inc"
        },
        {
#include "dolphin.inc"
        }
};
int current = 0;

void render() {
    sys_setvideo(VGA_MODE_VIDEO);
    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            sys_draw_pixel(row, col, images[current % 2][row][col]);
        }
    }
}

void zoo_run() {
    sys_setvideo(VGA_MODE_VIDEO);

    render();

    while(1) {
        char c = getc();
        if (c == 45) {
            break;
        } else if (c == 75 || c == 77) {
            if (current == 0) {
                current = 1;
            } else {
                current = 0;
            }
            render();
        }
    }
    sys_setvideo(VGA_MODE_TERMINAL);
}