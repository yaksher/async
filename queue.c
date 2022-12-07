#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "queue.h"

const size_t TPOOL_MIN_LIST_SIZE = 4;
const size_t TPOOL_LIST_SCALE = 2;

static void *lock_get(pthread_mutex_t *mutex, void **loc) {
    pthread_mutex_lock(mutex);
    void *item = *loc;
    pthread_mutex_unlock(mutex);
    return item;
}


void tpool_list_init(tpool_list *list) {
    list->alloc_size = 0;
    list->count = 0;
    list->data = NULL;
}

void tpool_list_free(tpool_list *list) {
    if (list->data != NULL) {
        free(list->data);
    }
}

void tpool_list_push(tpool_list *list, void *item) {
    if (list->data == NULL) {
        list->data = malloc(TPOOL_MIN_LIST_SIZE * sizeof(void *));
        assert(list->data != NULL && "Allocation failed in tpool_list_push");
        list->alloc_size = TPOOL_MIN_LIST_SIZE;
    }
    else if (list->count == list->alloc_size) {
        list->alloc_size *= TPOOL_LIST_SCALE;
        list->data = realloc(list->data, list->alloc_size * sizeof(void *));
        assert(list->data != NULL && "Allocation failed in tpool_list_push");
    }
    list->data[list->count++] = item;
}

void *tpool_list_pop(tpool_list *list) {
    return list->count > 0 ? list->data[--list->count] : NULL;
}

tpool_queue *tpool_queue_init() {
    tpool_queue *queue = malloc(sizeof(tpool_queue));
    assert(!pthread_mutex_init(&queue->body_mutex, NULL));

    tpool_list_init(&queue->in);
    tpool_list_init(&queue->out);
    queue->count = 0;
    queue->unblock = false;

    assert(!pthread_mutex_init(&queue->new_mut, NULL));
    assert(!pthread_cond_init(&queue->new_cond, NULL));

    return queue;
}

void tpool_queue_free(tpool_queue *queue) {
    tpool_list_free(&queue->in);
    tpool_list_free(&queue->out);
    free(queue);
}

void tpool_enqueue(tpool_queue *queue, void *item) {
    pthread_mutex_lock(&queue->body_mutex);

    tpool_list_push(&queue->in, item);
    queue->count++;

    pthread_mutex_unlock(&queue->body_mutex);

    pthread_mutex_lock(&queue->new_mut);
    pthread_cond_signal(&queue->new_cond);
    pthread_mutex_unlock(&queue->new_mut);
}

void tpool_queue_unblock(tpool_queue *queue) {
    pthread_mutex_lock(&queue->new_mut);
    queue->unblock = true;
    pthread_cond_broadcast(&queue->new_cond);
    pthread_mutex_unlock(&queue->new_mut);
}

void tpool_queue_wait(tpool_queue *queue) {
    pthread_mutex_lock(&queue->new_mut);
    while (!queue->unblock && (size_t) lock_get(&queue->body_mutex, (void **) &queue->count) == 0) {
        pthread_cond_wait(&queue->new_cond, &queue->new_mut);
    }
    pthread_mutex_unlock(&queue->new_mut);
}

void *tpool_dequeue(tpool_queue *queue) {
    tpool_queue_wait(queue);

    void *ret;
    pthread_mutex_lock(&queue->body_mutex);
    // if the out-list is empty, transfer the contents of in to out
    if (queue->out.count == 0 && queue->in.count > 0) {
        for (
            void *item = tpool_list_pop(&queue->in);
            item != NULL;
            item = tpool_list_pop(&queue->in)
        ) {
            tpool_list_push(&queue->out, item);
        }
    }
    // if the out list has something, pop it
    if (queue->out.count > 0) {
        ret = tpool_list_pop(&queue->out);
        queue->count--;
    } else {
        ret = NULL;
    }
    pthread_mutex_unlock(&queue->body_mutex);
    return ret;
}