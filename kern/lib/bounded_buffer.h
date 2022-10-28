#ifndef _KERN_LIB_BOUNDED_BUFFER_H_
#define _KERN_LIB_BOUNDED_BUFFER_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include "spinlock.h"

typedef struct _bounded_buffer_t bounded_buffer_t;

void bbq_init(bounded_buffer_t *bbq);
void bbq_insert(bounded_buffer_t *bbq, unsigned int value);
unsigned int bbq_remove(bounded_buffer_t *bbq);

#endif  /* _KERN_ */

#endif  /* !_KERN_LIB_BOUNDED_BUFFER_H_ */
