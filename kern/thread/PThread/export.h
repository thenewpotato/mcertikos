#ifndef _KERN_THREAD_PTHREAD_H_
#define _KERN_THREAD_PTHREAD_H_

#ifdef _KERN_

#include <lib/spinlock.h>

void thread_init(unsigned int mbi_addr);
unsigned int thread_spawn(void *entry, unsigned int id,
                          unsigned int quota);
void thread_suspend(spinlock_t *cv_lock, unsigned int old_pid);
void thread_make_ready(unsigned int pid);
void thread_yield(void);
void sched_update(void);

#endif /* _KERN_ */

#endif /* !_KERN_THREAD_PTHREAD_H_ */
