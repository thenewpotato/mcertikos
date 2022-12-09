#include "vga.h"

#include <lib/x86.h>
#include <lib/debug.h>
#include <lib/string.h>
#include "font8x8_basic.h"

uint16_t planes[4] = { VGA_PLANE_1, VGA_PLANE_2, VGA_PLANE_3, VGA_PLANE_4 };

struct vga_terminal terminal;

int terminal_active = 1;

char reverse_byte(char byte) {
    char reversed = 0;
    for (int i = 0; i < 8; i++) {
        int bit = (byte & (1 << i)) != 0;
        reversed = reversed | (bit << (7 - i));
    }
    return reversed;
}

void vga_plane_draw_byte(unsigned int row, unsigned int col, unsigned char bitmap, unsigned char color, uint16_t plane) {
    KERN_ASSERT(col % 8 == 0);
    size_t bit_position = VGA_BUF * 8 + row * VGA_COLS + col;
    unsigned char *byte = (unsigned char *) (bit_position / 8);
    int plane_active;
    if (plane == VGA_PLANE_1) {
        plane_active = (color & 0b00000001) != 0;
    } else if (plane == VGA_PLANE_2) {
        plane_active = (color & 0b00000010) != 0;
    } else if (plane == VGA_PLANE_3) {
        plane_active = (color & 0b00000100) != 0;
    } else if (plane == VGA_PLANE_4) {
        plane_active = (color & 0b00001000) != 0;
    } else {
        KERN_ASSERT(0);
        return;
    }

    *byte = plane_active ? bitmap : 0;
}

void vga_draw_chars_plane(uint16_t plane) {
    outw(0x3ce, 0x5);
    outw(0x3c4, plane);
    for (int i = terminal.cursor_start; i < terminal.cursor_start + VGA_NUM_CHARS; i++) {
        int vga_index = i - terminal.cursor_start;
        int char_row = vga_index / VGA_TEXT_COLS;
        int char_col = vga_index % VGA_TEXT_COLS;
        int c = terminal.buffer[i % VGA_NUM_CHARS];
        if (i < terminal.cursor_pos) {
            for (int j = 0; j < 8; j++) {
                char bitmap = font8x8_basic[c][j];
                char reversed = reverse_byte(bitmap);
                //KERN_INFO("%d %d %x\n", char_row * 8, char_col * 8, font8x8_basic[c][j]);
                vga_plane_draw_byte(char_row * 8 + j, char_col * 8, reversed, 0xf, plane);
            }
        } else if (i == terminal.cursor_pos) {
            // cursor
            for (int j = 0; j < 8; j++) {
                char bitmap = font8x8_basic['_'][j];
                char reversed = reverse_byte(bitmap);
                vga_plane_draw_byte(char_row * 8 + j, char_col * 8, reversed, 0xf, plane);
            }
        } else {
            // print blank
            for (int j = 0; j < 8; j++) {
                vga_plane_draw_byte(char_row * 8 + j, char_col * 8, 0b11111111, 0, plane);
            }
        }
    }
    outw(0x3c4, 0xf02);
}

void vga_draw_chars() {
    vga_draw_chars_plane(VGA_PLANE_1);
    vga_draw_chars_plane(VGA_PLANE_2);
    vga_draw_chars_plane(VGA_PLANE_3);
    vga_draw_chars_plane(VGA_PLANE_4);
}

void vga_init(void) {
    struct rect_loc loc;
    loc.width = 8;
    loc.height = 16;
    loc.row_start = 400;
    loc.col_start = 400;
    char bitmap[16];
    memset(bitmap, 0b11111111, 16);
    vga_set_rectangle(loc, bitmap, 0x4);
}

void vga_putc(char c) {
    if (c == '\n') {
        memset(&terminal.buffer[terminal.cursor_pos % VGA_NUM_CHARS], 0,
               VGA_TEXT_COLS - terminal.cursor_pos % VGA_TEXT_COLS);
        int row = terminal.cursor_pos / VGA_TEXT_COLS;
        terminal.cursor_pos = (row + 1) * VGA_TEXT_COLS;
    } else if (c == '\t') {
        vga_putc(' ');
        vga_putc(' ');
        vga_putc(' ');
        vga_putc(' ');
    } else if (c == '\b') {
        if (terminal.cursor_pos > terminal.cursor_start) {
            terminal.cursor_pos--;
        }
    } else {
        terminal.buffer[terminal.cursor_pos % VGA_NUM_CHARS] = c;
        terminal.cursor_pos++;
    }

    if (terminal.cursor_pos >= terminal.cursor_start + VGA_NUM_CHARS) {
        terminal.cursor_start += VGA_TEXT_COLS;
    }

    if (terminal_active) {
        vga_draw_chars();
    }
}

void vga_clear() {
    for (int i = 0; i < 4; i++) {
        outw(0x3ce, 0x5);
        outw(0x3c4, planes[i]);
        for (int row = 0; row < VGA_ROWS; row++) {
            for (int col = 0; col < VGA_COLS; col += 8) {
                vga_plane_draw_byte(row, col, 0b11111111, 0, planes[i]);
            }
        }
        outw(0x3c4, 0xf02);
    }
}

void vga_set_rectangle(struct rect_loc loc, const char * bitmap_rect, unsigned char color) {
    KERN_ASSERT(loc.row_start % 8 == 0);
    KERN_ASSERT(loc.col_start % 8 == 0);
    KERN_ASSERT(loc.width % 8 == 0);
    KERN_ASSERT(loc.height % 8 == 0);
    for (int pi = 0; pi < 4; pi++) {
        outw(0x3ce, 0x5);
        outw(0x3c4, planes[pi]);
        for (int i = 0; i < loc.height; i++) {
            for (int j = 0; j < loc.width; j+=8) {
                unsigned int row = loc.row_start + i;
                unsigned int col = loc.col_start + j;
                unsigned char bitmap = bitmap_rect[(i * loc.width + j) / 8];
                vga_plane_draw_byte(row, col, bitmap, color, planes[pi]);
            }
        }
        outw(0x3c4, 0xf02);
    }
}

void vga_set_mode(unsigned int mode) {
    if (mode == VGA_MODE_VIDEO) {
        vga_clear();
        terminal_active = 0;
    } else if (mode == VGA_MODE_TERMINAL) {
        vga_clear();
        vga_draw_chars();
        terminal_active = 1;
    } else {
        KERN_ASSERT(0);
    }
}
