#include "bounded_buffer.h"

void bbq_init(bounded_buffer_t *bbq)
{
    bbq->front = 0;
    bbq->next_empty = 0;
    spinlock_init(&bbq->lock);
    cv_init(&bbq->item_added);
    cv_init(&bbq->item_removed);
}

// inserts an item with value into the bounded buffer queue
void bbq_insert(bounded_buffer_t *bbq, unsigned int value)
{
    spinlock_acquire(&bbq->lock);
    while ((bbq->next_empty - bbq->front) == BUFFER_SIZE)
    {
        cv_wait(&bbq->item_removed, &bbq->lock);
    }

    bbq->items[bbq->next_empty % BUFFER_SIZE] = value;
    bbq->next_empty++;
    cv_signal(&bbq->item_added);
    spinlock_release(&bbq->lock);
}

// removes an item from the bounded buffer queue and returns it
unsigned int bbq_remove(bounded_buffer_t *bbq)
{
    spinlock_acquire(&bbq->lock);
    while (bbq->front == bbq->next_empty)
    {
        cv_wait(&bbq->item_added, &bbq->lock);
    }
    unsigned int item = bbq->items[bbq->front % BUFFER_SIZE];
    bbq->front++;
    cv_signal(&bbq->item_removed);
    spinlock_release(&bbq->lock);
    return item;
}
