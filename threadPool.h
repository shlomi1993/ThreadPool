// Shlomi Ben-Shushan

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "osqueue.h"


typedef struct thread_pool {
    char status;                    // A thread pool has 4 statuses -- Ready, Running, Freed (renew) and Dying.
    pthread_t *threads;             // A thread pool contains an array of threads.
    int size;                       // This field indicate the number of threads in "threads" array.
    OSQueue *queue;                 // A thread pool has a queue for the waiting tasks.
    pthread_mutex_t lock;           // A mutex is needed to sync between threads.
    pthread_mutex_t destroyLock;    // A different mutex is needed to support thread pool's destructor. .
    char destroyFlag;               // When this flag is on, it means destructing procedure has begun.
    pthread_cond_t condition;       // A condition variable is required for the thread pool.
} ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
