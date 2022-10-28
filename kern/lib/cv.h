#ifndef _KERN_LIB_CV_H_
#define _KERN_LIB_CV_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include "spinlock.h"

typedef struct _cv_t cv_t;

void cv_init(cv_t *cv);
void cv_wait(cv_t *cv, spinlock_t *lk);
void cv_signal(cv_t *cv);
void cv_broadcast(cv_t *cv);

#endif  /* _KERN_ */

#endif  /* !_KERN_LIB_CV_H_ */
