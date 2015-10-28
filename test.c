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
#include <stdio.h>

#define CO_SIZE 128

struct arguments {
    int n;
};

static void foo(void *user_data) {
    struct arguments * args = user_data;
    int n = args->n;
    int i;
    for (i = 0; i < n; i++) {
        printf("coroutine %d : %d\n", co_running(), i);
        co_yield();
    }
}

int main() {
    struct arguments args = { 2 };
    int co_ids[CO_SIZE];
    int i;
    int finished = 0;
    for (i = 0; i < CO_SIZE; i++)
        co_ids[i] = co_create(foo, &args);
    printf("main start\n");
    while (finished < CO_SIZE)
        for (i = 0; i < CO_SIZE; i++)
            if (co_status(co_ids[i]))
                co_enter(co_ids[i]);
            else {
                co_destroy(co_ids[i]);
                finished++;
            }
    printf("main end\n");
    return 0;
}
