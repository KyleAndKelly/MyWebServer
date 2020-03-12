#include "threadpool.h"

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags) //线程池创建
{
    threadpool_t *pool=NULL;//线程池对象
    int i;
    do
    {
        
        if(thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE) {
            return NULL;
        }
        /* 创建一个线程池对象 */
        if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) //
        {
            break;
            
        }
    
        /* 初始化线程池参数 */
        pool->thread_count = 0;   //初始化线程数量
        pool->queue_size = queue_size;//初始化队列大小
        pool->head = pool->tail = pool->count = 0;//初始化队列头 尾 队列中任务数量
        pool->shutdown = pool->started = 0;//初始化启动和关闭标志
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);//初始化线程池中线程队列
        pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);//初始化线程池中任务队列 
    
        if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||//初始化线程池中锁 和条件变量
           (pthread_cond_init(&(pool->notify), NULL) != 0) ||
           (pool->threads == NULL) ||
           (pool->queue == NULL)) 
        {
            break;
          
        }
    
         /*启动线程池中的线程 */
        for(i = 0; i < thread_count; i++) 
        {
            if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0) //初始化(启动)线程池中每个线程  同时绑定线程运行的函数为threadpool_thread  该函数的参数是pool
            {
                threadpool_destroy(pool, 0);//如果没有创建成功则销毁
                return NULL;
            }
            pool->thread_count++; //如果创建成功则线程数目++
            pool->started++;//如果创建成功则start++
        }
          return pool;
    } while(false);//使用do{...}while(false)结构可以简化多级判断时代码的嵌套。
    
    if (pool != NULL)  //如果线程池创建失败 则释放
    {
        threadpool_free(pool);
    }
    return NULL;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags) 
{
    //printf("add to thread pool !\n");
    int err = 0;
    int next;
    //(void) flags;
    if(pool == NULL || function == NULL)
    {
        return THREADPOOL_INVALID;
    }
    if(pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return THREADPOOL_LOCK_FAILURE;
    }
    next = (pool->tail + 1) % pool->queue_size;
    do 
    {
        /* Are we full ? */
        if(pool->count == pool->queue_size) {
            err = THREADPOOL_QUEUE_FULL;
            break;
        }
        /* Are we shutting down ? */
        if(pool->shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        /* Add task to queue */
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count += 1;
        
        /* pthread_cond_broadcast */
        if(pthread_cond_signal(&(pool->notify)) != 0) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while(false);

    if(pthread_mutex_unlock(&pool->lock) != 0) {
        err = THREADPOOL_LOCK_FAILURE;
    }

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    printf("Thread pool destroy !\n");
    int i, err = 0;

    if(pool == NULL)
    {
        return THREADPOOL_INVALID;
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) 
    {
        return THREADPOOL_LOCK_FAILURE;
    }

    do 
    {
        /* Already shutting down */
        if(pool->shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }

        pool->shutdown = (flags & THREADPOOL_GRACEFUL) ?
            graceful_shutdown : immediate_shutdown;

        /* Wake up all worker threads */
        if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }

        /* Join all worker thread */
        for(i = 0; i < pool->thread_count; ++i)
        {
            if(pthread_join(pool->threads[i], NULL) != 0)
            {
                err = THREADPOOL_THREAD_FAILURE;
            }
        }
    } while(false);

    /* Only if everything went well do we deallocate the pool */
    if(!err) 
    {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool)
{
    if(pool == NULL || pool->started > 0)
    {
        return -1;
    }

    /* Did we manage to allocate ? */
    if(pool->threads) 
    {
        free(pool->threads);
        free(pool->queue);
 
        /* Because we allocate pool->threads after initializing the
           mutex and condition variable, we're sure they're
           initialized. Let's lock the mutex just in case. */
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);    
    return 0;
}


static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;
    for(;;)
    {
     /* 根据条件变量和互斥锁线程调度抢占 */


        pthread_mutex_lock(&(pool->lock));//加互斥锁避免多个线程占用条件变量
        while((pool->count == 0) && (!pool->shutdown)) //如果线程池中的任务队列暂时任务数量为0 用while防止虚假唤醒
        {
            pthread_cond_wait(&(pool->notify), &(pool->lock));//则先释放互斥锁 同时根据条件变量等待阻塞 当任务数量大于0时 恢复运行 再次获取互斥锁进入任务处理环节
        }



     /* 该线程获取执行权以后 处理开始任务处理环节 */
        if((pool->shutdown == immediate_shutdown) ||  //如果此时线程池已经标志关闭了 则该线程退出
        ((pool->shutdown == graceful_shutdown) &&
            (pool->count == 0)))
        {
            break;
        }

      
        task.function = pool->queue[pool->head].function;//如果此时线程池还没有标志关闭 则从任务队列头取任务来处理 // 指定任务的处理函数
        task.argument = pool->queue[pool->head].argument;//指定任务的处理函数的参数
        pool->head = (pool->head + 1) % pool->queue_size; //调整队列头
        pool->count -= 1;//减少队列数目
        pthread_mutex_unlock(&(pool->lock));   //释放互斥锁 
        (*(task.function))(task.argument);// 执行任务处理函数

        
    }

    --pool->started;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}