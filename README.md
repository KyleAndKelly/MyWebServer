# C++ Reactor Web Server
## Version1.0
### 编译
```
 git clone https://github.com/KyleAndKelly/MyWebServer.git
cd MyWebServer/version_1.0/&&mkdir build&&cd build
cmake ..
cd ../bin
./MyWebServer 
然后打开地址栏输入 127.0.0.1:8888 

```
### 架构

![在这里插入图片描述](https://img-blog.csdnimg.cn/20200312222549395.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3ZqaGdoamdoag==,size_16,color_FFFFFF,t_70)
点击查看大图
 其中线程池中工作线程取用任务队列里面的任务
也就是进行http报文分析 报文响应时候的流程如下
![在这里插入图片描述](https://img-blog.csdnimg.cn/20200312224128436.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3ZqaGdoamdoag==,size_16,color_FFFFFF,t_70)

### 源码
详细的源码讲解请看注释


### 特性

```
1. 在整个epoll监听循环开始之前  先屏蔽掉SIGPIPE信号

		 //默认读写一个关闭的socket会触发sigpipe信号 该信号的默认操作是关闭进程 这明显是我们不想要的
        //所以我们需要重新设置sigpipe的信号回调操作函数   比如忽略操作等  使得我们可以防止调用它的默认操作 
        //信号的处理是异步操作  也就是说 在这一条语句以后继续往下执行中如果碰到信号依旧会调用信号的回调处理函数
		//处理sigpipe信号
void handle_for_sigpipe()
{
    struct sigaction sa; //信号处理结构体
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;//设置信号的处理回调函数 这个SIG_IGN宏代表的操作就是忽略该信号 
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))//将信号和信号的处理结构体绑定
        return;
}

```

```
2.用setsockopt(listen_fd, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval)消除bind时"Address already in use"错误
即设置SO_REUSEADDR 重用本地地址
```

```
3. epoll监管套接字的时候用边沿触发+EPOLLONESHOT+非阻塞IO   

```

```
5. 使用多线程充分利用多核CPU，并使用线程池避免线程频繁创建销毁的开销
创建一个线程池  线程池中主要包含任务队列 和工作线程集合  使用了一个固定线程数的工作线程
工作线程之间对任务队列的竞争采用条件变量和互斥锁结合使用
一个工作线程先先加互斥锁 当任务队列中任务数量为0时候 阻塞在条件变量,
当任务数量大于0时候 用条件变量通知阻塞在条件变量下的线程 这些线程来继续竞争获取任务
对任务队列中任务的调度采用先来先服务算法
```
```
5.采用reactor模式 主线程只负责IO  获取io请求后把请求对象给工作线程 工作线程负责数据读取以及逻辑处理
```

```
6. 在主线程循环监听到读写套接字有报文传过来以后 在工作线程调用requestData中的handleRequest进行使用状态机解析了HTTP请求
http报文解析和报文响应 解析过程状态机如上图所示. 
值得注意的是

(1)这里支持了两种类型GET和POST报文的解析 

get报文
GET /0606/01.php HTTP/1.1\r\n	请求行
Host: localhost\r\n					首部行 首部行后面还有其他的这里忽略
\r\n								空行分割
空								实体主体

post报文
POST /0606/02.php HTTP/1.1 \r\n   请求行
Host: localhost\r\n             首部行 首部行中必须有Contenr-length，告诉服务器我要给你发的实体主体有多少字节 
Content-type: application/x-www-form-urlencoded\r\n
Contenr-length: 23\r\n				
 \r\n                                                       空行分割
username=zhangsan&age=9	\r\n		实体主体

根据http请求报文的请求行去判断是请求类型字符串是"GET"还是"POST"
然后用一个map存放首部行的键值对数据
如果是post报文的话 首部行里面必然会有Content-length字段而get没有 所以取出这个字段 求出后面实体主体时候要取用的长度 
然后往下走回送相应的http响应报文即可
而get报文 实体主体是空的 直接读取请求行的url数据 然后往下走回送相应的http响应报文即可




(2)在这是支持长连接 keep-alive
 在首部行读取出来数据以后如果请求方设置了长连接 则Connection字段为keep-alive以此作为依据
 如果读取到这个字段的话就在报文解析 报文回送完毕之后将requestData重置 
 然后将该套接字属性也用epoll_ctl重置 再次加入epoll监听

```
```
7.实现了一个小根堆的定时器及时剔除超时请求，使用了STL的优先队列来管理定时器
```
```
8.
锁的使用有两处：
第一处是任务队列的添加和取操作，都需要加锁，并配合条件变量，跨越了多个线程。
第二处是定时器结点的添加和删除，需要加锁，主线程和工作线程都要操作定时器队列。
```

### 性能
![在这里插入图片描述](https://img-blog.csdnimg.cn/20200313014124381.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3ZqaGdoamdoag==,size_16,color_FFFFFF,t_70)
用webbench压测工具 
并发1000个请求 压测30s 用get请求 且用长连接 对server进行压测
结果如下


