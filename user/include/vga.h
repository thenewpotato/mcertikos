#ifndef _USER_VGA_H_
#define _USER_VGA_H_

struct rect_loc {
    int row_start;
    int col_start;
    int width;
    int height;
};

#define VGA_COLOR_WHITE     0xf
#define VGA_COLOR_BLACK     0x0
#define VGA_COLOR_RED       0x4
#define VGA_COLOR_BLUE      0x1
#define VGA_COLOR_GREEN     0x2
#define VGA_COLOR_CYAN      0x3
#define VGA_COLOR_MAGENTA   0x5
#define VGA_COLOR_BROWN     0x6
#define VGA_COLOR_BRIGHT_RED    0xc
#define VGA_COLOR_BRIGHT_BLUE   0x9

#define VGA_COLS    640
#define VGA_ROWS    480

#define VGA_MODE_VIDEO      0
#define VGA_MODE_TERMINAL   1

#endif  /* !_USER_VGA_H_ */
