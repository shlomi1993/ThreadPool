# ThreadPool

This repository documents my implementation of Thread Pool in C programming language.
This implementation is a solution to an assignment I have been given as part of Operation System course I took at Bar-Ilan University.


## From Wikipedia

In computer programming, a thread pool is a software design pattern for achieving concurrency of execution in a computer program. Often also called a replicated workers or worker-crew model, a thread pool maintains multiple threads waiting for tasks to be allocated for concurrent execution by the supervising program. By maintaining a pool of threads, the model increases performance and avoids latency in execution due to frequent creation and destruction of threads for short-lived tasks. The number of available threads is tuned to the computing resources available to the program, such as a parallel task queue after completion of execution.

To read more: https://en.wikipedia.org/wiki/Thread_pool


## Files

1. threadPool.c - my implementation of thread pool in C.
2. threadPool.h - the header file for threadPool.c.
3. test.c - the program is n API so there is no main.c to test is, so test.c file is required for debugging/presentation.
4. osqueue.c - a basic queue implementation  I've not implemented this file, the T.As gave it to the students.
5. osqueue.h - the header file for osqueue.c, I've not implemented this file.


## IDE and tools

1. Visual Studio Code
2. Notepad++
