#include "pong.h"

#include <syscall.h>
#include <stdio.h>
#include <fdlibm.h>

#define COLOR_LEFT_PADDLE   VGA_COLOR_BRIGHT_BLUE
#define COLOR_RIGHT_PADDLE  VGA_COLOR_BRIGHT_RED
#define COLOR_BALL          VGA_COLOR_WHITE

#define BALL_WIDTH  8
#define BALL_HEIGHT 8
struct ball_state {
    double x;
    double y;
    double angle;
};
struct game_state {
    int sleep_ball;
    int left_score;
    int right_score;
};

#define PADDLE_WIDTH    16
#define PADDLE_HEIGHT   64

char bitmap_ball[8];

int rect_loc_equals(struct rect_loc l1, struct rect_loc l2) {
    return (
            l1.row_start == l2.row_start &&
            l1.col_start == l2.col_start &&
            l1.width == l2.width &&
            l1.height == l2.height
    );
}

int ball_loc_equals(struct ball_state b1, struct ball_state b2) {
    return (b1.x == b2.x && b1.y == b2.y);
}

struct rect_loc ball_loc_appr(struct ball_state b) {
    int x_int = b.x;
    int y_int = b.y;
    int y_aligned = 8 * (y_int / 8);
    struct rect_loc loc = { x_int, y_aligned, BALL_WIDTH, BALL_HEIGHT };
    return loc;
}

void draw_ball(struct ball_state b, unsigned char color) {
    struct rect_loc ball_loc = ball_loc_appr(b);
    sys_draw(&ball_loc, bitmap_ball, color);
}

#define PI  3.14159265359
#define BALL_VELOCITY   8

void pong_run() {
    int aDown = 0;
    int sDown = 0;
    int kDown = 0;
    int lDown = 0;

    struct ball_state ball = { VGA_ROWS / 2, VGA_COLS / 2, PI};
    struct rect_loc left_paddle = {(VGA_ROWS - PADDLE_HEIGHT) / 2, 0, PADDLE_WIDTH, PADDLE_HEIGHT};
    struct rect_loc right_paddle = {(VGA_ROWS - PADDLE_HEIGHT) / 2, VGA_COLS - PADDLE_WIDTH, PADDLE_WIDTH, PADDLE_HEIGHT};
    char bitmap_paddle[128];
    memset(bitmap_paddle, 0b11111111, 128);
    memset(bitmap_ball, 0b11111111, 8);
    struct rect_loc left_line = { 0, PADDLE_WIDTH, 1, VGA_ROWS };
    struct rect_loc right_line = { 0, VGA_COLS - PADDLE_WIDTH - 1, 1, VGA_ROWS };

    sys_setvideo(VGA_MODE_VIDEO);

    // initial frame
    sys_draw(&left_paddle, bitmap_paddle, COLOR_LEFT_PADDLE);
    sys_draw(&right_paddle, bitmap_paddle, COLOR_RIGHT_PADDLE);
    draw_ball(ball, COLOR_BALL);
    sys_draw(&left_line, bitmap_paddle, VGA_COLOR_GRAY);
    sys_draw(&right_line, bitmap_paddle, VGA_COLOR_GRAY);

    struct game_state state = { 100 };

    int counter = 0;
    while (1) {
        int c = getc();
        if (c == 30) {
            aDown = 1;
        } else if (c == 158) {
            aDown = 0;
        } else if (c == 31) {
            sDown = 1;
        } else if (c == 159) {
            sDown = 0;
        } else if (c == 37) {
            kDown = 1;
        } else if (c == 165) {
            kDown = 0;
        } else if (c == 38) {
            lDown = 1;
        } else if (c == 166) {
            lDown = 0;
        } else if (c == 45) {
            break;
        }

        if (counter == 1000) {
            counter = 0;
        } else {
            counter++;
            continue;
        }

        struct ball_state old_ball = ball;
        struct rect_loc old_left_paddle = left_paddle;
        struct rect_loc old_right_paddle = right_paddle;

        // state updates
        if (aDown) {
            // left up
            if (left_paddle.row_start > 0) {
                left_paddle.row_start -= 8;
            }
        }
        if (sDown) {
            // left down
            if (left_paddle.row_start < VGA_ROWS - left_paddle.height) {
                left_paddle.row_start += 8;
            }
        }
        if (kDown) {
            // right up
            if (right_paddle.row_start > 0) {
                right_paddle.row_start -= 8;
            }
        }
        if (lDown) {
            // right down
            if (right_paddle.row_start < VGA_ROWS - right_paddle.height) {
                right_paddle.row_start += 8;
            }
        }
        if (state.sleep_ball) {
            state.sleep_ball--;
        } else {
            ball.x -= sin(ball.angle) * BALL_VELOCITY;
            ball.y += cos(ball.angle) * BALL_VELOCITY;
        }
        if (ball.y <= PADDLE_WIDTH) {
            if (ball.x + BALL_HEIGHT >= left_paddle.row_start && ball.x <= left_paddle.row_start + PADDLE_HEIGHT) {
                int paddle_middle = left_paddle.row_start + PADDLE_HEIGHT / 2;
                ball.angle = (paddle_middle - ball.x) / 32 * 1.2;
                ball.y = PADDLE_WIDTH + 2;
            } else {
                ball.x = VGA_ROWS / 2;
                ball.y = VGA_COLS / 2;
                ball.angle = 0;
                state.sleep_ball = 100;
                state.right_score++;
            }
        }
        if (ball.y >= VGA_COLS - PADDLE_WIDTH) {
            if (ball.x + BALL_HEIGHT >= right_paddle.row_start && ball.x <= right_paddle.row_start + PADDLE_HEIGHT) {
                int paddle_middle = right_paddle.row_start + PADDLE_HEIGHT / 2;
                ball.angle = (ball.x - paddle_middle)/ 32 * 1.2 + PI;
                ball.y = VGA_COLS - PADDLE_WIDTH - 2;
            } else {
                ball.x = VGA_ROWS / 2;
                ball.y = VGA_COLS / 2;
                ball.angle = PI;
                state.sleep_ball = 100;
                state.left_score++;
            }
        }
        if (ball.x <= 0 || ball.x + BALL_HEIGHT >= VGA_ROWS) {
            ball.angle = -ball.angle;
        }

        // re-render
        if (!rect_loc_equals(old_left_paddle, left_paddle)) {
            sys_draw(&old_left_paddle, bitmap_paddle, VGA_COLOR_BLACK);
            sys_draw(&left_paddle, bitmap_paddle, COLOR_LEFT_PADDLE);
        }
        if (!rect_loc_equals(old_right_paddle, right_paddle)) {
            sys_draw(&old_right_paddle, bitmap_paddle, VGA_COLOR_BLACK);
            sys_draw(&right_paddle, bitmap_paddle, COLOR_RIGHT_PADDLE);
        }
        if (!ball_loc_equals(old_ball, ball)) {
            draw_ball(old_ball, VGA_COLOR_BLACK);
            draw_ball(ball, COLOR_BALL);
        }
        sys_draw(&left_line, bitmap_paddle, VGA_COLOR_GRAY);
        sys_draw(&right_line, bitmap_paddle, VGA_COLOR_GRAY);
    }

    sys_setvideo(VGA_MODE_TERMINAL);
}