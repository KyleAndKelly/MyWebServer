#include "epoll.h"
#include <sys/epoll.h>
#include <errno.h>
#include "threadpool.h"

struct epoll_event* events;

int epoll_init()
{
    int epoll_fd = epoll_create(LISTENQ + 1); //创建一个epoll事件表
    if(epoll_fd == -1)
        return -1;
    //events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    events = new epoll_event[MAXEVENTS];//创建一个监听事件结构体数组
    return epoll_fd;
}

// 注册新描述符
int epoll_add(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.ptr = request;//套接字收到的数据存放位置
    event.events = events;//套接字被监听的事件
    //printf("add to epoll %d\n", fd);
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_add error");
        return -1;
    }
    return 0;
}

// 修改描述符状态
int epoll_mod(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        perror("epoll_mod error");
        return -1;
    } 
    return 0;
}

// 从epoll中删除描述符
int epoll_del(int epoll_fd, int fd, void *request, __uint32_t events)
{
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        perror("epoll_del error");
        return -1;
    } 
    return 0;
}

// 返回活跃事件数
int my_epoll_wait(int epoll_fd, struct epoll_event* events, int max_events, int timeout)//epoll_fd存放epoll事件表描述符events存放就绪的套接字的事件结构体(可以从里面判断就绪的事件和获取传过来的数据)
{
    int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
    if (ret_count < 0)
    {
        perror("epoll wait error");
    }
    return ret_count;
}