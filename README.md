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

# Collaborators

- Tiger Wang (jw2723)
- Jackie Dong (jd2598)


# Project 1 Video Mode

# NOTE: To interact with our user programs please test locally on the zoo, as testing with the -X option on your local computer may lead to an unexpected keyboard binding within the qemu vga monitor.

# Running: Use `make qemu` to launch the QEMU virtual machine which will display our graphics


# User Programs
To demonstrate the functionality of our video mode project, we implemented two user programs which are called `pong` and `zoo`. Both `pong` and `zoo` are called within our shell user program. `pong` is a user program allowing a user to play multi-player pong. `zoo` is a user program allowing a user to display a number of images.

# `pong`
In order to start the user program `pong` type "pong" in the shell user program. 
Now focus (click) on the QEMU virtual machine to register keyboard inputs
## Controls
a - move player 1 paddle up
s - move player 2 paddle down
j - move player 2 paddle up 
k - move player 2 paddle down 
- x to quit `pong`


#`zoo`
In order to start the user program `zoo` type "zoo" in the shell user program. Now focus (click) on the QEMU virtual machine to register keyboard inputs. 
Note our `zoo` user program is only capable of storing the bitmap for six images and we have included a directory in the shell program named include with other various images of the zoo nodes if you are curious how they look like within our video mode project. To do so, simply alter the images array declared globally in zoo.c.
To see arbitrary images in QEMU, here is a github repository that provides a python script for compressing images to 16-colors and outputting a colormap https://github.com/jackiedong6/bitmap-generator.git. 
## Controls 
- → to move to the next image 
- ← to move to the previous image
- x to quit `zoo`


# Kernel Code

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