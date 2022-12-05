// Text-mode CGA/VGA display output

#include <lib/string.h>

#include <lib/x86.h>
#include <lib/debug.h>

#include "video.h"
#include "font8x8_basic.h"

static unsigned addr_6845;
static struct video terminal;

void draw_pixels(unsigned int row, unsigned int col, unsigned char bitmap, unsigned char color) {
    KERN_ASSERT(col % 8 == 0);
    size_t bit_position = VGA_BUF * 8 + row * VGA_PIXELS_PER_ROW + col;
    unsigned char *byte = (unsigned char *) (bit_position / 8);
    unsigned char color_0 = (color & 0b00000001) != 0; // bit 0 of color
    unsigned char color_1 = (color & 0b00000010) != 0; // bit 1 of color
    unsigned char color_2 = (color & 0b00000100) != 0; // bit 2 of color
    unsigned char color_3 = (color & 0b00001000) != 0; // bit 3 of color

    // set plane 0 to bit 0 of color
    outw(0x3ce, 0x5);
    outw(0x3c4, 0x102);
    *byte = color_0 ? bitmap : 0;
    outw(0x3c4, 0xf02);

    // set plane 1 to bit 1 of color
    outw(0x3ce, 0x5);
    outw(0x3c4, 0x202);
    *byte = color_1 ? bitmap : 0;
    outw(0x3c4, 0xf02);

    // set plane 2 to bit 2 of color
    outw(0x3ce, 0x5);
    outw(0x3c4, 0x402);
    *byte = color_2 ? bitmap : 0;
    outw(0x3c4, 0xf02);

    // set plane 3 to bit 3 of color
    outw(0x3ce, 0x5);
    outw(0x3c4, 0x802);
    *byte = color_3 ? bitmap : 0;
    outw(0x3c4, 0xf02);
}

void draw_char(int char_row, int char_col, int c) {
    // specifies the location of the top-left pixel of the char
    int pixel_row = char_row * 8;
    int pixel_col = char_col * 8;
    for (int i = 0; i < 8; i++) {
        draw_pixels(pixel_row + i, pixel_col, font8x8_basic[c][i], 0xf);
    }
}

void video_init(void)
{
    volatile uint16_t *cp;
    uint16_t was;
    unsigned pos;

    /* Get a pointer to the memory-mapped text display buffer. */
    cp = (uint16_t *) CGA_BUF;
    was = *cp;
    *cp = (uint16_t) 0xA55A;
    if (*cp != 0xA55A) {
        cp = (uint16_t *) MONO_BUF;
        addr_6845 = MONO_BASE;
        dprintf("addr_6845:%x\n", addr_6845);
    } else {
        *cp = was;
        addr_6845 = CGA_BASE;
        dprintf("addr_6845:%x\n", addr_6845);
    }

    /* Extract cursor location */
    outb(addr_6845, 14);
    pos = inb(addr_6845 + 1) << 8;
    outb(addr_6845, 15);
    pos |= inb(addr_6845 + 1);

    terminal.crt_buf = (uint16_t *) cp;
    terminal.crt_pos = pos;
    terminal.active_console = 0;

    draw_char(0, 0, 'A');
}



void video_putc(int c)
{

}

void video_set_cursor(int x, int y)
{
    terminal.crt_pos = x * CRT_COLS + y;
}

void video_clear_screen()
{
    int i;
    for (i = 0; i < CRT_SIZE; i++) {
        terminal.crt_buf[i] = ' ';
    }
}
