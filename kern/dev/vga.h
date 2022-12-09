#ifndef _KERN_DEV_VGA_H_
#define _KERN_DEV_VGA_H_

#ifdef _KERN_

#include <lib/x86.h>
#define VGA_BUF   0xA0000
#define VGA_COLS    640
#define VGA_ROWS    480

#define VGA_TEXT_ROWS   (60)
#define VGA_TEXT_COLS    (80)
#define VGA_NUM_CHARS  (VGA_TEXT_ROWS * VGA_TEXT_COLS)

#define VGA_PLANE_1 0x102
#define VGA_PLANE_2 0x202
#define VGA_PLANE_3 0x402
#define VGA_PLANE_4 0x802

#define VGA_MODE_VIDEO      0
#define VGA_MODE_TERMINAL   1

struct vga_terminal {
    char buffer[VGA_NUM_CHARS];
    int cursor_start;
    int cursor_pos; // length of buffer
};

struct rect_loc {
    int row_start;
    int col_start;
    int width;
    int height;
};

void vga_init(void);
void vga_putc(char);
void vga_set_mode(unsigned int);
void vga_set_rectangle(struct rect_loc loc, const char * bitmap_rect, unsigned char color);

#endif  /* _KERN_ */

#endif  /* !_KERN_DEV_VGA_H_ */
