#include "cv.h"
#include "thread/PCurID/export.h"
#include "thread/PThread/export.h"
#include <dev/intr.h>
#include <lib/debug.h>

void cv_init(cv_t *cv) {
    cv->front = 0;
    cv->next_empty = 0;
}

void cv_wait(cv_t *cv, spinlock_t *lk) {
    // Add to waiting queue
    unsigned int cur_pid = get_curid();
    cv->waiting[cv->next_empty % NUM_IDS] = cur_pid;
    cv->next_empty++;

    intr_local_disable();
    KERN_DEBUG("Added process %d to CV waiting queue\n", cur_pid);
    intr_local_enable();

    spinlock_release(lk);
    thread_suspend();
    spinlock_acquire(lk);
}

void cv_signal(cv_t *cv) {
    if (cv->front != cv->next_empty) {
        unsigned int wake_pid = cv->waiting[cv->front % NUM_IDS];
        cv->front++;

        intr_local_disable();
        KERN_DEBUG("Signaling process %d to be ready\n", wake_pid);
        intr_local_enable();

        thread_make_ready(wake_pid);
    }
}

void cv_broadcast(cv_t *cv) {
    while (cv->front != cv->next_empty) {
        unsigned int wake_pid = cv->waiting[cv->front % NUM_IDS];
        cv->front++;

        intr_local_disable();
        KERN_DEBUG("Broadcasting process %d to be ready\n", wake_pid);
        intr_local_enable();

        thread_make_ready(wake_pid);
    }
}
