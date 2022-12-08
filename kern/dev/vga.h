#ifndef _KERN_DEV_VGA_H_
#define _KERN_DEV_VGA_H_

#ifdef _KERN_

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

struct vga_terminal {
    char buffer[VGA_NUM_CHARS];
    int cursor_pos; // length of buffer
};

void vga_init(void);
void vga_putc(char);
void vga_flush(void);

#endif  /* _KERN_ */

#endif  /* !_KERN_DEV_VGA_H_ */
