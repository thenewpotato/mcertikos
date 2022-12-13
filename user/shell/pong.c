#include "pong.h"

#include <syscall.h>
#include <stdio.h>

#define COLOR_LEFT_PADDLE   VGA_COLOR_BRIGHT_BLUE
#define COLOR_RIGHT_PADDLE  VGA_COLOR_BRIGHT_RED

void pong_run() {
    struct rect_loc ball = { 300, 300, 8, 8 };
    struct rect_loc left_paddle = {0, 0, 16, 64};
    struct rect_loc right_paddle = {0, VGA_COLS - 16, 16, 64};
    char bitmap_paddle[128];
    memset(bitmap_paddle, 0b11111111, 128);

    sys_setvideo(VGA_MODE_VIDEO);

    // initial frame
    sys_draw(&left_paddle, bitmap_paddle, COLOR_LEFT_PADDLE);
    sys_draw(&right_paddle, bitmap_paddle, COLOR_RIGHT_PADDLE);

    while (1) {
        struct rect_loc old_ball = ball;
        struct rect_loc old_left_paddle = left_paddle;
        struct rect_loc old_right_paddle = right_paddle;

        // state updates
        char c = getc();
        if (c == 24) {
            break;
        }
        if (c == 97) {
            // left up
            if (left_paddle.row_start <= 0)
                continue;
            left_paddle.row_start -= 8;
        } else if (c == 115) {
            // left down
            if (left_paddle.row_start >= VGA_ROWS - left_paddle.height) {
                continue;
            }
            left_paddle.row_start += 8;
        } else if (c == 107) {
            // right up
            if (right_paddle.row_start <= 0)
                continue;
            right_paddle.row_start -= 8;
        } else if (c == 108) {
            // right down
            if (right_paddle.row_start >= VGA_ROWS - right_paddle.height) {
                continue;
            }
            right_paddle.row_start += 8;
        }

        // re-render
        sys_draw(&old_left_paddle, bitmap_paddle, VGA_COLOR_BLACK);
        sys_draw(&left_paddle, bitmap_paddle, COLOR_LEFT_PADDLE);
        sys_draw(&old_right_paddle, bitmap_paddle, VGA_COLOR_BLACK);
        sys_draw(&right_paddle, bitmap_paddle, COLOR_RIGHT_PADDLE);
    }

    sys_setvideo(VGA_MODE_TERMINAL);
}