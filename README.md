SimpleCoroutine
===============

This is a simple coroutine library implementation in C.

Introduction
------------

The implementation is based on __glibc ucontext__ instead of __setjmp__ for stackfulness although __setjmp__ can provide a better context switch performance. You should call *co_enter* in the thread that you call *co_create*.

TODO
----
- Support *Lock* in coroutines.
- Support *Sleep* in coroutines.

Evaluation
----------

