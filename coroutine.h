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

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

typedef void (*co_func)(void *);

int co_create(co_func, void *);
void co_enter(int id);
void co_yield();
void co_destroy(int id);

int co_status(int id);
int co_running(void);

#endif
