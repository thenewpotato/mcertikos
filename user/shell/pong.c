#include "pong.h"

#include <syscall.h>
#include <stdio.h>

void pong_run() {
    sys_setvideo(VGA_MODE_VIDEO);

    struct rect_loc loc;
    loc.width = 16;
    loc.height = 64;
    loc.row_start = 400;
    loc.col_start = VGA_COLS - loc.width;
    char bitmap[128];
    memset(bitmap, 0b11111111, 128);
    sys_draw(&loc, bitmap, VGA_COLOR_RED);

    while (1) {
        char c = getc();
        if (c == 24) {
            break;
        }
        if (c == 97) {
            if (loc.row_start <= 0) {
                continue;
            }
            sys_draw(&loc, bitmap, VGA_COLOR_BLACK);
            loc.row_start -= 8;
            sys_draw(&loc, bitmap, VGA_COLOR_RED);
        } else if (c == 115) {
            if (loc.row_start >= VGA_ROWS - loc.height) {
                continue;
            }
            sys_draw(&loc, bitmap, VGA_COLOR_BLACK);
            loc.row_start += 8;
            sys_draw(&loc, bitmap, VGA_COLOR_RED);
        }
        // printf("%d\n", c);
    }

    sys_setvideo(VGA_MODE_TERMINAL);
}