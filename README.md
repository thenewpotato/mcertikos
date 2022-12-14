Compile: make / make all
Run tests: make clean && make TEST=1
Run in qemu: make qemu / make qemu-nox
Debug with gdb: make qemu-gdb / make qemu-nox-gdb
(in another terminal) gdb

To use your solutions from lab 1: git merge lab1
To use sample lab 1 solutions: copy files in samples/ to appropriate directories

List here the following info:

1. who you have worked with
2. whether you coded this assignment together, and if not, who worked on which part
3. brief description of what you have implemented
4. and anything else you would like us to know

# CPSC 422 Lab 6 (Project 1: Video Mode)

You can preview this README file on Github (with screenshots and GIFs!): https://github.com/thenewpotato/mcertikos/tree/lab6

## Collaborators
- Tiger Wang (jw2723)
- Jackie Dong (jd2598)

## Building & Running

***NOTE: To interact with our graphics programs please test locally on a zoo machine, as testing with the -X option on your local computer may lead to an unexpected keyboard binding within the qemu vga monitor.***

On a physical zoo machine:
- Run `make` in the `mcertikos` directory to build
- Run `make qemu` to launch the QEMU virtual machine, which launches a VGA graphics window with resolution 640x480

## Testing

### Text Output
Upon launching `qemu`, you should see all terminal input/output shown in the console being replicated in the VGA graphics window. The text output is rendered pixel-by-pixel using a 8x8 bit font. We additionally support printing tabs, newlines, and backspace characters to better simulate the terminal experience. When the screen is full, we shift up existing text in order to make room for new text, just like a regular terminal.
![vga_terminal.png](screenshots%2Fvga_terminal.png)

### Graphics Programs
To demonstrate the functionality of our video mode project, we implemented two user programs which are called `pong` and `zoo`. Both `pong` and `zoo` can be launched from within our existing shell user program, which is started on boot. `pong` is a user program allowing a user to play multi-player pong. `zoo` is a user program allowing a user to display a number of images.

#### `pong`
In order to start the user program `pong` type "pong" in the shell user program. 
Now focus (click) on the QEMU VGA graphics window to register keyboard inputs.
##### Controls
- `a` - move player 1 paddle up
- `s` - move player 1 paddle down
- `k` - move player 2 paddle up 
- `l` - move player 2 paddle down 
- `x` - quit `pong`

![vga_pong.gif](screenshots%2Fvga_pong.gif)

#### `zoo`
In order to start the user program `zoo` type "zoo" in the shell user program. Now focus (click) on the QEMU VGA graphics window to register keyboard inputs. 

##### Controls 
- `→` - move to the next image 
- `←` - move to the previous image
- `x` - quit `zoo`

![vga_zoo.gif](screenshots%2Fvga_zoo.gif)

Note our `zoo` user program is only capable of storing the bitmap for six images and we have included a directory in the shell program (under `user/shell/images`) with other various images of the zoo nodes if you are curious how they look like within our video mode project. To do so, simply alter the images array declared globally in `zoo.c`.
To see arbitrary images in QEMU, here is a github repository that provides a python script for compressing images to 16-colors and outputting a colormap https://github.com/jackiedong6/bitmap-generator.git.

## Implementation

### Kernel



### Pong Program



### Zoo Program



### Notes
TODO:
- switch bootloader assembly code to 12H
# vga.c
- vga_set_pixel
- vga_plane_draw_byte
- vga_draw_chars 
- vga_draw_all_chars
- vga_clear
- vga_putc
- vga_set_rectangle
- vga_set_mode

sycalls
- sys_draw
- sys_setvideo
- sys_getc
- sys_draw_pixel
- 