#include "vga.h"

#include <lib/x86.h>
#include <lib/debug.h>
#include "font8x8_basic.h"

struct vga_terminal terminal;

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
    for (int i = 0; i < VGA_NUM_CHARS; i++) {
        int char_row = i / VGA_TEXT_COLS;
        int char_col = i % VGA_TEXT_COLS;
        int c = terminal.buffer[i];
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
                vga_plane_draw_byte(char_row * 8, char_col * 8, 0b11111111, 0, plane);
            }
        }
    }
    outw(0x3c4, 0xf02);
}

void vga_draw_chars_old() {
    vga_draw_chars_plane(VGA_PLANE_1);
    vga_draw_chars_plane(VGA_PLANE_2);
    vga_draw_chars_plane(VGA_PLANE_3);
    vga_draw_chars_plane(VGA_PLANE_4);
}

void vga_init(void) {
    // terminal fields are automatically zeroed
}

void vga_putc(char c) {
    if (c == '\n') {
        if (terminal.cursor_pos % VGA_TEXT_COLS != 0) {
            int row = terminal.cursor_pos / VGA_TEXT_COLS;
            terminal.cursor_pos = (row + 1) * VGA_TEXT_COLS;
        }
    } else {
        terminal.buffer[terminal.cursor_pos++] = c;
    }
    vga_draw_chars_old();
}

