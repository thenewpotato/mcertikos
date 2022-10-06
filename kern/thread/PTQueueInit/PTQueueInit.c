#include "lib/x86.h"

#include "import.h"

/**
 * Initializes all the thread queues with tqueue_init_at_id.
 */
void tqueue_init(unsigned int mbi_addr)
{
    tcb_init(mbi_addr);

    for (unsigned int i = 0; i < NUM_IDS + 1; i++) {
        tqueue_init_at_id(i);
    }
}

/**
 * Insert the TCB #pid into the tail of the thread queue #chid.
 * Recall that the doubly linked list is index based.
 * So you only need to insert the index.
 * Hint: there are multiple cases in this function.
 */
void tqueue_enqueue(unsigned int chid, unsigned int pid)
{
    unsigned int old_tail = tqueue_get_tail(chid);
    if (old_tail == NUM_IDS) {
        // Queue is empty
        tqueue_set_head(chid, pid);
        tqueue_set_tail(chid, pid);
        // Ensure current tail points to NULL in both directions
        tcb_set_next(pid, NUM_IDS);
        tcb_set_prev(pid, NUM_IDS);
    } else {
        // Queue is not empty
        tqueue_set_tail(chid, pid);
        // Connect old and current tails
        tcb_set_prev(pid, old_tail);
        tcb_set_next(old_tail, pid);
        // Ensure current tail points to NULL
        tcb_set_next(pid, NUM_IDS);
    }
}

/**
 * Reverse action of tqueue_enqueue, i.e. pops a TCB from the head of the specified queue.
 * It returns the popped thread's id, or NUM_IDS if the queue is empty.
 * Hint: there are multiple cases in this function.
 */
unsigned int tqueue_dequeue(unsigned int chid)
{
    unsigned int old_head = tqueue_get_head(chid);
    if (old_head == NUM_IDS) {
        // Queue is empty, cannot dequeue
        return NUM_IDS;
    } 
    unsigned int new_head = tcb_get_next(old_head);
    if (new_head == NUM_IDS) {
        // Queue contains 1 element
        tqueue_init_at_id(chid);
        // To be safe...
        tcb_set_prev(old_head, NUM_IDS);
        tcb_set_next(old_head, NUM_IDS);
        return old_head;
    } else {
        // Queue contains multiple elements
        tqueue_set_head(chid, new_head);
        tcb_set_prev(new_head, NUM_IDS);
        // To be safe...
        tcb_set_prev(old_head, NUM_IDS);
        tcb_set_next(old_head, NUM_IDS);
        return old_head;
    }
}

/**
 * Removes the TCB #pid from the queue #chid.
 * Hint: there are many cases in this function.
 */
void tqueue_remove(unsigned int chid, unsigned int pid)
{
    // TODO: is it guaranteed that pid is in the queue?
    if (tqueue_get_head(chid) == pid) {
        // Need to remove the only element OR Need to remove at head
        tqueue_dequeue(chid);
    } else if (tqueue_get_tail(chid) == pid) {
        // Need to remove at tail (more than 1 element)
        unsigned int new_tail = tcb_get_prev(pid);
        tqueue_set_tail(chid, new_tail);
        tcb_set_next(new_tail, NUM_IDS);
        // To be safe...
        tcb_set_next(pid, NUM_IDS);
        tcb_set_prev(pid, NUM_IDS);
    } else {
        // Need to remove in the middle (more than 1 element)
        unsigned int prev = tcb_get_prev(pid);
        unsigned int next = tcb_get_next(pid);
        tcb_set_next(prev, next);
        tcb_set_prev(next, prev);
        // To be safe...
        tcb_set_next(pid, NUM_IDS);
        tcb_set_prev(pid, NUM_IDS);
    }
}
