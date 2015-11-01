//
//coroutine.h
//
//Copyright (c) 2015 Weichao Guo (guoweichao at mail dot com)
//
//Distributed under the terms of the GNU General Public License as published by
//the Free Software Foundation; under version 2 of the License, you may obtain
//a copy of the license at http://www.gnu.org/licenses/.
//

#ifndef C_COROUTINE_H
#define C_COROUTINE_H

#include <ucontext.h>

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

#define COROUTINE_LOCK 4
#define COROUTINE_SLEEP 5

struct coroutine;

typedef void (*co_func)(struct coroutine *, void *);

struct coroutine {
    int status;
    co_func func;
    void *args;
    ucontext_t ctx;
    void *stack;
    struct schedule *sched;
    pthread_mutex_t *mutex;
    int countdown;
    struct coroutine *next;
};

struct co_queue {
    struct coroutine *front;
    struct coroutine *rear;
};

struct schedule {
    struct co_queue suspend;
    struct co_queue lock;
    struct co_queue sleep;
    ucontext_t main_ctx;
};

struct coroutine co_create(co_func, void *);
void co_enter(struct coroutine *);
void co_yield(struct coroutine *);
void co_destroy(struct coroutine *);

void co_schedule(struct schedule *);
void co_attach(struct schedule *, struct coroutine *);

void co_lock(struct coroutine *, pthread_mutex_t *);
int co_unlock(struct coroutine *, pthread_mutex_t *);

void co_sleep(struct coroutine *, int);

#endif
