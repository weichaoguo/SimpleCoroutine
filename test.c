//
//test.c
//
//Copyright (c) 2015 Weichao Guo (guoweichao at mail dot com)
//
//Distributed under the terms of the GNU General Public License as published by
//the Free Software Foundation; under version 2 of the License, you may obtain
//a copy of the license at http://www.gnu.org/licenses/.
//

#include "coroutine.h"

#include <pthread.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#define COROUTINES (32 * 1024)
#define ITERATION 4
#define THREADS 2

struct arguments {
    int cid;
    int tid;
    int lock;
    int sleep;
};

pthread_mutex_t mutex;

static void foo(struct coroutine *co, void *user_data) {
    struct arguments * args = user_data;
    int i;
    int r;
    int a[COROUTINES];
    memset(a, 0, COROUTINES * sizeof(int));
    srand(time(0));
    for (i = 0; i < ITERATION; i++) {
        a[args->cid] = args->tid;
        //printf("%u: %d\n", args->tid, args->cid);
        r = rand() % 100;
        if (r < args->sleep)
            co_sleep(co, rand() % 10);
        else if (r < args->sleep + args->lock) {
            co_lock(co, &mutex);
            co_unlock(co, &mutex);
        }
        else
            co_yield(co);
    }
}

void test1(int n, int id, int sleep, int lock) {
    int i;
    struct arguments *args = malloc(n * sizeof(struct arguments));
    struct coroutine *coro = malloc(n * sizeof(struct coroutine));
    struct schedule *sched = calloc(1, sizeof(struct schedule));
    for (i = 0; i < n; i++) {
        args[i].cid = i;
        args[i].tid = id;
        args[i].sleep = sleep;
        args[i].lock = lock;
        coro[i] = co_create(foo, args + i);
        co_attach(sched, coro + i);
    }
    co_schedule(sched);
}

void *test2(void * args) {
    test1(COROUTINES, pthread_self(), 0, (long)args);
    return NULL;
}

void *test3(void * args) {
    test1(COROUTINES, pthread_self(), (long)args, 25);
    return NULL;
}

int main() {
    long eval1[9];
    long eval2[9] = {5, 10, 15, 20, 25, 30, 35, 40, 45};
    long eval3[9] = {5, 10, 15, 20, 25, 30, 35, 40, 45};
    double results1[9], results2[9], results3[9];
    pthread_t threads[THREADS];
    int i, c;
    struct timeval t;
    double start, end;
    for (c = 0; c < 9; c++)
        eval1[c] = 128<<c;
    printf("TEST STAGE 1 ...\n");
    for (c = 0; c < 9; c++) {
        gettimeofday(&t, 0);
        start = 1000000 * t.tv_sec + t.tv_usec;
        start /= 1000000;
        test1(eval1[c], 12345, 0, 0);
        gettimeofday(&t, 0);
        end = 1000000 * t.tv_sec + t.tv_usec;
        end /= 1000000;
        results1[c] = end - start;
    }

    pthread_mutex_init(&mutex, NULL);
    for (c = 0; c < 9; c++)
        printf("%lf ", results1[c]);
    printf("\n");

    printf("TEST STAGE 2 ...\n");
    for (c = 0; c < 9; c++) {
        gettimeofday(&t, 0);
        start = 1000000 * t.tv_sec + t.tv_usec;
        start /= 1000000;
        for (i = 0; i < THREADS; i++)
            if (pthread_create(&threads[i], NULL, test2, (void *)eval2[c]))
                printf("pthread_create failed!\n");
        for (i = 0; i < THREADS; i++)
            pthread_join(threads[i], NULL);
        gettimeofday(&t, 0);
        end = 1000000 * t.tv_sec + t.tv_usec;
        end /= 1000000;
        results2[c] = end - start;
    }
    for (c = 0; c < 9; c++)
        printf("%lf ", results2[c]);
    printf("\n");

    printf("TEST STAGE 3 ...\n");
    for (c = 0; c < 9; c++) {
        gettimeofday(&t, 0);
        start = 1000000 * t.tv_sec + t.tv_usec;
        start /= 1000000;
        for (i = 0; i < THREADS; i++)
            if (pthread_create(&threads[i], NULL, test3, (void *)eval3[c]))
                printf("pthread_create failed!\n");
        for (i = 0; i < THREADS; i++)
            pthread_join(threads[i], NULL);
        gettimeofday(&t, 0);
        end = 1000000 * t.tv_sec + t.tv_usec;
        end /= 1000000;
        results3[c] = end - start;
    }
    for (c = 0; c < 9; c++)
        printf("%lf ", results3[c]);
    printf("\n");
    return 0;
}
