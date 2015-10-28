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
#include <ucontext.h>
#include <sys/mman.h>

#define NULL ((void *)0)
#define STACK_SIZE 8*1024*1024  //8 MB -- same as Linux Thread Stack Size
#define COROUTINES 256  //COROUTINES * STACK_SIZE < The size of HEAP SPACE

struct coroutine {
    int status;
    co_func func;
    void *args;
    ucontext_t ctx;
    void *stack;
};

struct coroutine coros[COROUTINES];
int running = -1;  //current running coroutine id
int co_id = 0;  //for allocating coroutine id
int co_count = 0;  //the number of coroutines

ucontext_t main_ctx;

// allocating & initializing coroutine struct, return the coroutine id,
// or -1 if reaching the limitation of coroutines
int co_create(co_func func, void *args) {
    if (co_count == COROUTINES)
        return -1;
    co_count++;

    //get an available co_id
    while (coros[co_id].status)
        co_id = (co_id + 1 ) % COROUTINES;

    //initial the coroutine struct
    coros[co_id].func = func;
    coros[co_id].args = args;
    coros[co_id].status = COROUTINE_READY;
    return co_id;
}

static void _co_wrapper() {
    coros[running].func(coros[running].args);
    coros[running].status = COROUTINE_DEAD;
    //co_destory(running);
    running = -1;
}

// setting up the context for a 'ready' coroutine including stack allocation,
// & swapping context for a 'ready' or 'suspended' coroutine
void co_enter(int id) {
    ucontext_t *ctx;
    if ((running != -1) || !(id >= 0 && id < COROUTINES))
        return;
    if (!coros[id].status)
        return;
    switch(coros[id].status) {
        case COROUTINE_READY:
            ctx = &(coros[id].ctx);
            getcontext(ctx);
            coros[id].stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
            if (coros[id].stack == MAP_FAILED)
                return;
            ctx->uc_stack.ss_sp = coros[id].stack;
            ctx->uc_stack.ss_size = STACK_SIZE;
            ctx->uc_link = &main_ctx;
            makecontext(ctx, (void (*)(void)) _co_wrapper, 0);
            running = id;
            coros[id].status = COROUTINE_RUNNING;
            swapcontext(&main_ctx, ctx);
            break;
        case COROUTINE_SUSPEND:
            running = id;
            coros[id].status = COROUTINE_RUNNING;
            swapcontext(&main_ctx, &(coros[id].ctx));
            break;
        default:
            break;
    }
}

// yielding the execution & return to the main routine
void co_yield() {
    int id = running;
    if (!(id >= 0 && id < COROUTINES))
        return;
    coros[id].status = COROUTINE_SUSPEND;
    running = -1;
    swapcontext(&(coros[id].ctx), &main_ctx);
}

// deleting the coroutine id's struct & the stack allocating for it
void co_destroy(id) {
    if (!(id >= 0 && id < COROUTINES))
        return;
    if (!coros[id].status) {
        munmap(coros[id].stack, STACK_SIZE);
        co_count--;
    }
}

int co_status(int id) {
    if (!(id >= 0 && id < COROUTINES))
        return -1;
    return coros[id].status;
}

int co_running() {
    return running;
}
