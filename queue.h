#ifndef TPOOL_QUEUE_H
#define TPOOL_QUEUE_H


#include <stdlib.h>
#include <stdbool.h>

typedef struct tpool_list {
    size_t alloc_size;
    size_t count;
    void **data;
} tpool_list;

typedef struct tpool_queue tpool_queue;

void tpool_list_init(tpool_list *list);

void tpool_list_free(tpool_list *list);

void tpool_list_push(tpool_list *list, void *item);

void *tpool_list_pop(tpool_list *list);

tpool_queue *tpool_queue_init();

void tpool_queue_free(tpool_queue *queue);

void tpool_enqueue(tpool_queue *queue, void *item);

void tpool_queue_unblock(tpool_queue *queue);

void tpool_queue_wait(tpool_queue *queue);

void *tpool_dequeue(tpool_queue *queue);

#endif