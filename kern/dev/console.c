#include <lib/string.h>
#include <lib/types.h>
#include <lib/debug.h>
#include <lib/spinlock.h>

#include "video.h"
#include "console.h"
#include "serial.h"
#include "keyboard.h"

#define BUFLEN 1024
static char linebuf[BUFLEN];

spinlock_t console_readwrite_lock;
spinlock_t char_lock;
//int running_readline = 0;

struct {
    char buf[CONSOLE_BUFFER_SIZE];
    uint32_t rpos, wpos;
} cons;

void cons_init()
{
    memset(&cons, 0x0, sizeof(cons));
    serial_init();
    video_init();

    spinlock_init(&console_readwrite_lock);
    spinlock_init(&char_lock);
}

void cons_intr(int (*proc)(void))
{
    int c;
    while ((c = (*proc)()) != -1) {
        if (c == 0)
            continue;
        cons.buf[cons.wpos++] = c;
        if (cons.wpos == CONSOLE_BUFFER_SIZE)
            cons.wpos = 0;
    }
}

char cons_getc(void)
{
    int c;

    // poll for any pending input characters,
    // so that this function works even when interrupts are disabled
    // (e.g., when called from the kernel monitor).
    serial_intr();
    keyboard_intr();

    // grab the next character from the input buffer.
    if (cons.rpos != cons.wpos) {
        c = cons.buf[cons.rpos++];
        if (cons.rpos == CONSOLE_BUFFER_SIZE)
            cons.rpos = 0;
        return c;
    }
    return 0;
}

void cons_putc(char c)
{
    spinlock_acquire(&char_lock);
    serial_putc(c);
    video_putc(c);
    spinlock_release(&char_lock);
}

char getchar(void)
{
    char c;

    while ((c = cons_getc()) == 0)
        /* do nothing */ ;
    return c;
}

void putchar(char c)
{
    cons_putc(c);
}

char *readline(const char *prompt)
{
    int i;
    char c;

    spinlock_acquire(&console_readwrite_lock);
//    running_readline = 1;

    if (prompt != NULL) {
        spinlock_release(&console_readwrite_lock);
        dprintf("%s", prompt);
        spinlock_acquire(&console_readwrite_lock);
    }

    i = 0;
    while (1) {
        c = getchar();
        if (c < 0) {
            spinlock_release(&console_readwrite_lock);
            dprintf("read error: %e\n", c);
            spinlock_acquire(&console_readwrite_lock);

//            running_readline = 0;
            spinlock_release(&console_readwrite_lock);

            return NULL;
        } else if ((c == '\b' || c == '\x7f') && i > 0) {
            putchar('\b');
            i--;
        } else if (c >= ' ' && i < BUFLEN - 1) {
            putchar(c);
            linebuf[i++] = c;
        } else if (c == '\n' || c == '\r') {
            putchar('\n');
            linebuf[i] = 0;

//            running_readline = 0;
            spinlock_release(&console_readwrite_lock);

            return linebuf;
        }
    }
}
