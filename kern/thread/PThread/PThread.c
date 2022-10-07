#include <lib/x86.h>
#include <lib/thread.h>

#include "import.h"

void thread_init(unsigned int mbi_addr)
{
    tqueue_init(mbi_addr);
    set_curid(0);
    tcb_set_state(0, TSTATE_RUN);
}

/**
 * Allocates a new child thread context, sets the state of the new child thread
 * to ready, and pushes it to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
    unsigned int child_id = kctx_new(entry, id, quota);
    if (child_id == NUM_IDS) {
        return NUM_IDS;
    }
    tcb_set_state(child_id, TSTATE_READY);
    tqueue_enqueue(NUM_IDS, child_id);
    return child_id;
}

/**
 * Yield to the next thread in the ready queue.
 * You should set the currently running thread state as ready,
 * and push it back to the ready queue.
 * Then set the state of the popped thread as running, set the
 * current thread id, and switch to the new kernel context.
 * Hint: If you are the only thread that is ready to run,
 * do you need to switch to yourself?
 */
void thread_yield(void)
{
    unsigned int next_ready = tqueue_dequeue(NUM_IDS);
    if (next_ready == NUM_IDS) {
        // The ready queue is empty, no need to switch to self
        return;
    }

    // Add current thread back to ready queue
    unsigned int current_id = get_curid();
    tcb_set_state(current_id, TSTATE_READY);
    tqueue_enqueue(NUM_IDS, current_id);

    // Switch to the popped thread
    tcb_set_state(next_ready, TSTATE_RUN);
    set_curid(next_ready);
    kctx_switch(current_id, next_ready);
}
