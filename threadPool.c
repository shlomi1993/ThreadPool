// Shlomi Ben-Shushan

// Include header file.
#include "threadPool.h"

// Define error message.
#define ERROR_MSG "Error in system call"

// Define thread-pool's possible statuses.
#define READY 1
#define FREED 2
#define RUNNING 3
#define DYING 4

// This struct defines a task which is a function with specific arguments.
typedef struct Task {
    void (*function)(void *);
    void *arguments;
} Task;

/**********************************************************************************
* Function:     startThread
* Input:        A void* pointer to arguments.
* Output:       A void* pointer to output.
* Operation:    This function "ignite" a thread.
*               It is passed as argument to the function pthread_create().
***********************************************************************************/
void* startThread(void* threadPool) {

    // Cast the ThreadPool type given by void* pointer, to a ThreadPool type.
    ThreadPool *tp = (ThreadPool *) threadPool;

    // Lock mutex and run task as long as thread pool is not freed.
    while (tp->status != FREED) {

        // Case 1 -- tpDestroy() has already called and the queue is empty.
        if (tp->status == DYING && osIsQueueEmpty(tp->queue)) {
            break;
        }

        // Case 2 -- the thread pool is running (or ready) and the queue is still empty.
        else if ((tp->status == RUNNING || tp->status == READY) && osIsQueueEmpty(tp->queue)) {
            if (pthread_mutex_lock(&tp->lock) != 0) {
                perror(ERROR_MSG);
                break;
            }
            if (pthread_cond_wait(&(tp->condition), &tp->lock) != 0) {
                perror(ERROR_MSG);
                pthread_mutex_unlock(&tp->lock);
                break;
            }
        }

        // Case 3 -- Otherwise.
        else {
            if (pthread_mutex_lock(&tp->lock) != 0) {
                perror(ERROR_MSG);
                break;
            }
        }

        // If queue is empty, unlock the mutex and break.
        if (osIsQueueEmpty(tp->queue)) {
            if (pthread_mutex_unlock(&tp->lock) != 0) {
                perror(ERROR_MSG);
                break;
            }
        }

        // Else, run a task from it.
        else {
            Task *task = (Task *) osDequeue(tp->queue);
            if (pthread_mutex_unlock(&tp->lock) != 0) {
                perror(ERROR_MSG);
                break;
            }
            task->function(task->arguments);
            free(task);
        }

        // This case happen when tpDestroy() is called with number 0.
        if (tp->status == READY) {
            break;
        }

    }

    // Return nothing.
    return NULL;

}

/**********************************************************************************
* Function:     tpCreate
* Input:        integer number of threads to load.
* Output:       A pointer to a READY ThreadPool type.
* Operation:    This function creates a READY Thread Pool and initialize its fields.
***********************************************************************************/
ThreadPool* tpCreate(int numOfThreads){

    // Sanity check.
    if (numOfThreads < 1) {
        perror("Error: Illegal numOfThreads");
        return NULL;
    }

    // Allocate memory for the thread pool.
    ThreadPool *tp = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (tp == NULL) {
        perror(ERROR_MSG);
        return NULL;
    }

    // Initialize a queue.
    tp->queue = osCreateQueue();
    if (tp->queue == NULL) {
        perror(ERROR_MSG);
        free(tp);
        return NULL;
    }

    // Initialize condition.
    if (pthread_cond_init(&tp->condition,NULL) != 0) {
        perror(ERROR_MSG);
        osDestroyQueue(tp->queue);
        free(tp);
        return NULL;
    }

    // Initialize lock (mutex).
    if (pthread_mutex_init(&tp->lock,NULL) != 0) {
        perror(ERROR_MSG);
        pthread_cond_destroy(&tp->condition);
        osDestroyQueue(tp->queue);
        free(tp);
        return NULL;
    }

    // Initialize another lock -- a destroy lock.
    if (pthread_mutex_init(&tp->destroyLock,NULL) != 0) {
        perror(ERROR_MSG);
        pthread_mutex_destroy(&tp->lock);
        pthread_cond_destroy(&tp->condition);
        osDestroyQueue(tp->queue);
        free(tp);
        return NULL;
    }

    // Allocate memory for an array of threads.
    tp->threads = (pthread_t*)malloc(sizeof(pthread_t) * numOfThreads);
    if (tp->threads == NULL) {
        perror(ERROR_MSG);
        pthread_mutex_destroy(&tp->destroyLock);
        pthread_mutex_destroy(&tp->lock);
        pthread_cond_destroy(&tp->condition);
        osDestroyQueue(tp->queue);
        free(tp);
        return NULL;
    }

    // Initialize "primitive" fields.
    tp->size = numOfThreads;
    tp->destroyFlag = 0;
    tp->status = RUNNING;

    // Create numOfThreads threads.
    int i;
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(tp->threads[i]), NULL, startThread, tp) != 0) {
            perror(ERROR_MSG);
            tpDestroy(tp, 0);
        }
    }

    // If everything went well, return a pointer to the thread pool.
    return tp;

}

/**********************************************************************************
* Function:     tpInsertTask
* Input:        Thread pool, function and parameters for the function.
* Output:       0 for success, -1 otherwise.
* Operation:    This function creates a READY Task from the given function and
*               parameters, and insert it to the queue of the given thread pool.
***********************************************************************************/
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {

    // Block this function if destroyFlag is on.
    if (threadPool->destroyFlag) {
        return -1;
    }

    // Allocate memory for a new Task.
    Task *task = (Task *) malloc(sizeof(Task));
    if (task == NULL) {
        perror(ERROR_MSG);
        return -1;
    }

    // Assign the given function and parameters to the Task fields.
    task->function = computeFunc;
    task->arguments = param;

    // lock the given thread-pool's mutex.
    if (pthread_mutex_lock(&threadPool->lock) != 0) {
        perror(ERROR_MSG);
        free(task);
        return -1;
    }

    // Add the task to the queue.
    osEnqueue(threadPool->queue, task);

    // Make a signal.
    if (pthread_cond_signal(&threadPool->condition) != 0) {
        perror(ERROR_MSG);
        free(task);
        pthread_mutex_unlock(&threadPool->lock);
        return -1;
    }

    // Unlock mutex.
    if (pthread_mutex_unlock(&threadPool->lock) != 0) {
        perror(ERROR_MSG);
        free(task);
        return -1;
    }

    // If everything went well, return 0.
    return 0;

}

/**********************************************************************************
* Function:     tpDestroy
* Input:        A thread pool and a shouldWaitForTasks flag.
* Output:       void.
* Operation:    This function destroy the given thread pool in one of two possible
*               ways -- immediately while stop all the threads, or not-immediately
*               which means just after all the threads are done.
***********************************************************************************/
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){

    // First, lock the destroyLock in order to prevent another tpDestroy() call.
    if (pthread_mutex_lock(&threadPool->destroyLock) != 0) {
        perror(ERROR_MSG);
        return;
    }

    // If the destroy flag is already on, it means other thread run tpDestroy() so return.
    if (threadPool->destroyFlag) {
        return;
    }

    // Now we are sure that destroyFlag is off so we can release the destroyLock.
    if (pthread_mutex_unlock(&threadPool->destroyLock) != 0) {
        perror(ERROR_MSG);
        return;
    }

    // Lock the "regular" mutex lock to use the thread pool.
    if (pthread_mutex_lock(&threadPool->lock) != 0) {
        perror(ERROR_MSG);
        return;
    }

    // Broadcast condition to all of the threads.
    if (pthread_cond_broadcast(&threadPool->condition) != 0) {
        perror(ERROR_MSG);
        return;
    }

    // Unlock "regular" mutex.
    if (pthread_mutex_unlock(&threadPool->lock) !=0 ) {
        perror(ERROR_MSG);
        return;
    }

    // Change thread-pool's status according to the given shouldWaitForTasks number.
    if (shouldWaitForTasks) {
        threadPool->status = DYING;
    } else {
        threadPool->status = READY;
    }

    // Turn on the destroy flag to block other threads.
    threadPool->destroyFlag = 1;

    // Use pthread_join to wait for the running threads to finish.
    int i;
    for (i = 0; i < threadPool->size; ++i) {
        if (pthread_join(threadPool->threads[i], NULL) != 0) {
            perror(ERROR_MSG);
            return;
        }
    }

    // Clear thread-pool's queue, release locks and deallocate memory.
    threadPool->status = FREED;
    while (!osIsQueueEmpty(threadPool->queue)) {
        Task* task = (Task *) osDequeue(threadPool->queue);
        free(task);
    }
    free(threadPool->threads);
    pthread_mutex_destroy(&threadPool->lock);
    pthread_mutex_destroy(&threadPool->destroyLock);
    pthread_cond_destroy(&threadPool->condition);
    osDestroyQueue(threadPool->queue);
    free(threadPool);

}