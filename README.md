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
In order to switch `qemu` from video mode to graphic mode, we first had to change the bootloader code in `boot0.S.` We did so by changing the argument passed in to BIOS's software interrupt from $0x03 to $0x12 which specifies VGA 640x480 16 color mode.  

After enabling graphic mode, we then implemented the functionality to draw text onto the screen to mimic terminal behavior. In `kern/dev/vga.c` we implemented the functionality for drawing characters onto the screen using a 8x8 bitmap font-header (`vga_draw_chars`), drawing a rectangle on the screen(`vga_set_rectangle`), and also drawing a single pixel on the screen (`vga_set_pixel`). For each of these implementaitons, since video mode 12 has a four planar layout, in order to draw output on the screen, we needed to write data to four specific port-mapped devices representing the four color planes. To do this, we used the outw command to specify the port as well as the index within the port to get data from. Drawing pixel by pixel was slightly challenging as video mode only supoprts writing a byte of memory at a time which is 8 pixels. To address this nuance, when drawing pixel by pixel in order to avoid overwriting data in the neighboring pixels, we had to first issue a read from each of the four planes. 
To allow users to turn on and off "video" mode we implemented `vga_set_mode` which through `sys_setvideo` allows the user to either clear the VGA graphics window or render the buffer of text from the terminal. We also chose to expose drawing pixels and rectangles to the user and thus implemented the system call interfaces `sys_drawpixel` and `sys_draw`. 

We also implemented a new system call, `sys_getkey`. This system call returns output directly from keyboard I/O and allowed us to circumvent using cons_getc. Ultimately, this enabled multi-player support for Pong (both player 1 and player 2 can move their paddles simultaneously) and also provided finer grain control over the paddles. 

### Pong Program
In the `Pong` user program, we first toggled VGA video mode and set up the environment for pong. To do so, we used our `sys_draw` system call to draw both player1 and player2 paddles, the pong ball, and also goal lines. Afterwards, to provide support for smooth control of the paddle we used our `sys_getkey` to register user input. In order to support the logic for the ball bouncing at various angles depending on incident of collision, we needed a way to compute the sine and cosine of a given angle. Given that we could not directly import the math library, we found a library called fdlibm online with both functions and then compiled it to a static library file labeled `libm.a` and included it within our user program directory. To ensure the best user experience possible, we implemented a rendering logic similar to React's archictecture, only rerendering certain objects when necessary. Furthemore, we also spent an apt amount of time tweaking certain parameters of our game such as grace perio upon scoring and ball speed to better the user experience.


### Zoo Program
In the `Zoo` user program, we defined a global array of images with each image being loaded from the `user/pong/images` folder. We then toggled VGA video mode and used our system call `sys_draw_pixel` to render specified images pixel by pixel. Then, using our `sys_getkey` system call, we capture keyboard input to allow users to switch back and forth between images (see controls in testing section above).


### Notes
