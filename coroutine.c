//
//coroutine.c
//
//Copyright (c) 2015 Weichao Guo (guoweichao at mail dot com)
//
//Distributed under the terms of the GNU General Public License as published by
//the Free Software Foundation; under version 2 of the License, you may obtain
//a copy of the license at http://www.gnu.org/licenses/.
//

#include "coroutine.h"

#include <sys/mman.h>
#include <pthread.h>

#define NULL ((void *)0)
#define STACK_SIZE (8 * 1024 * 1024)  //8 MB -- same as Linux Thread Stack Size

//specific enqueue & dequeue operations for struct co_queue
static void _enqueue(struct co_queue *q, struct coroutine *co) {
    if (q->front == NULL && q->rear == NULL)
        q->front = q->rear = co;
    else {
        q->rear->next = co;
        q->rear = co;
    }
}
void _dequeue(struct co_queue *q) {
    struct coroutine *co = q->front;
    if (q->front == NULL)
        return;
    else if (q->front == q->rear)
        q->front = q->rear = NULL;
    else
        q->front = q->front->next;
    co->next = NULL;
}

// allocating & initializing coroutine struct, return the coroutine struct,
// if allocating stack failed, coroutine status will be COROUTINE_DEAD
struct coroutine co_create(co_func func, void *args) {
    struct coroutine co;
    co.func = func;
    co.args = args;
    co.status = COROUTINE_READY;
    co.stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (co.stack == MAP_FAILED)
        co.status = COROUTINE_DEAD;
    co.sched = NULL;
    co.mutex = NULL;
    co.countdown = 0;
    return co;
}

//obtain the coroutine struct pointer from the specific address & 
//maintain the coroutine status
static void _co_wrapper() {
    char dummy;
    struct coroutine *co = *((struct coroutine **)(&dummy + 33 - STACK_SIZE));
    co->func(co, co->args);
    co->status = COROUTINE_DEAD;
}

// setting up the context for a 'ready' coroutine,
// & swapping context for a 'ready' or 'suspended' coroutine
void co_enter(struct coroutine *co) {
    switch(co->status) {
        case COROUTINE_READY:
            getcontext(&(co->ctx));
            co->ctx.uc_stack.ss_sp = co->stack;
            co->ctx.uc_stack.ss_size = STACK_SIZE;
            co->ctx.uc_link = &(co->sched->main_ctx);
            makecontext(&(co->ctx), (void (*)(void)) _co_wrapper, 0);
        case COROUTINE_SUSPEND:
            *((struct coroutine **)(co->stack)) = co;
            co->status = COROUTINE_RUNNING;
            swapcontext(&(co->sched->main_ctx), &(co->ctx));
            break;
        default:
            break;
    }
}

// yielding the execution & return to the main routine
void co_yield(struct coroutine *co) {
    co->status = COROUTINE_SUSPEND;
    _enqueue(&(co->sched->suspend), co);
    swapcontext(&(co->ctx), &(co->sched->main_ctx));
}

// deleting the coroutine stack allocating for it
void co_destroy(struct coroutine *co) {
    if (co->status == COROUTINE_DEAD && co->stack != NULL) {
        munmap(co->stack, STACK_SIZE);
        co->stack = NULL;
    }
}

//attach a coroutine to a schedule
void co_attach(struct schedule *sched, struct coroutine *co) {
    co->sched = sched;
    _enqueue(&(sched->suspend), co);
}

//schedule lock queue first, for a coroutine, 
//if lock success, enter this coroutine.
//then schedule sleep queue, for a coroutine, if the countdown reaches 0, 
//put the coroutine back to the suspend queue.
//then schedule suspend queue
void co_schedule(struct schedule *sched) {
    struct coroutine *co;
    struct coroutine *next;
    struct co_queue checked = {NULL, NULL};
    while (sched->suspend.front || sched->lock.front || sched->sleep.front) {
        if (sched->lock.front) {
            co = sched->lock.front;
            while (co) {
                next = co->next;
                _dequeue(&(sched->lock));
                if (pthread_mutex_trylock(co->mutex) == 0) {
                    co->status = COROUTINE_SUSPEND;
                    co_enter(co);
                }
                else
                    _enqueue(&checked, co);
                co = next;
            }
            sched->lock = checked;
            checked.front = NULL;
            checked.rear = NULL;
        }
        if (sched->sleep.front) {
            co = sched->sleep.front;
            while (co) {
                next = co->next;
                _dequeue(&(sched->sleep));
                if ((co->countdown--) == 0) {
                    co->status = COROUTINE_SUSPEND;
                    _enqueue(&(sched->suspend), co);
                }
                else
                    _enqueue(&checked, co);
                co = next;
            }
            sched->sleep = checked;
            checked.front = NULL;
            checked.rear = NULL;
        }
        if (sched->suspend.front) {
            co = sched->suspend.front;
            _dequeue(&(sched->suspend));
            if (co->status == COROUTINE_DEAD)
                co_destroy(co);
            else
                co_enter(co);
        }
    }
}

//put the coroutine to the lock queue & switch back to main context
void co_lock(struct coroutine *co, pthread_mutex_t *mutex) {
    co->mutex = mutex;
    co->status = COROUTINE_LOCK;
    _enqueue(&(co->sched->lock), co);
    swapcontext(&(co->ctx), &(co->sched->main_ctx));
}

int co_unlock(struct coroutine *co, pthread_mutex_t *mutex) {
    return pthread_mutex_unlock(mutex);
}

//put the coroutine to the sleep queue & sleep 'round' schedules
void co_sleep(struct coroutine *co, int round) {
    if (round <= 0)
        return;
    co->countdown = round;
    co->status = COROUTINE_SLEEP;
    _enqueue(&(co->sched->sleep), co);
    swapcontext(&(co->ctx), &(co->sched->main_ctx));
}

