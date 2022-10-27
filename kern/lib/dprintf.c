#ifdef DEBUG_MSG

#include <dev/console.h>
#include <dev/serial.h>

#include <lib/debug.h>
#include <lib/stdarg.h>
#include <lib/spinlock.h>

extern spinlock_t console_readwrite_lock;
extern int running_readline;

struct dprintbuf {
    int idx;  /* current buffer index */
    int cnt;  /* total bytes printed so far */
    char buf[CONSOLE_BUFFER_SIZE];
};

// Writing a string to console
static void cputs(const char *str)
{
    while (*str) {
        cons_putc(*str);
        str += 1;
    }
}

static void putch(int ch, struct dprintbuf *b)
{
    b->buf[b->idx++] = ch;
    if (b->idx == CONSOLE_BUFFER_SIZE - 1) {
        b->buf[b->idx] = 0;
        cputs(b->buf);
        b->idx = 0;
    }
    b->cnt++;
}

int vdprintf(const char *fmt, va_list ap)
{
    struct dprintbuf b;

    if (running_readline == 0) {
        // If running readline, we already have the lock
        spinlock_acquire(&console_readwrite_lock);
    }

    b.idx = 0;
    b.cnt = 0;
    vprintfmt((void *) putch, &b, fmt, ap);

    b.buf[b.idx] = 0;
    cputs(b.buf);

    if (running_readline == 0) {
        spinlock_release(&console_readwrite_lock);
    }

    return b.cnt;
}

int dprintf(const char *fmt, ...)
{
    va_list ap;
    int cnt;

    va_start(ap, fmt);
    cnt = vdprintf(fmt, ap);
    va_end(ap);

    return cnt;
}

#endif  /* DEBUG_MSG */
