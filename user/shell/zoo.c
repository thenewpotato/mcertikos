#include "zoo.h"

#include <syscall.h>
#include <stdio.h>

char giraffe[VGA_ROWS][VGA_COLS] = {
#include "giraffe.inc"
};

char dolphin[VGA_ROWS][VGA_COLS] = {
#include "dolphin.inc"
};

void zoo_run() {
    sys_setvideo(VGA_MODE_VIDEO);

    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            sys_draw_pixel(row, col, dolphin[row][col]);
        }
    }

    while(1) {
        char c = getc();
        if (c == 24) {
            break;
        }
    }
    sys_setvideo(VGA_MODE_TERMINAL);
}