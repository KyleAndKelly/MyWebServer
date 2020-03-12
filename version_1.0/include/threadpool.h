#ifndef THREADPOOL
#define THREADPOOL
#include "requestData.h"
#include <pthread.h>

const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

typedef enum 
{
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct

 *  @var notify       Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var count        Number of pending tasks
 *  @var shutdown     Flag indicating if the pool is shutting down
 *  @var started      Number of started threads
 */
struct threadpool_t
{
    pthread_mutex_t lock;//互斥锁
    pthread_cond_t notify;//条件变量
    pthread_t *threads;//线程队列对象 用数组去表达 数组中每一个元素代表一个线程id
    threadpool_task_t *queue;//任务队列对象   用数组去表达 数组中每一个元素代表一个任务对象
     int thread_count;//线程数目
    int queue_size;//队列大小
    int head;//队列头索引
    int tail;//队列尾索引
    int count;//挂在队列中任务数目
    int shutdown;//标志线程池关闭
    int started;//已经启动的线程
};

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags);
int threadpool_destroy(threadpool_t *pool, int flags);
int threadpool_free(threadpool_t *pool);
static void *threadpool_thread(void *threadpool);

#endif