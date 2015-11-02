SimpleCoroutine
===============

This is a simple coroutine library implementation in C.

Introduction
------------

The implementation is based on __glibc ucontext__ instead of __setjmp__ for stackfulness although __setjmp__ can provide a better context switch performance. For different platforms, you can try to customize a specfic __ucontext__ implementation for fast _swapcontext_ which is comparative to _setjmp_ & _longjmp_. SimpleCoroutine is especially suitable for those systems that have huge logical address space like x86_64 platform as there is no need to do _memcpy_ staffs for saving coroutine stack.

API
---

- struct coroutine __co_create__ (cofunc func, void *args)
  - creating coroutine struct with given routine
- void __co_attach__ (struct schedule *sched, struct coroutine *co)
  - attaching a coroutine to a schedule
- void __co_schedule__ (struct schedule *sched)
  - scheduling a batch of coroutines
- void __co_lock__ (struct coroutine *co, pthread_mutex_lock *mutex)
  - getting a pthread mutex lock asynchronously
- void __co_unlock__ (struct coroutine *co, pthread_mutex_lock *mutex)
  - releasing the lock
- void __co_sleep__ (struct coroutine *co, int round)
  - sleeping for _round_ schedules. 

Example
-------

```bash
struct arguments {
    int n;
    int cid;
};
static void foo(struct coroutine *co, void *user_data) {
    arguments *args = user_data;
    int i;
    for (i = 0; i < args->n; i++) {
        printf("%d: %d\n", args->cid, i);
        co_yield(co);
    }
}
struct schedule sched;
struct coroutine co;
co = co_create(foo, args);
co_attach(&sched, &co);
co_schedule(&sched);
```

You can learn more examples from 'test.c'.

Evaluation
----------

There are some code in 'test.c' for evaluation. Results has shown the efficiency of the coroutine schedule under varies of lock rate, sleep rate, and thread number. 

We run the evaluation on a Dell R720 with Ubuntu 12.04 LTS (Linux 3.16 x86_64). Comparing with [a similar coroutine implementation of cloudwu](https://github.com/cloudwu/coroutine/), SimpleCoroutine shows its 'no memory copy' advantage when spawning a large mount of coroutines.

__Time Cost of different coroutines (in Seconds)__

| Coroutines | SimpleCoroutine | cloudwu's coroutine |
|------------|-----------------|---------------------|
| 1024       | 0.128012        | 0.269253            |
| 2048       | 0.255682        | 0.274202            |
| 4096       | 0.250057        | 0.547564            |
| 8192       | 0.435186        | 1.120908            |
| 16384      | 0.874813        | 2.227877            |
| 32768      | 1.77499         | 4.415931            |

For 32768 coroutines per thread, SimpleCoroutine's schedule show its efficiency under different lock rate, sleep rate, and thread number.

__Time Cost of 32768 coroutines under different lock rate and threads (in Seconds)__

| Lock Rate | 2 Threads | 4 Threads |
|-----------|-----------|-----------|
| 5%         | 2.611119  | 3.169691  |
| 10%        | 2.398041  | 3.13245   |
| 15%        | 2.456234  | 2.675862  |
| 20%        | 2.647638  | 2.69208   |
| 25%        | 2.396058  | 2.594353  |
| 30%        | 2.489139  | 2.641342  |
| 35%        | 2.529191  | 2.587147  |
| 40%        | 2.238608  | 2.57414   |
| 45%        | 2.155451  | 2.639133  |

__Time Cost of 32768 coroutines with a 25% lock rate under differnt sleep rate and threads (in Seconds)__

| Sleep Rate | 2 Threads | 4 Threads |
|------------|-----------|-----------|
| 5%          | 2.535662  | 3.227598  |
| 10%         | 2.360012  | 2.712066  |
| 15%         | 2.351964  | 2.626561  |
| 20%         | 2.125586  | 2.635053  |
| 25%         | 2.357346  | 2.69312   |
| 30%         | 2.285894  | 2.6724    |
| 35%         | 2.361594  | 2.874758  |
| 40%         | 2.400527  | 2.977598  |
| 45%         | 2.563666  | 2.689279  |
